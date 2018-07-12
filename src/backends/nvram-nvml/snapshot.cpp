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


#include <logging.h>
#include <nvram-nvml/file.h>
#include <nvram-nvml/mapping.h>
#include <nvram-nvml/snapshot.h>

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

/* make sure that this copy is protected by a lock! */
snapshot::snapshot(const mapping& mapping) 
    : m_data(mapping.m_data),
      m_offset(mapping.m_offset),
      m_size(mapping.m_size),
      m_bytes(mapping.m_bytes),
      m_copies(mapping.m_copies) {
}

} // namespace nvml
} // namespace efsng

#ifdef __EFS_DEBUG__
std::ostream& operator<<(std::ostream& os, const efsng::nvml::snapshot& snap) {
    os << "{ "
       << "m_data = " << snap.m_data << ", "
       << "m_offset = 0x" << std::hex << snap.m_offset << ", "
       << "m_size = 0x" << std::hex << snap.m_size << ", "
       << "m_bytes = 0x" << std::hex << snap.m_size
       << "}";

    return os;
}

#endif /* __EFS_DEBUG__ */

