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

 /*
* This software was developed as part of the
* EC H2020 funded project NEXTGenIO (Project ID: 671951)
* www.nextgenio.eu
*/ 


#ifndef __NVML_BACKEND_H__
#define  __NVML_BACKEND_H__

/* C++ includes */
#include <string>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include <mutex>
#include <atomic>

/* internal includes */
#include <efs-common.h>
#include "backend-base.h"
#include "errors.h"

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

/* class to manage file allocations in NVRAM based on the NVML library */
class nvml_backend : public efsng::backend {

    // some aliases for convenience
    using file_ptr = std::shared_ptr<backend::file>;
    using dir_ptr = std::shared_ptr<backend::dir>;

    static constexpr const char* s_name = "NVRAM-NVML";

public:
    nvml_backend(uint64_t capacity, bfs::path daxfs_mount, bfs::path root_dir, int64_t segment_size);
    ~nvml_backend();

    std::string name() const override;
    uint64_t capacity() const override;
    error_code load(const bfs::path& pathname, backend::file::type type) override;
    error_code unload(const bfs::path& pathname, const bfs::path & mntpathname) override;
    bool exists(const char* pathname) const override;
    int do_readdir (const char * path, void * buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) const override;
    int do_stat ( const char * path, struct stat& stbuf) const override;
    int do_create(const char* pathname, mode_t mode, std::shared_ptr < backend::file> & file) override;
    int do_unlink(const char * pathname) override;
    int do_rename(const char * oldpath, const char * newpath) override;
    int do_mkdir(const char * pathname, mode_t mode) override;
    int do_rmdir(const char * pathname) override;
    
    int do_chmod(const char * pathname, mode_t mode) override;
    int do_chown(const char * pathname, uid_t owner, gid_t group) override;
    void do_change_type(const char * pathname, backend::file::type type) override;
    int new_inode() const;


    backend::iterator find(const char* path) override;
    backend::iterator begin() override;
    backend::iterator end() override;
    backend::const_iterator cbegin() override;
    backend::const_iterator cend() override;



private:
    /* maximum allocatable size in bytes */
    uint64_t m_capacity;

    /* mount point of the DAX filesystem needed to access NVRAM */
    bfs::path m_daxfs_mount_point;
    bfs::path m_root_dir;

    mutable std::mutex                    m_files_mutex;
    
    std::unordered_map<std::string, file_ptr> m_files;

    mutable std::mutex m_dirs_mutex;
    std::unordered_map<std::string, dir_ptr> m_dirs;
    
    mutable std::atomic<ino_t> i_inode;

    std::list <std::string> find_s(const std::string path) const;

    // Utils
    std::string remove_root (std::string path) const;
    int do_renamedir(std::string opath, std::string npath);
}; // nvml_backend

} // namespace nvml
} // namespace efsng

#endif /* __NVML_BACKEND_H__ */
