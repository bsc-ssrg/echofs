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

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

namespace bfs = boost::filesystem;

#include "backend.h"
#include "dram/dram-cache.h"
#include "nvram-nvml/nvram-nvml.h"

namespace efsng {

Backend* Backend::backend_factory(const std::string& type, const kv_list& backend_opts){

    if(type == "DRAM"){

        int64_t bend_size = -1;

        /* parse the options provided by the user */
        for(const auto& kv : backend_opts){

            if(kv.first == "size"){
                bend_size = parse_size(kv.second);

                if(bend_size == -1){
                    BOOST_LOG_TRIVIAL(error) << "Unable to parse backend-store size '" << kv.second << "'";
                    return nullptr;
                }
            }
        }

        if(bend_size == -1){
            BOOST_LOG_TRIVIAL(error) << "Mandatory arguments missing for " << type << " backend"; 
        }

        return new DRAM_cache(bend_size);

    }
    else if(type == "NVRAM-NVML"){

        int64_t bend_size = -1;
        bfs::path dax_fs_path;
        bfs::path root_dir;

        /* parse the options provided by the user */
        for(const auto& kv : backend_opts){

            if(kv.first == "size"){
                bend_size = parse_size(kv.second);

                if(bend_size == -1){
                    BOOST_LOG_TRIVIAL(error) << "Unable to parse backend-store size '" << kv.second << "'";
                    return nullptr;
                }
            }
            else if(kv.first == "dax-fs-path"){
                dax_fs_path = kv.second;
            }
            else if(kv.first == "root-dir"){
                root_dir = kv.second;
            }
        }

        if(bend_size == -1 || dax_fs_path == ""){
            BOOST_LOG_TRIVIAL(error) << "Mandatory arguments missing for " << type << " backend"; 
        }

        return new NVML_backend(bend_size, dax_fs_path, root_dir);

    }

    return nullptr;
}

Backend::Type Backend::name_to_type(const std::string& name){

    if(name == "DRAM"){
        return DRAM;
    }

    if(name == "NVRAM-NVML"){
        return NVRAM_NVML;
    }

    return UNKNOWN;
}

int64_t Backend::parse_size(const std::string& str){

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
