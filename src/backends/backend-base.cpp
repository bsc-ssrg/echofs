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

namespace bfs = boost::filesystem;

#include "backend-base.h"
#include "utils.h"
#include "dram/dram.h"
#include "nvram-nvml/nvram-nvml.h"

namespace efsng {

backend::backend_ptr backend::create_from_options(const config::backend_options& opts) {

    const std::string id = opts.m_id;
    const std::string type = opts.m_type;

    if(type == "DRAM") {
        return std::make_unique<dram::dram_backend>(opts.m_capacity);
    }
    else if(type == "NVRAM-NVML") {

        if(opts.m_extra_options.count("daxfs") == 0) {
            throw std::runtime_error("Mandatory option 'daxfs' missing in definition of backend '" + id + "'");
        }

        const bfs::path& daxfs = opts.m_extra_options.at("daxfs");

        int64_t ssize = -1;

        if(opts.m_extra_options.count("segment-size") != 0) {
            try {
                ssize = parse_size(opts.m_extra_options.at("segment-size"));
            }
            catch(const std::exception& e) {
                throw std::runtime_error("Invalid argument in option 'segment-size' of backend '" + id + "'");
            }
        }

        return std::make_unique<nvml::nvml_backend>(opts.m_capacity, daxfs, opts.m_root_dir, ssize);
    }

    return std::unique_ptr<backend>(nullptr);
}

backend::Type backend::name_to_type(const std::string& name) {

    if(name == "DRAM"){
        return DRAM;
    }

    if(name == "NVRAM-NVML"){
        return NVRAM_NVML;
    }

    return UNKNOWN;
}

int64_t backend::parse_size(const std::string& str) {

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

    if((found = scopy.find("KB")) != std::string::npos) {
        factor = KB_FACTOR;
    }
    else if((found = scopy.find("KiB")) != std::string::npos) {
        factor = KiB_FACTOR;
    }
    else if((found = scopy.find("MB")) != std::string::npos) {
        factor = MB_FACTOR;
    }
    else if((found = scopy.find("MiB")) != std::string::npos) {
        factor = MiB_FACTOR;
    }
    else if((found = scopy.find("GB")) != std::string::npos) {
        factor = GB_FACTOR;
    }
    else if((found = scopy.find("GiB")) != std::string::npos) {
        factor = GiB_FACTOR;
    }
    else {
        if(!std::all_of(scopy.begin(), scopy.end(), ::isdigit)) {
            return -1;
        }
        factor = B_FACTOR;

    }

    double value = std::stod(scopy.substr(0, found));

    return std::round(value*factor);

}

} // namespace efsng

#ifdef __EFS_DEBUG__
std::ostream& operator<<(std::ostream& os, const efsng::backend::buffer& buf) {

    os << "{ "
       << "m_data_ptr: " << buf.m_data << ", "
       << "m_size: 0x" << std::hex << buf.m_size
       << "}";

    return os;
}

std::ostream& operator<<(std::ostream& os, const efsng::backend::buffer_map& bmap) {

    os << "buffer_map {\n";

    for(const auto& buf: bmap){
        os << buf;
    }

    os << "};\n";

    return os;
}
#endif /* __EFS_DEBUG__ */
