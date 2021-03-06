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


#ifndef __FILE_H__
#define __FILE_H__

#include <iostream>
#include <efs-common.h>
#include <dram/mapping.h>
#include "backend-base.h"
#include <fuse.h>

namespace bfs = boost::filesystem;

namespace efsng {
namespace dram {

/* descriptor for a file loaded in DRAM */
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

    void stat(struct stat& buf) const override { 
        (void) buf; 
        return ; 
    }
    
    void save_attributes(struct stat & stbuf) override {
        (void) stbuf;
        return ;
    }

    ssize_t get_data(off_t offset, size_t size, struct fuse_bufvec* fuse_buffer) override { 
        (void) offset;
        (void) size;
        (void) fuse_buffer;
        return 0; 
    }

    ssize_t put_data(off_t offset, size_t size, struct fuse_bufvec* fuse_buffer) override { 
        (void) offset;
        (void) size;
        (void) fuse_buffer;
        return 0; 
    }

    ssize_t append_data(off_t offset, size_t size, struct fuse_bufvec* fuse_buffer) override { 
        (void) offset;
        (void) size;
        (void) fuse_buffer;
        return 0; 
    }

    size_t size() const { return 0; }
    void set_size(size_t size) { (void) size; }
    void extend(off_t offset, size_t size) { (void) offset; (void) size; }
    
    void change_type (file::type type);
    void add(const mapping& mp);
    int unload(const std::string path);
};

} // namespace dram
} // namespace efsng

#endif /* __FILE_H__ */
