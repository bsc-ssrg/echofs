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

#ifndef __POSIX_FILE_H__
#define __POSIX_FILE_H__

#include <fcntl.h>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace efsng {
namespace posix {

/* class to encapsulate a standard Unix file descriptor so that we can rely on RAII */
struct file {
    int       m_fd;       /* file descriptor */
    bfs::path m_pathname; /* pathname */

    file(const bfs::path& pathname, int flags=O_RDONLY);
    ~file();

    size_t get_size() const;
    void close();
    int stat(struct stat& stbuf);
};

} // namespace posix
} // namespace efsng

#endif /* __POSIX_FILE_H__ */
