/*************************************************************************
 * (C) Copyright 2016-2017 Barcelona Supercomputing Center               *
 *                         Centro Nacional de Supercomputacion           *
 *                                                                       *
 * This file is part of the Echo Filesystem NG.                          *
 *                                                                       *
 * See AUTHORS file in the top level directory for information           *
 * regarding developers and contributors.                                *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU Lesser General Public            *
 * License as published by the Free Software Foundation; either          *
 * version 3 of the License, or (at your option) any later version.      *
 *                                                                       *
 * The Echo Filesystem NG is distributed in the hope that it will        *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied         *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR               *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Echo Filesystem NG; if not, write to the Free      *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    *
 *                                                                       *
 *************************************************************************/

#include <nvram-nvml/file.h>

namespace efsng {
namespace nvml {


file::file() 
    : backend::file(),
      m_offset_tree(0, std::numeric_limits<off_t>::max(), nullptr) {
}

file::file(const bfs::path& pathname, mapping& mp, struct ::stat& stbuf, file::type type) 
    : efsng::backend::file(),
      m_pathname(pathname),
      m_type(type),
      m_offset_tree(0, std::numeric_limits<off_t>::max(), nullptr) {

    m_mappings.emplace_back(std::move(mp));

    // get a reference to the instance that we just added to the list
    auto& new_mp = m_mappings.back();

    std::cerr << "Insert [" << new_mp.m_offset << ", " << new_mp.m_offset + new_mp.m_size << "] = " << &new_mp << "\n";
    m_offset_tree.insert_back(new_mp.m_offset, new_mp.m_offset + new_mp.m_size, &new_mp);
    m_offset_tree.build_tree();

    save_attributes(stbuf);
}

file::~file(){
    std::cerr << "a nvml::file instance died...\n";
}

// these are the public interface in case we need to call them from FUSE
// internally we can use scoped locks
void file::acquire_read_lock() const {
    m_mutex.lock_shared();
}

void file::release_read_lock() const {
    m_mutex.unlock_shared();
}

void file::acquire_write_lock() const {
    m_mutex.lock();
}

void file::release_write_lock() const {
    m_mutex.unlock();
}

void file::save_attributes(struct stat& stbuf) {
    memcpy(&m_attributes, &stbuf, sizeof(m_attributes));
}

lock_manager::range_lock file::lock_range(off_t start, off_t end, operation op) {

    if(op == operation::read) {
        return m_lock_manager.lock_range(start, end, lock_manager::type::reader);
    }

    assert(op == operation::write);
    return m_lock_manager.lock_range(start, end, lock_manager::type::writer);
}

void file::unlock_range(lock_manager::range_lock& rl) {
    m_lock_manager.unlock_range(rl);
}


void file::stat(struct stat& stbuf) const {
    memcpy(&stbuf, &m_attributes, sizeof(stbuf));
}

size_t file::get_size() const {

    assert(!m_mappings.empty());

    // since we only add one mapping at a time, we only need to update
    // the last one
    const auto& last_mp = m_mappings.back();

    std::cerr << "get_size() = " << last_mp.m_offset + last_mp.m_bytes << "\n";

    return last_mp.m_offset + last_mp.m_bytes;
}

void file::set_size(size_t size) {

    std::cerr << "Called with " << size << "\n";

    auto& last_mp = m_mappings.back();

    last_mp.m_bytes = size - last_mp.m_offset;

    // update cached attributes
    m_attributes.st_size = size;
    m_attributes.st_blocks = size/512;
}

void file::add(mapping&& mp) {

    assert(!m_mappings.empty());

    //XXX we may need a mutex here

    auto& prev_mp = m_mappings.back();

    prev_mp.m_bytes = prev_mp.m_size;

    // calculate the offset for the file region covered by this mapping
    mp.m_offset = prev_mp.m_offset + prev_mp.m_size;

    m_mappings.push_back(std::move(mp));

    // get a reference to the instance that we just added to the list
    auto& new_mp = m_mappings.back();

    m_offset_tree_mutex.lock();

    std::cerr << "Insert [" << new_mp.m_offset << ", " << new_mp.m_offset + new_mp.m_size << "] = " << &new_mp << "\n";
    m_offset_tree.insert_back(new_mp.m_offset, new_mp.m_offset + new_mp.m_size, &new_mp);
    m_offset_tree.build_tree();

    m_offset_tree_mutex.unlock();
}

void file::extend(off_t end_offset, const std::string& prefix, const bfs::path& base_path) {

    //XXX we may need a mutex here
    off_t eof_offset = get_size();
    size_t new_bytes = end_offset - eof_offset;
    size_t remaining_bytes = m_mappings.back().m_size - m_mappings.back().m_bytes;

    // we need a new mapping
    if(new_bytes > remaining_bytes) {
        mapping mp(prefix, base_path, new_bytes - remaining_bytes);
        this->add(std::move(mp));
    }
}

void file::range_lookup(off_t range_start, off_t range_end, backend::buffer_map& bmap) const {

    m_offset_tree_mutex.lock_shared();

    assert(range_start <= range_end);
    assert(m_offset_tree.is_tree_valid());

    mapping* mp_ptr;
    auto res = m_offset_tree.search_tree(range_start, mp_ptr);

    // the offset must exist, since the offset_tree covers from 0 to 
    // std::numeric_limits<off_t>::max(). If a non-allocated offset is accessed,
    // the associated mapping will be nullptr
    assert(res.second == true);

    offset_tree_t::const_iterator& it = res.first;
    size_t req_size = range_end - range_start;

    if(req_size == 0){
        return;
    }

    do {
        const auto& s = it->second;

        // out of allocated area
        if(s == nullptr){
            break;
        }

        //std::cerr << "Considering mapping:\n" << (*s) << "\n";


        // mapping created but not populated (should not happen)
        if(s->m_bytes == 0){
            break;
        }

        off_t s_start = it->first;
        off_t s_end = s_start + s->m_size;
        off_t s_delta = range_start - s_start;
        // we need ssize_t given that 'delta' might be larger than 'm_bytes'
        ssize_t s_size = std::min({(ssize_t) req_size, 
                                   (ssize_t) (s->m_bytes - s_delta), 
                                   (ssize_t) (s->m_size - s_delta)});
        data_ptr_t s_addr = (data_ptr_t) ((uintptr_t)s->m_data + s_delta);

        // s_delta == s.m_size
        if(s_size <= 0){
            break;
        }

        bmap.emplace_back(s_addr, s_size);

        if(s_end >= range_end){
            break;
        }

        range_start = s_end;
        req_size -= s_size;
        ++it;
    } while(true);

    m_offset_tree_mutex.unlock_shared();
}

void file::find_segments(off_t range_start, off_t range_end, backend::buffer_map& bmap) const {

    m_offset_tree_mutex.lock_shared();

    assert(range_start <= range_end);
    assert(m_offset_tree.is_tree_valid());

    mapping* mp_ptr;
    auto res = m_offset_tree.search_tree(range_start, mp_ptr);

    // the offset must exist, since the offset_tree covers from 0 to 
    // std::numeric_limits<off_t>::max(). If a non-allocated offset is accessed,
    // the associated mapping will be nullptr
    assert(res.second == true);

    offset_tree_t::const_iterator& it = res.first;
    size_t req_size = range_end - range_start;

    if(req_size == 0){
        return;
    }

    do {
        const auto& s = it->second;

        if(s == nullptr){
            break;
        }

        off_t s_start = it->first;
        off_t s_end = s_start + s->m_size;
        off_t s_delta = range_start - s_start;
        // we need ssize_t given that 'delta' might be larger than 'm_bytes'
        ssize_t s_size = std::min({(ssize_t) req_size, 
                                   (ssize_t) (s->m_size - s_delta)});
        data_ptr_t s_addr = (data_ptr_t) ((uintptr_t)s->m_data + s_delta);

        // s_delta == s.m_size
        if(s_size == 0){
            break;
        }

        bmap.emplace_back(s_addr, s_size);

        if(s_end >= range_end){
            break;
        }

        range_start = s_end;
        req_size -= s_size;
        ++it;
    } while(true);

    m_offset_tree_mutex.unlock_shared();
}

} // namespace nvml
} // namespace efsng


#ifdef __EFS_DEBUG__
std::ostream& operator<<(std::ostream& os, const efsng::nvml::segment& seg){

    os << "{"
       << "m_data_ptr: " << seg.m_data << ", "
       << "m_size: " << std::dec << seg.m_size << " (0x" << std::hex << seg.m_size << "), "
       << "m_bytes: " << std::dec << seg.m_bytes << " (0x" << std::hex << seg.m_bytes
       << "}";

    return os;
}
#endif /* __EFS_DEBUG__ */

