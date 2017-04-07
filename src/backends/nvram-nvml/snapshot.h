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

#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__

#include "mapping.h"

namespace efsng {
namespace nvml {

/* frozen representation of a mapping state in time 
 * does not claim ownership of the resources, which still 
 * belong to the mapping */
struct snapshot {
    data_ptr_t                  m_data;     /* mapped data */
    off_t                       m_offset;   /* base offset within file */
    size_t                      m_size;     /* mapped size */
    size_t                      m_bytes;    /* used size */
    std::list<data_copy>        m_copies;   /* block copies */

    snapshot(const mapping& mapping);
};


} // namespace nvml
} // namespace efsng

#endif /* __SNAPSHOT_H__ */
