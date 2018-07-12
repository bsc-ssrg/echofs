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


#include "file-options.h"
#include "parsers.h"

namespace efsng {
namespace config {

// define some constant keywords so that we can refer to them throughout
// the parsing code without having to rely on the actual string literal
// (this will allow us to rename options in the configuration file
// without having to modify all the statements that refer to them)
namespace keywords {

// section names
static const std::string global_settings("global-settings");
static const std::string backends("backends");
static const std::string resources("resources");

// option names for 'global-settings' section
static const std::string root_dir("root-dir");
static const std::string mount_dir("mount-dir");
static const std::string results_dir("results-dir");
static const std::string log_file("log-file");
static const std::string workers("workers");
static const std::string transfer_size("transfer-size");

// option names for 'backends' section
static const std::string id("id");
static const std::string type("type");
static const std::string capacity("capacity");

// option names for 'resources' section
static const std::string path("path");
static const std::string backend("backend");
static const std::string flags("flags");

}

using file_options::file_schema;
using file_options::declare_option;
using file_options::declare_list;
using file_options::declare_group;
using file_options::declare_file;

// define the configuration file structure and declare the supported options
const file_schema config_schema = declare_file({
    declare_section(
        keywords::global_settings, 
        true,
        declare_group({   
            declare_option<bfs::path>(keywords::root_dir,      true,            path_parser), 
            declare_option<bfs::path>(keywords::mount_dir,     true,            path_parser),
            declare_option<bfs::path>(keywords::results_dir,   false,           path_parser),
            declare_option<bfs::path>(keywords::log_file,      false,           path_parser),
            declare_option<uint32_t> (keywords::workers,       false, 8,        number_parser),
            declare_option<uint32_t> (keywords::transfer_size, false, 128*1024, size_parser)
        })
    ),
    declare_section(
        keywords::backends, 
        true,
        declare_list({
            declare_option<std::string>(keywords::id,            true),
            declare_option<std::string>(keywords::type,          true),
            declare_option<uint32_t>   (keywords::capacity,      true, size_parser),
            declare_option<std::string>(file_options::match_any, false)
        })
    ),
    declare_section(
        keywords::resources, 
        true,
        declare_list({
            declare_option<std::string>(keywords::path,    true),
            declare_option<std::string>(keywords::backend, true),
            declare_option<std::string>(keywords::flags,   true),
        })
    ),
});

} // namespace config
} // namespace efsng
