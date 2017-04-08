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


#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <memory>

#include <libpmem.h>

/* internal includes */
#include "../../logging.h"
#include "../../utils.hpp"
#include "file.h"
#include "mapping.h"
#include "snapshot.h"
#include "../posix-file.h"
#include "nvram-nvml.h"

namespace boost { 
namespace filesystem {

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


namespace {

std::string compute_prefix(const bfs::path& rootdir, const bfs::path& basepath){
    const bfs::path relpath = make_relative(rootdir, basepath);
    std::string mp_prefix = relpath.string();

// due to an obscure reason, compiling with -ggdb3 -O0 produces linking errors
// against with boost::filesystem::path::preferred_separator
#ifndef __DEBUG__
    std::replace(mp_prefix.begin(), mp_prefix.end(), bfs::path::preferred_separator, '_');
#else
    std::replace(mp_prefix.begin(), mp_prefix.end(), '/', '_');
#endif /* __DEBUG__ */

    return mp_prefix;
}

} // anonymous namespace

namespace efsng {
namespace nvml {

nvml_backend::nvml_backend(uint64_t size, bfs::path daxfs_mount, bfs::path root_dir)
    : backend(size),
      m_daxfs_mount_point(daxfs_mount),
      m_root_dir(root_dir) {
}

nvml_backend::~nvml_backend(){
}

uint64_t nvml_backend::get_capacity() const {
    return m_capacity;
}

/** start the preload process of a file requested by the user */
void nvml_backend::preload(const bfs::path& pathname){

    BOOST_LOG_TRIVIAL(debug) << "Loading file " << pathname << " into NVM..."; 

    /* open the file */
    posix::file fd(pathname);

    /* compute an appropriate path for the mapping file */
    std::string mp_prefix = ::compute_prefix(m_root_dir, pathname);

    /* create a mapping and fill it with the current contents of the file */
    mapping mp(mp_prefix, m_daxfs_mount_point, fd.get_size());
    mp.populate(fd);
    fd.close();

    /* add the mapping to a nvml::file descriptor and insert it into m_files */
    BOOST_LOG_TRIVIAL(debug) << "Transfer complete (" << mp.m_bytes << ")";

    std::lock_guard<std::mutex> lock(m_files_mutex);

    //XXX consider using the TBB concurrent_map to discriminate between locking
    // readers and writers
    m_files.insert(std::make_pair(
        pathname.c_str(),
        std::make_unique<nvml::file>(mp)
        )
    );

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
void nvml_backend::read_data(const backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const {

    const nvml::file& f = dynamic_cast<const nvml::file&>(file);

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

        if(offset <= mp_offset) {
            buf_start = mp_data;
        }
        else {
            buf_start = (uintptr_t*)mp_data + offset - mp_offset;
        }

        if(offset + size >= mp_offset + mp_size) {
            buf_size = mp_size;
        }
        else {
            // if the mapping contains less data than 
            // requested we need to account for it
            buf_size = std::min(offset + size - mp_offset, mp_bytes);
        }

        bufmap.emplace_back(buf_start, buf_size);
    }

    // XXX WARNING: the snapshots are currently deleted here. We don't know 
    // (yet) if this is really what we need
    std::cerr << "Deleting snapshots!\n";

}

void nvml_backend::write_data(const backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const {
    (void) file;
    (void) offset;
    (void) size;
    (void) bufmap;
    //TODO
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
