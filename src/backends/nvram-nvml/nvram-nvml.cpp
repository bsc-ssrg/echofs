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
#include <libpmem.h>

/* C++ includes */
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <memory>
#include <chrono>
#include <thread>
#include <list>

/* internal includes */
#include <logger.h>
#include <utils.h>
#include <posix-file.h>
#include <nvram-nvml/file.h>
#include <nvram-nvml/dir.h>
#include <nvram-nvml/segment.h>
#include <nvram-nvml/nvram-nvml.h>

namespace efsng {
namespace nvml {

nvml_backend::nvml_backend(uint64_t capacity, bfs::path daxfs_mount, bfs::path root_dir)
    : m_capacity(capacity),
      m_daxfs_mount_point(daxfs_mount),
      m_root_dir(root_dir) {
    // Insert the root dir into the map

    std::lock_guard<std::mutex> lock(m_dirs_mutex);
    m_dirs.emplace("/", std::make_unique<nvml::dir>("/"));

}

nvml_backend::~nvml_backend(){
}

std::string nvml_backend::name() const {
    return s_name;
}

uint64_t nvml_backend::capacity() const {
    return m_capacity;
}

/** start the load process of a file requested by the user */
/* pathname = file path in underlying filesystem */
error_code nvml_backend::load(const bfs::path& pathname) {

    LOGGER_DEBUG("Import {} to NVRAM", pathname);

    if(!bfs::exists(pathname)) {
        return error_code::no_such_path;
    }

    /* add the mapping to a nvml::file descriptor and insert it into m_files */
    std::lock_guard<std::mutex> lock(m_files_mutex);


    std::string path_wo_root = remove_root (pathname.string());

   
    if(m_files.count(path_wo_root) != 0) {
        return error_code::path_already_imported;
    }


    /* create a new file into m_files (the constructor will fill it with
     * the contents of the pathname) */
    auto it = m_files.emplace(path_wo_root, 
                              std::make_unique<nvml::file>(m_daxfs_mount_point, pathname));

    // Iterate the path to fill the info
    std::vector <std::string> m_path;
    for(auto& part : boost::filesystem::path(path_wo_root))
    {
        m_path.push_back (part.c_str());
    }

    std::string tmp_parent = m_path.front();
    std::string t_path = path_wo_root;

    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
    
    for (int i = m_path.size()-1; i>0; i--)
    {
        // Build the path
        t_path = t_path.substr(0,t_path.rfind(m_path[i]));
    
        auto d_it = m_dirs.find(t_path);

        if (d_it == m_dirs.end()){
            // ADD PATH
            auto t_it = m_dirs.emplace(t_path, std::make_unique<nvml::dir>(t_path));
            t_it.first->second.get()->add_file(m_path[i]);
        }
        else
        {
            d_it->second.get()->add_file(m_path[i]);
        }
    }
  
   
  //  if (s != OK) return error_code::internal_error;
    
#ifdef __EFS_DEBUG__
    const auto& file_ptr = (*(it.first)).second;

    struct stat stbuf;
    file_ptr->stat(stbuf);
    LOGGER_DEBUG("Transfer complete ({} bytes)", stbuf.st_size);
#endif

    /* lock_guard is automatically released here */
    return error_code::success;
}

error_code nvml_backend::unload(const bfs::path& pathname) {
    LOGGER_ERROR("Unload not implemented!");
    return error_code::success;
}

bool nvml_backend::exists(const char* pathname) const {

    std::lock_guard<std::mutex> lock(m_files_mutex);

    const auto& it = m_files.find(pathname);

    return it != m_files.end();
}

// TODO: Error control
int nvml_backend::do_readdir (const char * path, void * buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) const {
    LOGGER_DEBUG("Inside backend readdir for {}", path);
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    std::string pathname = path;
    std::string path_wo_root = remove_root(pathname);

    std::list <std::string> ls = find_s(path_wo_root);
    
    for (auto &file : ls)
    {    
        if (file.length() > 0 and file[file.length()-1]=='/')
        { // We remove the last slash
            file.pop_back();
        }

        filler(buffer, file.c_str(), NULL, 0);
    }
    return 0;

}

backend::iterator nvml_backend::find(const char* path) {
    return m_files.find(path);
}

std::list <std::string> nvml_backend::find_s(const std::string path) const {
    std::string result;

    std::list <std::string> l_files;

    std::lock_guard<std::mutex> lock(m_dirs_mutex);

    // Only the root path is send with /. We add the slash at the end if not.
    std::string tmp_path = path;
    std::size_t slash = tmp_path.rfind("/");
    if (slash != tmp_path.length()-1) 
        tmp_path.push_back('/');

    auto d_it = m_dirs.find(tmp_path);
    LOGGER_DEBUG("FIND_S {}",tmp_path);
    if (d_it != m_dirs.end())
    {
        // Search the path and insert it on the dir structure
        // TODO: The dir structure will have a list with pointers to the nvml::file so we reduce one indirection.
        d_it->second.get()->list_files(l_files);  // Store the substr
    }

    return l_files;
}


std::string nvml_backend::remove_root (std::string pathname) const {
    std::size_t rdir = pathname.find(m_root_dir.string());
    if (rdir == std::string::npos){
        std::string last_part = boost::filesystem::path(pathname).rbegin()->c_str();
        return "/"+last_part; //If it does not have the roor dir, just send "/" and the last part
    }

    std::string path_wo_root = pathname.substr(rdir+m_root_dir.string().length()-1); // We keep the /
    return path_wo_root;
}


backend::iterator nvml_backend::begin() {
    return m_files.begin();
}

backend::iterator nvml_backend::end() {
    return m_files.end();
}

backend::const_iterator nvml_backend::cbegin() {
    return m_files.cbegin();
}

backend::const_iterator nvml_backend::cend() {
    return m_files.cend();
}

} // namespace nvml
} //namespace efsng
