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
 *     # data-stores (mandatory)
 *     data-stores:
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
 *     preload-files:
 *     (
 *         "path/to/file1",
 *         "path/to/file2"
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

    /* parse 'data-stores' */
    try{
        const libconfig::Setting& cfg_data_stores = root["efs-ng"]["data-stores"];
        int count = cfg_data_stores.getLength();

        for(int i=0; i<count; ++i){
            const libconfig::Setting& cfg_data_store = cfg_data_stores[i];

            std::string type;
            cfg_data_store.lookupValue("type", type);

            if(type == "DRAM" || type == "NVRAM-NVML"){

                std::string size;

                /* find the size */
                if(!cfg_data_store.lookupValue("size", size)){
                    BOOST_LOG_TRIVIAL(error) << type << " data-store missing mandatory 'size' argument";
                    return false;
                }

                int64_t value = parse_size(size);

                if(value == -1){
                    BOOST_LOG_TRIVIAL(error) << "Unable to parse data-store size '" << size << "'";
                    return false;
                }
            }
            else{
                BOOST_LOG_TRIVIAL(error) << "Unsupported data-store type '" << type << "'"; 
                return false;
            }

            if(type == "NVRAM-NVML"){
                cfg_data_store.lookupValue("TYPE", type);
                BOOST_LOG_TRIVIAL(debug) << "TYPE  \"" << type << "\"";

                std::string dax_fs_path;

                /* find the base-path of the DAX filesystem */
                if(!cfg_data_store.lookupValue("dax-fs-path", dax_fs_path)){
                    BOOST_LOG_TRIVIAL(error) << "NVRAM-NVML data-store missing mandatory 'dax-fs-path' argument";
                    return false;
                }
            }
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
    }

    /* parse 'preload-files' */
    try{
        const libconfig::Setting& cfg_files_to_preload = root["efs-ng"]["preload-files"];
        int count = cfg_files_to_preload.getLength();

        BOOST_LOG_TRIVIAL(debug) << "cfg_files_to_preload.getLength() = " << count;

        for(int i=0; i<count; ++i){

            const std::string filename = cfg_files_to_preload[i];

            out->files_to_preload.insert(filename);

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

int64_t Configuration::parse_size(const std::string& str){

    const uint64_t B_FACTOR = 1;
    const uint64_t KB_FACTOR = 1e3;
    const uint64_t KiB_FACTOR = (1 << 10);
    const uint64_t MB_FACTOR = 1e6;
    const uint64_t MiB_FACTOR = (1 << 20);
    const uint64_t GB_FACTOR = 1e9;
    const uint64_t GiB_FACTOR = (1 << 30);

    std::string scopy = str;

    /* remove whitespaces from the string */
    scopy.erase(std::remove_if(scopy.begin(), scopy.end(), 
                                [](char ch){
                                    return std::isspace<char>(ch, std::locale::classic()); 
                                }), 
                scopy.end() );

    /* determine the units */
    std::size_t found;
    uint64_t factor;

    if((found = scopy.find("KB")) != std::string::npos){
        factor = KB_FACTOR;
    }
    else if((found = scopy.find("KiB")) != std::string::npos){
        factor = KiB_FACTOR;
    }
    else if((found = scopy.find("MB")) != std::string::npos){
        factor = MB_FACTOR;
    }
    else if((found = scopy.find("MiB")) != std::string::npos){
        factor = MiB_FACTOR;
    }
    else if((found = scopy.find("GB")) != std::string::npos){
        factor = GB_FACTOR;
    }
    else if((found = scopy.find("GiB")) != std::string::npos){
        factor = GiB_FACTOR;
    }
    else{
        if(!std::all_of(scopy.begin(), scopy.end(), ::isdigit)){
            return -1;
        }
        factor = B_FACTOR;

    }

    double value = std::stod(scopy.substr(0, found));

    return std::round(value*factor);

}

} // namespace efsng
