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

#ifndef __COMMAND_LINE_H__
#define __COMMAND_LINE_H__

#include <memory>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;


// Maximum number of arguments that are passed to FUSE
const int MAX_FUSE_ARGS = 32;

// Arguments stores the parsed command-line arguments
struct Arguments{

    std::string exec_name;
    bfs::path root_dir;
    bfs::path mount_point;

    Arguments() :
        fuse_argc(0),
        fuse_argv() { }

    int fuse_argc;
    const char* fuse_argv[MAX_FUSE_ARGS];
};

void usage(const char* name, bool is_error=false);
bool process_args(int argc, char* argv[], const std::shared_ptr<Arguments>& out);

#endif /* __COMMAND_LINE_H__ */
