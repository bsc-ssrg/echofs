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

#ifndef __FILE_H__
#define __FILE_H__

#include <efs-common.h>
#include <nvram-nvml/mapping.h>
#include <backend.h>

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

/* descriptor for a file loaded onto NVML */
struct file : public backend::file {
    /* TODO skip lists might be a good choice here.
     * e.g. see: https://github.com/khizmax/libcds 
     *      for lock-free skip lists */
    std::list<mapping>   m_mappings; /* list of mappings associated to the file */

    file();
    file(mapping& mp);

    ~file(){
        std::cerr << "a nvml::file instance died...\n";
    }

    void add(const mapping& mp);
};

} // namespace nvml
} // namespace efsng


#endif /* __FILE_H__ */
