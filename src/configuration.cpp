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

#include <fstream>
#include <string>
#include <cstddef>
#include <algorithm>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include <libconfig.h++>

#include "command-line.h"
#include "configuration.h"

/** 
 * Sample configuration file for reference:
 *
 * efs-ng:
 * {
 *     # root directory to mirror (mandatory)
 *     root-dir    =   "path/to/a_root/";
 *
 *     # mount point of efs-ng (mandatory)
 *     mount-point =   "path/to/a_mnt/";
 *
 *     # backends (mandatory)
 *     backend-stores:
 *     (
 *         { 
 *             type = "DRAM";
 *             size = "1GiB";
*          },
*          {
*              type = "NVRAM-NVML";
*              base-path = "/mnt/pmem/";
*              size = "2GiB";
*          }
 *     );
 *
 *     # logfile (optional)
 *     log-file    = "path/to/a_logfile";
 *
 * 
 *     # files to prefetch (optional)
 *     preload:
 *     (
 *         {
 *             path = "path/to/file1";
 *             backend = "DRAM;"
 *         },
 *         {
 *             path = "path/to/file2";
 *             backend = "NVRAM-NVML";
 *         }
 *     );
 * };
 */

namespace bfs = boost::filesystem;

namespace efsng{

bool Configuration::load(const bfs::path& config_file, Arguments* out){

    Arguments out_backup(*out);

    libconfig::Config cfg;

    try{
        cfg.readFile(config_file.c_str());
    }
    catch(const libconfig::FileIOException& fioex){
        BOOST_LOG_TRIVIAL(warning) << "An error happened while reading the configuration file.";
        return false;
    }
    catch(const libconfig::ParseException& pex){
        BOOST_LOG_TRIVIAL(error) << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " 
                                 << pex.getError();
        BOOST_LOG_TRIVIAL(error) << "The configuration file will be ignored";
        return false;
    }

    const libconfig::Setting& root = cfg.getRoot();

    /* parse 'root-dir' */
    try{
        const libconfig::Setting& cfg_root_dir = root["efs-ng"]["root-dir"];
        std::string optval = cfg_root_dir;

        /* command-line arguments override the options passed in the configuration file. 
         * Thus, if 'root_dir' already has a value different from the default one, ignore the passed cfg_value */
        if(out->root_dir == "none"){
            out->root_dir = std::string(optval);

            BOOST_LOG_TRIVIAL(debug) << " * root-dir = " << out->root_dir;
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        /* ignore */
    }

    /* parse 'mount-point' */
    try{
        const libconfig::Setting& cfg_mount_point = root["efs-ng"]["mount-point"];
        std::string optval = cfg_mount_point;

        /* command-line arguments override the options passed in the configuration file. 
         * Thus, if 'root_dir' already has a value different from the default one, ignore the passed cfg_value */
        if(out->mount_point == "none"){
            out->mount_point = std::string(optval);

            BOOST_LOG_TRIVIAL(debug) << " * mount-point = " << optval;
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        /* ignore */
    }

    /* parse 'log-file' */
    try{
        const libconfig::Setting& cfg_logfile = root["efs-ng"]["log-file"];
        std::string optval = cfg_logfile;

        /* command-line arguments override the options passed in the configuration file. 
         * Thus, if 'log_file' already has a value different from the default one, ignore the passed cfg_value */
        if(out->log_file == "none"){
            out->log_file = std::string(optval);

            BOOST_LOG_TRIVIAL(debug) << " * log-file: " << optval;
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        /* ignore */
    }

    /* parse 'backend-stores' */
    try{
        const libconfig::Setting& cfg_backend_stores = root["efs-ng"]["backend-stores"];
        int count = cfg_backend_stores.getLength();

        for(int i=0; i<count; ++i){

            const libconfig::Setting& cfg_backend_store = cfg_backend_stores[i];
            std::string bend_type;
            kv_list bend_opts;

            for(int j=0; j<cfg_backend_store.getLength(); ++j){
                const libconfig::Setting& opt = cfg_backend_store[j];

                std::string opt_name = opt.getName();
                std::string opt_value = opt;

                if(opt_name == "type"){
                    bend_type = opt_value;
                }

                bend_opts.push_back({opt_name, opt_value});
            }

            if(bend_type != ""){
                out->backend_opts.insert({bend_type, bend_opts});
            }
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
    }

    /* parse 'preload-files' */
    try{
        const libconfig::Setting& cfg_files_to_preload = root["efs-ng"]["preload"];
        int count = cfg_files_to_preload.getLength();

        BOOST_LOG_TRIVIAL(debug) << "cfg_files_to_preload.getLength() = " << count;

        // for(int i=0; i<count; ++i){

        //     const std::string filename = cfg_files_to_preload[i];

        //     out->files_to_preload.insert(filename);

        //     BOOST_LOG_TRIVIAL(debug) << "  \"" << filename << "\"";
        // }

        for(int i=0; i<cfg_files_to_preload.getLength(); ++i){

            const libconfig::Setting& filedef = cfg_files_to_preload[i];

            bfs::path filename;
            std::string backend;

            for(int j=0; j<filedef.getLength(); ++j){

                const libconfig::Setting& opt = filedef[j];

                std::string opt_name = opt.getName();
                std::string opt_value = opt;

                if(opt_name == "path"){
                    filename = opt_value;
                }
                else if(opt_name == "backend"){
                    backend = opt_value;
                }
                else{
                    BOOST_LOG_TRIVIAL(error) << "Unsupported parameter in configuration file '" << opt_name << "'";
                    return false;
                }
            }

            out->files_to_preload.insert({filename, backend});

            BOOST_LOG_TRIVIAL(debug) << "  \"" << filename << "\"";
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        /* ignore */
        //BOOST_LOG_TRIVIAL(warning) << "'preload-files' setting not found.";
        //return false;
    }

    return true;
}


} // namespace efsng
