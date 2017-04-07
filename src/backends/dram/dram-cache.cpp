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

/* internal includes */
#include "../../logging.h"
#include "dram-cache.h"

namespace efsng {
namespace dram {

dram_backend::dram_backend(int64_t size)
    : efsng::Backend(size){
}

dram_backend::~dram_backend(){
}

uint64_t dram_backend::get_size() const {
    return max_size;
}

/** start the prefetch process of a file requested by the user */
void dram_backend::prefetch(const bfs::path& pathname){

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

    void* addr = (void*) malloc(stbuf.st_size);

    BOOST_LOG_TRIVIAL(debug) << "XXX Allocated buffer of 0x" << std::hex << stbuf.st_size << " bytes @{" << addr << "-" << (void*)((uint8_t*)addr + stbuf.st_size) << "}";
    BOOST_LOG_TRIVIAL(debug) << "XXX Allocated buffer of " << std::dec << stbuf.st_size << " bytes @{" << (uint64_t)addr << "-" << (uint64_t) ((char*)addr + stbuf.st_size) << "}";

    if(addr == NULL){
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate buffer for prefetched data";
        return;
    }

    bool eof = false;
    ssize_t byte_count = 0; 
    size_t total = stbuf.st_size;

    do{
        ssize_t rv = read(fd, (uint8_t*)addr + byte_count, total - byte_count);

        if(rv == -1){
            if(errno == EINTR){
                /* rerun the iteration */
                continue;
            }
            BOOST_LOG_TRIVIAL(error) << "read() error: " << strerror(errno);
            break;
        }

        if(rv != 0){
            byte_count += rv;
            continue;
        }

        eof = true;
    }while(!eof);

    BOOST_LOG_TRIVIAL(debug) << "Prefetching finished: read " << byte_count << "/" << total << " bytes";

    BOOST_LOG_TRIVIAL(debug) << "Inserting {" << pathname << ", " << addr << "}";

    entries.insert({
            pathname.c_str(), 
            std::move(chunk(addr, byte_count))
    });

    /* the fd can be closed safely */
    if(close(fd) == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to close file descriptor for " << pathname << ": " << strerror(errno);
        return;
    }
}

bool dram_backend::exists(const char* pathname) const {
    return entries.find(pathname) != entries.end();
}

efsng::Backend::iterator dram_backend::find(const char* path) {
    //return entries.find(path);
}

efsng::Backend::iterator dram_backend::begin() {
    //return entries.begin();
}

efsng::Backend::iterator dram_backend::end() {
    //return entries.end();
}

efsng::Backend::const_iterator dram_backend::cbegin() {
    //return entries.cbegin();
}

efsng::Backend::const_iterator dram_backend::cend() {
    //return entries.cend();
}

/** lookup an entry */
bool dram_backend::lookup(const char* pathname, void*& data_addr, size_t& size) const {

    auto it = entries.find(pathname);

    if(it == entries.end()){
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


} // namespace dram
} //namespace efsng
