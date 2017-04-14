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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>
#include <map>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace efsng {

using kv_list = std::list<std::pair<std::string, std::string>>;

/* settings stores command-line arguments and options from the configuration file */
struct settings {

    static const int s_max_fuse_args = 32;

    std::string                         m_exec_name;
    bool                                m_daemonize;
    bool                                m_debug;
    bfs::path                           m_root_dir;
    bfs::path                           m_mount_point;
    bfs::path                           m_config_file;
    bfs::path                           m_log_file;
    std::map<std::string, kv_list>      m_backend_opts;
    std::map<bfs::path, std::string>    m_files_to_preload;
    int                                 m_fuse_argc;
    const char*                         m_fuse_argv[s_max_fuse_args];

    settings();
    settings(const settings& args);

    // we don't want anyone calling this by mistake.
    // N.B: if we ever need to implement this, we need to make a
    // deep copy of fuse_argv in order to prevent this and other
    // from sharing the same dynamically allocated memory 
    settings& operator=(const settings& other) = delete;
    settings& operator=(settings&& other);
    ~settings();
    void reset(); 
    void from_cmdline(int argc, char* argv[]);
    void from_file(const bfs::path& config_file);


}; // struct settings


} // namespace efsng

#endif /* __SETTINGS_H__ */
