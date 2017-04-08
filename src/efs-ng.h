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
#ifndef __EFS_NG_H__
#define __EFS_NG_H__

#include <memory>
#include <unordered_map>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

namespace efsng{

/* forward declarations */
struct Arguments;
struct File;

/* internal state of the filesystem */
struct Efsng {

    // void add_open_file(const bfs::path& filename){

    //     auto record = std::unique_ptr<File>(new File(-1,-1,-1));
    //     open_files.insert(std::make_pair(
    //                         filename.c_str(), 
    //                         std::move(record)));
    // }

    /** configuration options passed by the user */
    Arguments* user_args;

    /** */
    //std::unordered_map<const char*, std::unique_ptr<File>> open_files;
    /** */
    //std::unordered_map<const char*, void*> ram_cache; // encapsulate
    //std::unordered_map<std::string, void*> ram_cache; // encapsulate
    //

    backend* backends[backend::TOTAL_COUNT];

}; // struct Efsng

} // namespace efsng

#endif /* __EFS_NG_H__ */
