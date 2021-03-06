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


#include <libpmem.h>

#include "fuse_buf_copy_pmem.h"
#include <nvram-devdax/dir.h>

//#define __PRINT_TREE__



namespace efsng {
namespace nvml_dev {

/**********************************************************************************************************************/
/* class implementation                                                                                               */
/**********************************************************************************************************************/
dir::dir() 
    : backend::dir() {
}

dir::dir(const bfs::path& pathname, const ino_t inode, const bfs::path & path_original, dir::type type, bool populate) 
    : m_pathname(pathname),
     m_type(type) {

    if (populate) {
        posix::file fd(path_original);
        struct stat stbuf;
        fd.stat(stbuf);
        stbuf.st_ino = inode;
        stbuf.st_nlink = 1;
        save_attributes(stbuf);
    } else  {
        struct stat stbuf;
        memset(&stbuf,0,sizeof(struct stat));
       // stbuf.st_dev = 48;
       // stbuf.st_size = 4096;
       // stbuf.st_blocks=8;
        stbuf.st_ino = inode;
        stbuf.st_nlink = 1;
        save_attributes(stbuf);
    }

}

dir::~dir() {
   
}

void dir::add_file (const std::string file) {
        m_files.insert( file );
        m_attributes_mutex.lock();
        m_attributes.st_nlink += 1;
        m_attributes.st_mtime = m_attributes.st_ctime = time (NULL);
        m_attributes_mutex.unlock();
}


void dir::remove_file (const std::string file) {
  //  auto it = std::find(m_files.begin(), m_files.end(), file );
  //  if (it != m_files.end()) {
        m_files.erase ( file );
        m_attributes_mutex.lock();
        m_attributes.st_nlink -= 1;
        m_attributes.st_mtime = m_attributes.st_ctime = time (NULL);
        m_attributes_mutex.unlock();
   // }
}

void dir::list_files(std::list <std::string> & m_f) const {
    for (const auto li : m_files){
        m_f.push_back(li);
    }
}

bool dir::find (const std::string fname,std::unordered_set < std::string >::iterator &it) const {
    // We should better do a map but for now...

    it =  m_files.find(fname);
    return (it != m_files.end());
}

unsigned int dir::num_links() const {
    return m_files.size();
}


void dir::stat(struct stat& stbuf) const {
    m_attributes_mutex.lock_shared();
    //LOGGER_DEBUG(" STAT NVML DIR {}",m_pathname);
    memcpy(&stbuf, &m_attributes, sizeof(stbuf));
    m_attributes_mutex.unlock_shared();
}

void dir::save_attributes(struct stat& stbuf) {
    m_attributes_mutex.lock();
    memcpy(&m_attributes, &stbuf, sizeof(m_attributes));
    m_attributes_mutex.unlock();
}

} // namespace nvml
} // namespace efsng
