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

#include <libpmem.h>

#include "fuse_buf_copy_pmem.h"
#include <nvram-nvml/dir.h>

//#define __PRINT_TREE__



namespace efsng {
namespace nvml {

/**********************************************************************************************************************/
/* class implementation                                                                                               */
/**********************************************************************************************************************/
dir::dir() 
    : backend::dir() {
}

dir::dir(const bfs::path& pathname, bool populate) 
    : m_pathname(pathname) {
}

dir::~dir() {
   
}

void dir::add_file (const std::string file) {
    if ( std::find(m_files.begin(), m_files.end(), file ) == m_files.end() )
        m_files.push_back( file );
}


void dir::list_files(std::list <std::string> & m_f) const {
    for (const auto li : m_files)
    {
        m_f.push_back(li);
    }
    
}

bool dir::find (const std::string fname,std::list < std::string >::iterator &it)
{
    // We should better do a map but for now...
    it =  std::find(m_files.begin(),m_files.end(),fname);
    return (it != m_files.end());
}

} // namespace nvml
} // namespace efsng
