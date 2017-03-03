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


#include <boost/filesystem/fstream.hpp>

#include <libpmem.h>

/* internal includes */
#include "../../logging.h"
#include "nvram-cache.h"

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

NVRAM_cache::NVRAM_cache(int64_t size, bfs::path dax_fs_base, bfs::path root_dir)
    : Backend(size),
      dax_fs_base(dax_fs_base),
      root_dir(root_dir) {
}

NVRAM_cache::~NVRAM_cache(){
}

uint64_t NVRAM_cache::get_size() const {
    return max_size;
}

/** start the prefetch process of a file requested by the user */
void NVRAM_cache::prefetch(const bfs::path& pathname){

    BOOST_LOG_TRIVIAL(debug) << "Prefetching file " << pathname;

    /* open the file */
    int fd = open(pathname.c_str(), O_RDONLY);

    if(fd == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to preload file " << pathname << ": " << strerror(errno);
        return;
    }

    /* determine the size of the file */
    struct stat stbuf;

    if(fstat(fd, &stbuf) == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to determine the size of file " << pathname << ": " << strerror(errno);
        return;
    }

    /* create a corresponding file in pmem by memory mapping it */
    void* pmem_addr;
    size_t mapped_len;
    int is_pmem;

    const bfs::path relpath = make_relative(root_dir, pathname);
    const bfs::path dst_abs_path = dax_fs_base / relpath;

    /* if the mapping file already exists delete it, since we can't trust that it's the same file */
    /* XXX we may need to change this in the future */
    if(bfs::exists(dst_abs_path)){
        if(unlink(dst_abs_path.c_str()) != 0){
            BOOST_LOG_TRIVIAL(error) << "Unable to remove file " << dst_abs_path << ": " << strerror(errno);
            return;
        }
    }

    if((pmem_addr = pmem_map_file(dst_abs_path.c_str(), stbuf.st_size, PMEM_FILE_CREATE | PMEM_FILE_EXCL,
                                  0666, &mapped_len, &is_pmem)) == NULL) {
        BOOST_LOG_TRIVIAL(error) << "Unable to create pmem file " << dst_abs_path << ": " << strerror(errno);
        return;
    }

    /* check that the range allocated is of the required size */
    if((off_t)mapped_len != stbuf.st_size){
        BOOST_LOG_TRIVIAL(error) << "NVRAM file mapping of different size than requested";
        pmem_unmap(pmem_addr, mapped_len);
        return;
    }

    ssize_t byte_count = 0;

    /* determine if the range allocated is true pmem and call the appropriate copy routine */
    if(is_pmem){
        byte_count = do_copy_to_pmem((char*) pmem_addr, fd);
    }
    else{
        byte_count = do_copy_to_non_pmem((char*) pmem_addr, fd);
    }

    /* the fd can be closed safely */
    if(close(fd) == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to close file descriptor for " << pathname << ": " << strerror(errno);
        return;
    }
    
    BOOST_LOG_TRIVIAL(debug) << "Prefetching finished: read " << byte_count << "/" << stbuf.st_size << " bytes";
    BOOST_LOG_TRIVIAL(debug) << "File preloaded to " << dst_abs_path;

    BOOST_LOG_TRIVIAL(debug) << "Inserting {" << pathname << ", " << pmem_addr << "}";

    entries.insert({
            pathname.c_str(), 
            std::move(chunk(pmem_addr, byte_count))
    });

}

/** lookup an entry */
bool NVRAM_cache::lookup(const char* pathname, void*& data_addr, size_t& size) const {

    (void) pathname;
    (void) data_addr;
    (void) size;

    auto it = entries.find(pathname);

    if(it == entries.end()){
        BOOST_LOG_TRIVIAL(debug) << "Prefetched data not found";
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "Prefetched data found at:" << it->second.data;

    data_addr = it->second.data;
    size = it->second.size;

    return true;
}

ssize_t NVRAM_cache::do_copy_to_pmem(char* addr, int fd){

    char* buf = (char*) malloc(block_size*sizeof(*buf));

    if(buf == NULL){
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate temporary buffer: " << strerror(errno);
    }

    ssize_t total = 0;
    ssize_t n;

    while((n = read(fd, buf, block_size)) != 0){

        if(n == -1){
            if(errno != EINTR){
                BOOST_LOG_TRIVIAL(error) << "Error while reading: " << strerror(errno);
                return n;
            }
            /* EINTR, repeat  */
            continue;
        }

        pmem_memcpy_nodrain(addr, buf, n);
        addr += n;
        total += n;
    }

    /* flush caches */
    pmem_drain();

    return total;
}

ssize_t NVRAM_cache::do_copy_to_non_pmem(char* addr, int fd){

    char* buf = (char*) malloc(block_size*sizeof(*buf));

    if(buf == NULL){
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate temporary buffer: " << strerror(errno);
    }

    ssize_t total = 0;
    ssize_t n;

    while((n = read(fd, buf, block_size)) != 0){

        if(n == -1){
            if(errno != EINTR){
                BOOST_LOG_TRIVIAL(error) << "Error while reading: " << strerror(errno);
                return n;
            }
            /* EINTR, repeat  */
            continue;
        }

        memcpy(addr, buf, n);
        addr += n;
        total += n;
    }

    return total;
}


} //namespace efsng
