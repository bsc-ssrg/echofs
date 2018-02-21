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

#ifndef __NVML_DIR_H__
#define __NVML_DIR_H__

#include <boost/thread/shared_mutex.hpp>

#include <efs-common.h>
#include "backend-base.h"
#include "file.h"
#include <fuse.h>
#include <set>

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {


    
/* descriptor for a directory structure in NVML */
struct dir : public backend::dir {

  
    dir();
    dir(const bfs::path& pathname, const ino_t inode, const bfs::path & path_original, dir::type type=dir::type::persistent, bool populate=true);
    void list_files(std::list <std::string> & m_files) const;
    ~dir();
    void add_file (const std::string fname);
    void remove_file (const std::string fname);
    bool find (const std::string fname, std::set < std::string >::iterator & it) const;
    unsigned int num_links () const;
    void stat(struct stat& stbuf) const;
    void save_attributes(struct stat & stbuf) override;    /* Saves attributes of the directory */

private:
    /* container with the list of files inside the directory. 
       We store the canonical name, 
       for the standard ls and a pointer to the file for the ls -ltrh optimization */
    bfs::path m_pathname;
    dir::type m_type;
    mutable std::set < std::string > m_files; // TODO : Mutex

    mutable boost::shared_mutex m_attributes_mutex;
    struct stat m_attributes; /*!< Dir attributes */

    
};

} // namespace nvml
} // namespace efsng


#endif /* __NVML_DIR_H__ */
