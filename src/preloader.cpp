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
 * Mercurium C/C++ source-to-source compiler is distributed in the hope  *
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the    *
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Mercurium C/C++ source-to-source compiler; if      *
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,       *
 * Cambridge, MA 02139, USA.                                             *
 *************************************************************************/

/* C includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

/* C++ includes */
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

/* project includes */
#include "preloader.h"

namespace bfs = boost::filesystem;

namespace efsng{

/* manage the preload of the requested files */
void Preloader::preload_file(const bfs::path& filename){
    BOOST_LOG_TRIVIAL(debug) << "Preloading file " << filename;

    /* open the file */
    int fd = open(filename.c_str(), O_RDONLY);

    if(fd == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to preload file " << filename << ": " << strerror(errno);
        return;
    }

    /* determine the size of the file */
    struct stat stbuf;

    if(fstat(fd, &stbuf) == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to determine the size of file " << filename << ": " << strerror(errno);
        return;
    }

    /* XXX in this first version, we are preloading the whole file as is */
    void* addr = mmap(NULL, stbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if(addr == MAP_FAILED){
        BOOST_LOG_TRIVIAL(error) << "Unable to preload the file " << filename << ": " << strerror(errno);
        return;
    }

    /* we can safely close the fd */
    if(close(fd) == -1){
        BOOST_LOG_TRIVIAL(error) << "Unable to close file descriptor for " << filename << ": " << strerror(errno);
        return;
    }
}

} // namespace efsng
