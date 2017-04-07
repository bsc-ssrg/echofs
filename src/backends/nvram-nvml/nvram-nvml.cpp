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
#include "posix-file.h"
#include "nvram-nvml.h"

//XXX move this to common header

namespace boost { namespace filesystem {

template <> path& path::append<path::iterator>(path::iterator begin, path::iterator end, const codecvt_type& cvt)
{

    (void) cvt;

    for( ; begin != end ; ++begin )
        *this /= *begin;
    return *this;
}

/* Return path when appended to a_From will resolve to same as a_To */
boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To )
{
    a_From = boost::filesystem::absolute( a_From ); a_To = boost::filesystem::absolute( a_To );
    boost::filesystem::path ret;
    boost::filesystem::path::const_iterator itrFrom( a_From.begin() ), itrTo( a_To.begin() );
    // Find common base
    for( boost::filesystem::path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ; itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo );
    // Navigate backwards in directory to reach previously found base
    for( boost::filesystem::path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom )
    {
        if( (*itrFrom) != "." )
            ret /= "..";
    }
    // Now navigate down the directory branch
    ret.append( itrTo, a_To.end() );
    return ret;
}

} } // namespace boost::filesystem




namespace efsng {
namespace nvml {

nvml_backend::nvml_backend(uint64_t size, bfs::path daxfs_mount, bfs::path root_dir)
    : Backend(size),
      m_daxfs_mount_point(daxfs_mount),
      m_root_dir(root_dir) {
}

nvml_backend::~nvml_backend(){
}

uint64_t nvml_backend::get_size() const {
    return max_size;
}

std::string nvml_backend::compute_prefix(const bfs::path& basepath){
    const bfs::path relpath = make_relative(m_root_dir, basepath);
    std::string mp_prefix = relpath.string();
    //std::replace(mp_prefix.begin(), mp_prefix.end(), bfs::path::preferred_separator, '_');
    std::replace(mp_prefix.begin(), mp_prefix.end(), '/', '_');

    return mp_prefix;
}

/** start the prefetch process of a file requested by the user */
void nvml_backend::prefetch(const bfs::path& pathname){

    BOOST_LOG_TRIVIAL(debug) << "Loading file " << pathname << " into NVM..."; 

    /* open the file */
    posix::file fd(pathname);

    /* compute an appropriate path for the mapping file */
    std::string mp_prefix = compute_prefix(pathname);

    /* create a mapping and fill it with the current contents of the file */
    mapping mp(mp_prefix, m_daxfs_mount_point, fd.get_size());
    mp.populate(fd);
    fd.close();

    /* add the mapping to m_files */
    BOOST_LOG_TRIVIAL(debug) << "Transfer complete (" << mp.m_bytes << ")";

    //auto foo = new nvml::file(mp);

    std::cerr << "temporary before insert\n";
    std::cerr << mp << "\n";

    auto rv = m_files.insert(std::make_pair(
        pathname.c_str(),
        std::make_unique<nvml::file>(mp)
        //nvml::file{mp}
        //std::move(*foo)
        )
    );

    std::cerr << "temporary after insert\n";
    std::cerr << mp << "\n";



    std::cerr << rv.second << "\n";

    auto xit = rv.first;

    auto fff = dynamic_cast<const nvml::file*>(xit->second.get());

    for(const mapping& mpp : fff->m_mappings) {

std::cerr << "Retrieved:\n";

std::cerr << "mapping{" << "\n";
std::cerr << "  m_name: " << mpp.m_name << "\n";
std::cerr << "  m_data: " << mpp.m_data << "\n";
std::cerr << "  m_offset: " << mpp.m_offset << "\n";
std::cerr << "  m_size: " << mpp.m_size << "\n";
std::cerr << "  m_bytes: " << mpp.m_bytes << "\n";
std::cerr << "  m_is_pmem: " << mpp.m_is_pmem << "\n";
std::cerr << "  m_copies.size(): " << mpp.m_copies.size() << "\n";
std::cerr << "  m_pmutex: " << mpp.m_pmutex.get() << "\n";
std::cerr << "};" << "\n";

        //std::cerr << mpp.m_pmutex << "\n";
        //std::cerr << mpp << "\n";

    }


#if 0
    { // test 
        const auto& it = m_files.find(pathname.c_str());

        assert(it != m_files.end());

        const auto& ff = static_cast<const nvml::file&>(it->second);

        for(const auto& mpp : ff.m_mappings) {
            //std::lock_guard<std::mutex>(*mpp.m_mutex);
        }

    }
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
void nvml_backend::read_data(const Backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const {

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

void nvml_backend::write_data(const Backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const {

}

Backend::iterator nvml_backend::find(const char* path) {
    return m_files.find(path);
}

Backend::iterator nvml_backend::begin() {
    return m_files.begin();
}

Backend::iterator nvml_backend::end() {
    return m_files.end();
}

Backend::const_iterator nvml_backend::cbegin() {
    return m_files.cbegin();
}

Backend::const_iterator nvml_backend::cend() {
    return m_files.cend();
}

/** lookup an entry */
bool nvml_backend::lookup(const char* pathname, void*& data_addr, size_t& size) const {

    (void) pathname;
    (void) data_addr;
    (void) size;

    const auto& it = m_files.find(pathname);

    if(it == m_files.end()){
        BOOST_LOG_TRIVIAL(debug) << "Prefetched data not found";
        return false;
    }

#if 0
    BOOST_LOG_TRIVIAL(debug) << "Prefetched data found at:" << it->second.data;

    data_addr = it->second.data;
    size = it->second.size;
#endif

    return true;
}

} // namespace nvml
} //namespace efsng
