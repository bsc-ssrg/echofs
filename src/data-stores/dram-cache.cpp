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

/* internal includes */
#include "../logging.h"
#include "dram-cache.h"

namespace efsng {

/** lookup an entry */
bool DRAM_cache::lookup(const char* pathname, void*& data_addr) const {

    auto it = entries.find(pathname);

    if(it == entries.end()){
        BOOST_LOG_TRIVIAL(debug) << "Prefetched data not found";
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "Prefetched data found at:" << it->second;

    data_addr = it->second;

    return true;
}

/** start the prefetch process of a file requested by the user */
void DRAM_cache::prefetch(const bfs::path& pathname){

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

    /* XXX in this first version, we are prefetching the whole file as is */
    void* addr = mmap(NULL, stbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if(addr == MAP_FAILED){
        BOOST_LOG_TRIVIAL(error) << "Unable to preload the file " << pathname << ": " << strerror(errno);
        return;
    }

#if 0 /* read()-based full file prefetching */
    void* addr = (void*) malloc(stbuf.st_size);

    assert(addr);

    ssize_t nread = 0;
    ssize_t res = 0;

    while(1){
        //res = read(fd, addr+nread, stbuf.st_size-nread);
        res = read(fd, addr, stbuf.st_size/2);

        if(res <= 0)
            break;

        nread += res;
    }*/
#endif

    BOOST_LOG_TRIVIAL(debug) << "Inserting {" << pathname << ", " << addr << "}";

    entries.insert({pathname.c_str(), addr});

    /* the fd can be closed safely */
    if(close(fd) == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to close file descriptor for " << pathname << ": " << strerror(errno);
        return;
    }
    
    //buf_addr = addr;
}

} //namespace efsng
