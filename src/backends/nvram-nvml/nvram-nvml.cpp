/*************************************************************************
 * (C) Copyright 2016 Barcelona Supercomputing Center                    *
 *                    Centro Nacional de Supercomputacion                *
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

/* C includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>


#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <memory>

#include <libpmem.h>

/* internal includes */
#include <logging.h>
#include <utils.h>
#include <posix-file.h>
#include <nvram-nvml/file.h>
#include <nvram-nvml/segment.h>
#include <nvram-nvml/nvram-nvml.h>

#if BOOST_VERSION <= 106000 // 1.6.0
namespace boost { 
namespace filesystem {

/**********************************************************************************************************************/
/* helper functions                                                                                                   */
/**********************************************************************************************************************/

template <> 
path& path::append<path::iterator>(path::iterator begin, path::iterator end, const codecvt_type& cvt) {

    (void) cvt;

    for(; begin != end; ++begin)
        *this /= *begin;
    return *this;
}

/* Return path when appended to 'from_path' will resolve to same as 'to_path' */
path make_relative(path from_path, path to_path) {

    path ret;
    from_path = absolute(from_path);
    to_path = absolute(to_path);

    path::const_iterator from_it(from_path.begin()); 
    path::const_iterator to_it(to_path.begin());

    // Find common base
    for(path::const_iterator to_end(to_path.end()), from_end(from_path.end()); 
        from_it != from_end && to_it != to_end && *from_it == *to_it; 
        ++from_it, ++to_it
       );

    // Navigate backwards in directory to reach previously found base
    for(path::const_iterator from_end(from_path.end()); from_it != from_end; ++from_it) {
        if((*from_it) != ".")
            ret /= "..";
    }

    // Now navigate down the directory branch
    ret.append(to_it, to_path.end());
    return ret;
}

} 
} // namespace boost::filesystem
#endif

namespace {

std::string compute_prefix(const bfs::path& rootdir, const bfs::path& basepath){

#if BOOST_VERSION <= 106000 // 1.6.0
    const bfs::path relpath = make_relative(rootdir, basepath);
#else
    const bfs::path relpath = bfs::relative(basepath, rootdir);
#endif
    std::string mp_prefix = relpath.string();

// due to an obscure reason, compiling with -ggdb3 -O0 produces linking errors
// against with boost::filesystem::path::preferred_separator
#ifndef __EFS_DEBUG__
    std::replace(mp_prefix.begin(), mp_prefix.end(), bfs::path::preferred_separator, '_');
#else
    std::replace(mp_prefix.begin(), mp_prefix.end(), '/', '_');
#endif /* __EFS_DEBUG__ */

    return mp_prefix;
}

} // anonymous namespace


