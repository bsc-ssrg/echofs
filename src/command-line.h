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

#ifndef __COMMAND_LINE_H__
#define __COMMAND_LINE_H__

#include <string>
#include <set>
#include <map>
#include <memory>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace efsng{

/* Maximum number of arguments that are passed to FUSE */
const int MAX_FUSE_ARGS = 32;

typedef std::list<std::pair<std::string, std::string>> kv_list;

/* Arguments stores command-line arguments and options from the configuration file */
struct Arguments{

    std::string                         exec_name;
    bfs::path                           root_dir;
    bfs::path                           mount_point;
    bfs::path                           config_file;
    bfs::path                           log_file;
    std::map<std::string, kv_list>      backend_opts;
    std::map<bfs::path, std::string>    files_to_preload;
    int                                 fuse_argc;
    const char*                         fuse_argv[MAX_FUSE_ARGS];

    Arguments() :
        exec_name("none"),
        root_dir("none"),
        mount_point("none"),
        config_file("none"),
        log_file("none"),
        fuse_argc(0),
        fuse_argv() { }

    Arguments(const Arguments& args) :
        exec_name(args.exec_name),
        root_dir(args.root_dir),
        mount_point(args.mount_point),
        config_file(args.config_file),
        log_file(args.log_file),
        files_to_preload(args.files_to_preload),
        fuse_argc(args.fuse_argc){

        for(int i=0; i<MAX_FUSE_ARGS; ++i){
            this->fuse_argv[i] = args.fuse_argv[i];
        }
    }


}; // struct Arguments

void usage(const char* name, bool is_error=false);
bool process_args(int argc, char* argv[], Arguments* out);

} //namespace efsng

#endif /* __COMMAND_LINE_H__ */
