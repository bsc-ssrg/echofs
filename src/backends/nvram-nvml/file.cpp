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

#include "file.h"
#include <iostream>

namespace efsng {
namespace nvml {


file::file() 
    : Backend::file() {
}

file::file(mapping& mp) 
    : efsng::Backend::file() {
        (void) mp;

    std::cerr << mp << "\n";


//    m_mappings.push_back(std::move(mp));
    m_mappings.emplace_back(std::move(mp));

    std::cerr << m_mappings.back() << "\n";


std::cerr << "file(mapping& mp) end\n";

}


void file::add(const mapping& mp) {
        (void) mp;
//    m_mappings.push_back(std::move(mp));
}

void file::add_m(const bfs::path& prefix, const bfs::path& base_path, size_t min_size) {
    m_mappings.emplace_back(prefix, base_path, min_size);
}

} // namespace nvml
} // namespace efsng