/**********************************************************************************************************************/
/* implementation                                                                                                     */
/**********************************************************************************************************************/
namespace efsng {
namespace nvml {

nvml_backend::nvml_backend(uint64_t capacity, bfs::path daxfs_mount, bfs::path root_dir, logger& logger)
    : m_capacity(capacity),
      m_logger(logger),
      m_daxfs_mount_point(daxfs_mount),
      m_root_dir(root_dir) {
}

nvml_backend::~nvml_backend(){
}

std::string nvml_backend::name() const {
    return s_name;
}

uint64_t nvml_backend::capacity() const {
    return m_capacity;
}

/** start the load process of a file requested by the user */
/* pathname = file path in underlying filesystem */
void nvml_backend::load(const bfs::path& pathname){

#ifdef __EFS_DEBUG__
    m_logger.debug("Loading file \"{}\" in NVRAM", pathname.string());
#endif

    /* add the mapping to a nvml::file descriptor and insert it into m_files */
    std::lock_guard<std::mutex> lock(m_files_mutex);

    /* create a new file into m_files (the constructor will fill it with
     * the contents of the pathname) */
    auto it = m_files.emplace(pathname.c_str(), 
                              std::make_unique<nvml::file>(m_daxfs_mount_point, pathname));

#ifdef __EFS_DEBUG__
    const auto& file_ptr = (*(it.first)).second;

    struct stat stbuf;
    file_ptr->stat(stbuf);
    m_logger.debug("Transfer complete ({} bytes)", stbuf.st_size);
#endif

    /* lock_guard is automatically released here */
}

bool nvml_backend::exists(const char* pathname) const {

    std::lock_guard<std::mutex> lock(m_files_mutex);

    const auto& it = m_files.find(pathname);

    // XXX it is safe to unlock the mutex here BECAUSE WE KNOW
    // that no other threads are going to remove a mapping from
    // the list; this happens because mappings are ONLY destroyed
    // when the std::list destructor is called (and we make sure that
    // this is done in a thread-safe manner). If this assumption ever
    // changes, we may NEED to revise this.
    return it != m_files.end();
}

/* build and return a list of all mapping regions affected by the read() operation */
void nvml_backend::read_prepare(const backend::file& file, off_t offset, size_t size, buffer_map& bmap) const {

#ifdef __EFS_DEBUG__
    m_logger.debug("nvml_backend::read_prepare(file, {}, {})", offset, size);
#endif

//    const nvml::file& f = dynamic_cast<const nvml::file&>(file);
//
//    f.range_lookup(offset, offset+size, bmap);

#if 0 // old version with snapshots -- discarded
    std::list<snapshot> snaps;

    for(const auto& mp : f.m_mappings) {

        // lock the mapping to make sure that there are no changes between
        // checking if we should process it and creating an snapshot for it
        std::lock_guard<std::mutex> lock(*mp.m_pmutex);

        // if the mapping exceeds the region affected by 
        // the operation, we can stop
        if(mp.m_offset >= (off_t) (offset + size)) {
            break;
        }

        // check if the mapping is affected by the operation and, if so,
        // create a snapshot of its current state and store it for later use
        if(mp.overlaps(offset, size)) {
            snaps.emplace_back(mp);
        }

        // lock is released here
    }

#ifdef __EFS_DEBUG__
    m_logger.debug("snapshots = {");
    for(const auto& sn : snaps){
        m_logger.debug("  {}", sn);
    }
    m_logger.debug("};");
#endif

    // from this point on, threads should work EXCLUSIVELY with 
    // the snapshots to compute the data that must be returned to the user,
    // for the sake of reducing contention. We now build a buffer map based 
    // on the snapshots collected
    for(const auto& sn : snaps) {

        data_ptr_t buf_start = 0;
        size_t buf_size = 0;

        data_ptr_t mp_data = sn.m_data;
        off_t mp_offset = sn.m_offset;
        size_t mp_size = sn.m_size;
        size_t mp_bytes = sn.m_bytes;
        off_t delta = 0;

        /* compute the address where the read should start */
        if(offset <= mp_offset){
            delta = 0;
        }
        else { /* offset > mp_offset */
            delta = offset - mp_offset;
        }
        
        buf_start = (data_ptr_t)((uintptr_t) mp_data + delta);

        assert(mp_bytes <= mp_size);

        /* compute how much to read from this mapping */
        if(offset + size <= mp_offset + mp_bytes) {
            buf_size = offset + size - mp_offset - delta;
        }
        else { /* offset + size > mp_offset + mp_bytes */
            buf_size = mp_bytes - delta;
        }

#ifdef __EFS_DEBUG__
        m_logger.debug("{} + {} => {}, {}", offset, size, buf_start, buf_size);
#endif

        bmap.emplace_back(buf_start, buf_size);
    }

    // XXX WARNING: the snapshots are currently deleted here. We don't know 
    // (yet) if this is really what we need
#ifdef __EFS_DEBUG__
    m_logger.debug("Deleting snapshots!");
#endif
#endif
}

void nvml_backend::read_finalize(const backend::file& file, off_t offset, size_t size, buffer_map& bmap) const {
    (void) file;
    (void) offset;
    (void) size;
    (void) bmap;
}

void nvml_backend::write_prepare(backend::file& file, off_t start_offset, size_t size, buffer_map& bmap) const {
    (void) file;

#if 0

#ifdef __EFS_DEBUG__
    m_logger.debug("nvml_backend::write_prepare(file, {}, {})", start_offset, size);
#endif

    nvml::file& f = dynamic_cast<nvml::file&>(file);

    off_t end_offset = start_offset + size;
    off_t eof_offset = f.size();

    if(end_offset > eof_offset) {
        f.extend(start_offset, size);
    }

//    f.find_segments(start_offset, end_offset, bmap);
#endif
}


void nvml_backend::write_finalize(backend::file& file, off_t start_offset, size_t size, buffer_map& bmap) const {
    (void) file;

#if 0
#ifdef __EFS_DEBUG__
    m_logger.debug("nvml_backend::write_finalize(file, {}, {})", start_offset, size);
#endif

    // XXX avoid this dynamic_cast by adding set_size to backend::file
    nvml::file& f = dynamic_cast<nvml::file&>(file);

    // we don't need a lock here for size/set_size because we are in 
    // the context of a lock_range call
    off_t end_offset = start_offset + size;
    off_t eof_offset = f.size();

    if(end_offset > eof_offset) {
        f.set_size(end_offset);
    }

#if 0

#ifdef __EFS_DEBUG__
    m_logger.debug("nvml_backend::write_finalize(file, {}, {})", start_offset, size);
#endif

    nvml::file& f = dynamic_cast<nvml::file&>(file);

    // create a new log entry that points to the buffers
    // created by write_prepare
    data_ptr_t dst_buf = bmap.front().m_data;
    off_t dst_offset = bmap.front().m_offset;
    size_t dst_size = bmap.front().m_size;

#if 0 // not needed because start_offset == b.m_offset in efs-ng.cpp
    // the file was zero-extended
    if(dst_offset != start_offset){
        start_offset = dst_offset;
    }
#endif

    off_t end_offset = start_offset + size;
    off_t start_block = align(start_offset, FUSE_BLOCK_SIZE);
    off_t end_block = xalign(end_offset, FUSE_BLOCK_SIZE);

    //XXX mutex!
    //XXX mutex!
    //XXX mutex!
    f.m_write_log.emplace_back(dst_offset, dst_size, dst_buf);

    std::cerr << "Before\n";
    for(auto it = f.m_offset_tree.begin();
        it != f.m_offset_tree.end();
        ++it){

        std::cerr << "{" << it->first << ": ";
        std::cerr << it->second << "}\n";
    }

    f.m_offset_tree.insert_back(start_block, end_block, {dst_buf, dst_size, dst_size});
    f.m_offset_tree.build_tree();

    std::cerr << "After\n";
    for(auto it = f.m_offset_tree.begin();
        it != f.m_offset_tree.end();
        ++it){

        std::cerr << "{" << std::dec << it->first << std::hex << " (0x" << it->first << "): ";
        std::cerr << it->second << "}\n";
    }


    f.m_size = end_offset;
#endif
#endif
}

backend::iterator nvml_backend::find(const char* path) {
    return m_files.find(path);
}

backend::iterator nvml_backend::begin() {
    return m_files.begin();
}

backend::iterator nvml_backend::end() {
    return m_files.end();
}

backend::const_iterator nvml_backend::cbegin() {
    return m_files.cbegin();
}

backend::const_iterator nvml_backend::cend() {
    return m_files.cend();
}

} // namespace nvml
} //namespace efsng
