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

 /*
* This software was developed as part of the
* EC H2020 funded project NEXTGenIO (Project ID: 671951)
* www.nextgenio.eu
*/ 


#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>
#include <unordered_map>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace efsng {
namespace config {

/*! Convenience alias for a list of key-value pairs */
using kv_list = std::unordered_map<std::string, std::string>;

/*! Class to pass around the backend options parsed from a config file */
struct backend_options {
    const std::string   m_id;               /*!< Backend ID */
    const std::string   m_type;             /*!< Backend type */
    const uint32_t      m_capacity;         /*!< Backend capacity */
    const bfs::path     m_root_dir;         /*!< Backend filesystem's root dir */
    const bfs::path     m_mount_dir;        /*!< Backend filesystem's mount dir */
    const bfs::path     m_results_dir;      /*!< Backend filesystem's results dir */
    kv_list             m_extra_options;    /*!< Extra options required by the backend */
};

/*! This class stores command-line arguments and options from the configuration file */
struct settings {

    /*! Maximum number of arguments that can be passed to FUSE */
    static const int s_max_fuse_args = 32;

    std::string                     m_exec_name;                    /*!< Program name */
    bool                            m_daemonize;                    /*!< Run echofs as a daemon? */
    bool                            m_debug;                        /*!< Run echofs in debug mode? */
    bfs::path                       m_root_dir;                     /*!< Path to underlying filesystem's root dir */
    bfs::path                       m_mount_dir;                    /*!< Path to echofs' mount point */
    bfs::path                       m_results_dir;                  /*!< Path to echofs' mount point */
    bfs::path                       m_config_file;                  /*!< Path to configuration file */
    bfs::path                       m_log_file;                     /*!< Path to log file (if any) */
    uint32_t                        m_workers;                      /*!< Number of workers in charge of importing/exporting resources */
    uint32_t                        m_transfer_size;                /*!< Transfer size */
    bfs::path                       m_api_sockfile;                 /*!< Path to socket for API communication */
    std::unordered_map<std::string, backend_options> m_backend_opts; /*!< User configuration options passed to any backends */
    std::list<kv_list>              m_resources;                    /*!< Resources that need to be imported/exported */
    int                             m_fuse_argc;                    /*!< FUSE argc */
    const char*                     m_fuse_argv[s_max_fuse_args];   /*!< FUSE argv */

    /*! Constructor */
    settings();

    /*! Copy constructor */
    settings(const settings& args);

    // we don't want anyone calling this by mistake.
    // N.B: if we ever need to implement this, we need to make a
    // deep copy of fuse_argv in order to prevent this and other
    // from sharing the same dynamically allocated memory 
    /*! Assignment operator (disabled) */
    settings& operator=(const settings& other) = delete;

    /*! Move-assignment operator (disabled) */
    settings& operator=(settings&& other);

    /*! Destructor */
    ~settings();

    /*! Reset setting and free all state associated to it */
    void reset(); 

    /*! Read configuration settings from command line */
    void from_cmdline(int argc, char* argv[]);

    /*! Read configuration settings from a YAML file */
    void load_from_yaml_file(const bfs::path& config_file);

}; // struct settings

} // namespace config
} // namespace efsng

#endif /* __SETTINGS_H__ */
