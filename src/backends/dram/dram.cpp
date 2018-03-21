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

/* C includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

/* internal includes */
#include <logger.h>
#include <utils.h>
#include <posix-file.h>
#include <dram/file.h>
#include <dram/mapping.h>
#include <dram/snapshot.h>
#include <dram/dram.h>

namespace efsng {
namespace dram {

dram_backend::dram_backend(uint64_t capacity)
    : m_capacity(capacity) {
}

dram_backend::~dram_backend(){
}

std::string dram_backend::name() const {
    return s_name;
}

uint64_t dram_backend::capacity() const {
    return m_capacity;
}

/** start the load process of a file requested by the user */
error_code dram_backend::load(const bfs::path& pathname, backend::file::type type){
    (void) pathname;
    (void) type;
    LOGGER_DEBUG("Loading file \"{}\" in RAM", pathname.string());

    LOGGER_ERROR("Not implemented yet!");

    return error_code::success;
}

/** start the load process of a file requested by the user */
error_code dram_backend::unload(const bfs::path& pathname, const bfs::path & mntpathname){
    (void)pathname;
    (void)mntpathname;
    LOGGER_ERROR("Unload not implemented!");
    return error_code::success;
}

bool dram_backend::exists(const char* pathname) const {
    
    std::lock_guard<std::mutex> lock(m_files_mutex);

    const auto& it = m_files.find(pathname);

    // XXX it is safe to unlock the mutex here BECAUSE WE KNOW
    // that no other threads are going to remove a mapping from
    // the list; this happens because mappings are ONLY destroyed
    // when the std::list destructor is called (and we make sure that
    // this is done in a thread-safe manner). If this assumption ever
    // changes, we may NEED to revise this.
    return it != m_files.end();
}

int dram_backend::do_readdir (const char * path, void * buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) const {
    (void) path;
    (void) buffer;
    (void) filler;
    (void) offset;
    (void) fi;
    LOGGER_ERROR("do_readdir not implemented");
    return 0;
}

int dram_backend::do_stat (const char *path, struct stat& stbuf) const {
    (void) path;
    (void) stbuf;
    LOGGER_ERROR("do_stat not implemented");
    return 0;
}

int dram_backend::do_create(const char* pathname, mode_t mode, std::shared_ptr < backend::file> & file) {
    (void) pathname;
    (void) mode;
    (void) file;
    LOGGER_ERROR("do_create not implemented");
    return 0;
}

int dram_backend::do_unlink(const char * pathname) {
    (void) pathname;
    return 0;
}

int dram_backend::do_rename(const char * oldpath, const char * newpath) {
    (void) oldpath;
    (void) newpath;
    return 0;
}

int dram_backend::do_mkdir(const char * pathname, mode_t mode) {
    (void) pathname;
    (void) mode;
    return 0;
}

int dram_backend::do_rmdir(const char * pathname) {
    (void) pathname;
    return 0;
}

int dram_backend::do_chmod(const char * pathname, mode_t mode) {   
    (void) pathname;
    (void) mode;
    return 0;
}

int dram_backend::do_chown(const char * pathname, uid_t owner, gid_t group) {
    (void) pathname;
    (void) owner;
    (void) group;
    return 0;
}

void dram_backend::do_change_type(const char * pathname, backend::file::type type) { 
    (void) pathname;
    (void) type;
}


efsng::backend::iterator dram_backend::find(const char* path) {
    return m_files.find(path);
}

efsng::backend::iterator dram_backend::begin() {
    return m_files.begin();
}

efsng::backend::iterator dram_backend::end() {
    return m_files.end();
}

efsng::backend::const_iterator dram_backend::cbegin() {
    return m_files.cbegin();
}

efsng::backend::const_iterator dram_backend::cend() {
    return m_files.cend();
}



} // namespace dram
} //namespace efsng
