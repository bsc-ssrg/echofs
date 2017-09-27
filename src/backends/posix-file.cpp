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

#include <sys/types.h>
#include <sys/stat.h>

#include <logging.h>
#include <posix-file.h>

namespace efsng {
namespace posix {

file::file(const bfs::path& pathname, int flags)
    : m_fd(-1),
      m_pathname(pathname){

    m_fd = ::open(pathname.c_str(), flags);

    if(m_fd == -1){
        //BOOST_LOG_TRIVIAL(error) << "Error loading file " << m_pathname << ": " << strerror(errno);
        throw std::runtime_error("");
    }
}

file::~file(){
    if(m_fd != -1 && ::close(m_fd) == -1){
        //BOOST_LOG_TRIVIAL(error) << "Error closing file " << m_pathname << ": " << strerror(errno);
    }
}

size_t file::get_size() const {

    struct stat stbuf;

    if(fstat(m_fd, &stbuf) == -1){
        //BOOST_LOG_TRIVIAL(error) << "Error finding size of file " << m_pathname << ": " << strerror(errno);
        return 0;
    }

    return stbuf.st_size;
}

int file::stat(struct stat& stbuf) {
    return fstat(m_fd, &stbuf);
}

void file::close() {
    if(m_fd != -1 && ::close(m_fd) == -1){
        //BOOST_LOG_TRIVIAL(error) << "Error closing file " << m_pathname << ": " << strerror(errno);
        return;
    }

    m_fd = -1;
}

} // namespace posix
} // namespace efsng
