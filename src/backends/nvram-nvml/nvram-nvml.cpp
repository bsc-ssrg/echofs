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
#include <errno.h>

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
    m_dirs.emplace("/", std::make_unique<nvml::dir>("/",new_inode(), m_root_dir));
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

    std::string path_wo_root = remove_root (pathname.string());
    
    if (bfs::is_directory(pathname)){
        // Recursive upload
        bfs::recursive_directory_iterator r(pathname);
        for (auto entry : r) {
            if (!bfs::is_directory(entry)) {
                auto error = load(entry);
                if (error != error_code::success) return error;
            }
        }
        return error_code::success;
    }

    /* add the mapping to a nvml::file descriptor and insert it into m_files */
    std::lock_guard<std::mutex> lock(m_files_mutex);
    if(m_files.count(path_wo_root) != 0) {
        return error_code::path_already_imported;
    }


    /* create a new file into m_files (the constructor will fill it with
     * the contents of the pathname) */
    auto it = m_files.emplace(path_wo_root, 
                              std::make_unique<nvml::file>(m_daxfs_mount_point, pathname, new_inode()));

    // Iterate the path to fill the info
    std::vector <std::string> m_path;
    for(auto& part : boost::filesystem::path(path_wo_root)){
        m_path.push_back (part.c_str());
    }

    std::string tmp_parent = m_path.front();
    std::string t_path = path_wo_root;

    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
    
    for (int i = m_path.size()-1; i>0; i--){
        // Build the path
        t_path = t_path.substr(0,t_path.rfind(m_path[i]));
        auto d_it = m_dirs.find(t_path);

        if (d_it == m_dirs.end()){
            // ADD PATH
            auto t_it = m_dirs.emplace(t_path, std::make_unique<nvml::dir>(t_path,new_inode(), m_root_dir.string()+t_path));
            t_it.first->second.get()->add_file(m_path[i]);
        }
        else{
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
   
    std::list <std::string> ls = find_s(path);
    
    for (auto &file : ls) {    
        if (file.length() > 0 and file[file.length()-1]=='/') { 
            // We remove the last slash
            file.pop_back();
        }
        filler(buffer, file.c_str(), NULL, 0);
    }
    return 0;
}


int nvml_backend::do_stat (const char * path, struct stat& stbuf) const {

    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);

    std::string path_wo_root_slash = path;
    if (path_wo_root_slash.back() != '/') path_wo_root_slash.push_back('/');
    auto dir = m_dirs.find(path_wo_root_slash);
    if (dir != m_dirs.end()) {
        //Fill directory entry
        dir->second.get()->stat(stbuf);
    }
    else  {
        std::lock_guard<std::mutex> lock(m_files_mutex);
        auto file = m_files.find(path);
        if (file != m_files.end()) {
            const auto& file_ptr = file->second;
            file_ptr->stat(stbuf);
        } 
        else { 
            LOGGER_DEBUG("do_stat not found file {} - {} ",path, path_wo_root_slash); 
            return -ENOENT;
        }
    }
     return 0;
}

/* Those are temporary files */
int nvml_backend::do_create(const char* pathname, mode_t mode, std::shared_ptr < backend::file> & file) {
    LOGGER_DEBUG("Inside backend do_create for {}",pathname);

    std::string path_wo_root = pathname;

    std::lock_guard<std::mutex> lock(m_files_mutex);
    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
    if(m_files.count(path_wo_root) != 0) {
        return -1;
    }

     // Add to the directory
    
    std::string path_wo_root_slash = path_wo_root.substr(0,path_wo_root.rfind('/')+1);

   
    if (path_wo_root_slash.size() == 0 or path_wo_root_slash.back() != '/') path_wo_root_slash.push_back('/');
    auto dir = m_dirs.find(path_wo_root_slash);
    if (dir != m_dirs.end()) {
         dir->second.get()->add_file(path_wo_root.substr(path_wo_root.rfind('/')+1));
    } else  {
        LOGGER_DEBUG("[CREATE] DIR {} - {} not found", path_wo_root_slash, pathname);
        return -1; 
    }
    
    /* create a new file into m_files (the constructor will fill it with
     * the contents of the pathname) */
    auto it = m_files.emplace(path_wo_root, 
                              std::make_unique<nvml::file>(m_daxfs_mount_point, pathname, 0 ,file::type::temporary,false));

    struct stat stbuf;
    memset ((void*)&stbuf,0,sizeof(struct stat));
    stbuf.st_ino = new_inode();
    stbuf.st_uid = getuid();
    stbuf.st_gid = getgid();
    stbuf.st_mode = mode;
    stbuf.st_nlink = 1;
    stbuf.st_atime = time (NULL);
    stbuf.st_mtime = time (NULL);
    stbuf.st_ctime = time (NULL);
   
    auto& file_ptr = (*(it.first)).second;
    auto nvml_f_ptr = dynamic_cast<nvml::file * >(file_ptr.get());
    nvml_f_ptr->save_attributes(stbuf);
    file = std::shared_ptr<backend::file> (file_ptr);
   
    return 0;
}



int nvml_backend::do_unlink(const char * pathname) {
    std::string path = pathname;
    std::lock_guard<std::mutex> lock(m_files_mutex);
    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
    if(m_files.count(pathname) == 0) {
        return -ENOENT;
    }
    // Remove the file
    auto file = m_files.find(path);
    if (file != m_files.end()) {
        const auto& file_ptr = file->second;
        m_files.erase(file);
    } 
    else { 
        LOGGER_DEBUG("do_unlink not found file {}",pathname); 
        return -ENOENT;
    }

    std::string path_wo_root_slash = path.substr(0,path.rfind('/')+1);

   
    if (path_wo_root_slash.size() == 0 or path_wo_root_slash.back() != '/') path_wo_root_slash.push_back('/');
    auto dir = m_dirs.find(path_wo_root_slash);
    if (dir != m_dirs.end()) {
         dir->second.get()->remove_file(path.substr(path.rfind('/')+1));
    }

    return 0;
}

int nvml_backend::do_rename(const char * oldpath, const char * newpath) {
    // Create the other file
    std::string opath = oldpath;
    std::string npath = newpath;
  
    std::lock_guard<std::mutex> lock(m_files_mutex);
    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
    if(m_files.count(npath) != 0) {
        return -1; // file exists
    }

    if(m_files.count(opath) == 0) {
        return -ENOENT; // file do not exists
    }

    // Create the new file

    // Search for oldpointer 
    auto file = m_files.find(opath);
    const auto& file_ptr = file->second;
     
    auto it = m_files.emplace(npath, file_ptr);
    
    // remove old file

    m_files.erase(file); // Check: pointer should not be deleted...
    
    // Now we should update directory

    std::string opath_wo_root_slash = opath.substr(0,opath.rfind('/')+1);
    std::string npath_wo_root_slash = npath.substr(0,npath.rfind('/')+1);

   
    if (opath_wo_root_slash.size() == 0 or opath_wo_root_slash.back() != '/') opath_wo_root_slash.push_back('/');
    if (npath_wo_root_slash.size() == 0 or npath_wo_root_slash.back() != '/') npath_wo_root_slash.push_back('/');
    
    auto dir = m_dirs.find(opath_wo_root_slash);
    if (dir != m_dirs.end()) {
         dir->second.get()->remove_file(opath.substr(opath.rfind('/')+1));
    }

    dir = m_dirs.find(npath_wo_root_slash);
    if (dir != m_dirs.end()) {
         dir->second.get()->add_file(npath.substr(npath.rfind('/')+1));
    }

   return 0;
}


int nvml_backend::do_mkdir(const char * pathname, mode_t mode) {
    LOGGER_DEBUG("Inside backend do_mkdir for {}",pathname);

    std::string path_wo_root = pathname;

    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
     // Add to the directory
    if (path_wo_root.size() == 0 or path_wo_root.back() != '/') path_wo_root.push_back('/');
    auto dir = m_dirs.find(path_wo_root);
    if (dir == m_dirs.end()) {


        auto it = m_dirs.emplace(path_wo_root, std::make_unique<nvml::dir>(path_wo_root, new_inode(), m_root_dir.string() + path_wo_root,dir::type::temporary,false));

        struct stat stbuf;
        it.first->second.get()->stat(stbuf);
        // Update stbuf
       
        stbuf.st_uid = getuid();
        stbuf.st_gid = getgid();
        stbuf.st_mode = mode | S_IFDIR;

        stbuf.st_atime = time (NULL);
        stbuf.st_mtime = time (NULL);
        stbuf.st_ctime = time (NULL);

        it.first->second.get()->save_attributes(stbuf);
        path_wo_root.pop_back();
        auto parent_path = path_wo_root.substr(0,path_wo_root.rfind('/')+1);

        auto parent_dir = m_dirs.find(parent_path);
        assert (parent_dir != m_dirs.end());
        auto dirname = path_wo_root.substr(path_wo_root.rfind('/')+1);
              
        parent_dir->second.get()->add_file(dirname);
      
    } else {
        LOGGER_DEBUG("[CREATE] MKDIR {} dir found", path_wo_root);
        return -1; 
    }
    return 0;
}


int nvml_backend::do_rmdir(const char * pathname) {
    LOGGER_DEBUG("Inside backend do_rmdir for {}",pathname);

    std::string path_wo_root = pathname;

    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
     // Add to the directory
    if (path_wo_root.size() == 0 or path_wo_root.back() != '/') path_wo_root.push_back('/');
    auto dir = m_dirs.find(path_wo_root);
    if (dir != m_dirs.end()) {

        struct stat stbuf;
        dir->second.get()->stat(stbuf);

        if (stbuf.st_ino != 1) return -ENOTEMPTY;

        m_dirs.erase(dir);  

        path_wo_root.pop_back();
        auto parent_path = path_wo_root.substr(0,path_wo_root.rfind('/')+1);

        auto parent_dir = m_dirs.find(parent_path);
        assert (parent_dir != m_dirs.end());
        auto dirname = path_wo_root.substr(path_wo_root.rfind('/')+1);
              
        parent_dir->second.get()->remove_file(dirname);
      
    } else {
        LOGGER_DEBUG("[CREATE] RMDIR {} dir not found", path_wo_root);
        return -ENOENT; 
    }
    return 0;
}


int nvml_backend::do_chmod(const char * pathname, mode_t mode) {  
    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
    std::string path_wo_root_slash = pathname;
    struct stat stbuf;
    if (path_wo_root_slash.back() != '/') path_wo_root_slash.push_back('/');
    auto dir = m_dirs.find(path_wo_root_slash);
    if (dir != m_dirs.end()) {
        //Fill directory entry
        dir->second.get()->stat(stbuf);
        stbuf.st_mode = mode;
        dir->second.get()->save_attributes(stbuf);
    }
    else  {
        std::lock_guard<std::mutex> lock(m_files_mutex);
        auto file = m_files.find(pathname);
        if (file != m_files.end()) {
            const auto& file_ptr = file->second;
            file_ptr->stat(stbuf);
            stbuf.st_mode = mode;
            file_ptr->save_attributes(stbuf);
        } 
        else { 
            LOGGER_DEBUG("do_chmod not found file {} ",pathname); 
            return -ENOENT;
        }
    }
     return 0;
}

int nvml_backend::do_chown(const char * pathname, uid_t owner, gid_t group) {
    std::lock_guard<std::mutex> lock_dir(m_dirs_mutex);
    std::string path_wo_root_slash = pathname;
    struct stat stbuf;
    if (path_wo_root_slash.back() != '/') path_wo_root_slash.push_back('/');
    auto dir = m_dirs.find(path_wo_root_slash);
    if (dir != m_dirs.end()) {
        //Fill directory entry
        dir->second.get()->stat(stbuf);
        stbuf.st_uid = owner;
        stbuf.st_gid = group;
        dir->second.get()->save_attributes(stbuf);
    }
    else  {
        std::lock_guard<std::mutex> lock(m_files_mutex);
        auto file = m_files.find(pathname);
        if (file != m_files.end()) {
            const auto& file_ptr = file->second;
            file_ptr->stat(stbuf);
            stbuf.st_uid = owner;
            stbuf.st_gid = group;
            file_ptr->save_attributes(stbuf);
        } 
        else { 
            LOGGER_DEBUG("do_chown not found file {} ",pathname); 
            return -ENOENT;
        }
    }
     return 0;
}



backend::iterator nvml_backend::find(const char* path) {
    LOGGER_DEBUG ("FIND {}", path);
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
    if (d_it != m_dirs.end()) {
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

int nvml_backend::new_inode() const {
    return i_inode++;
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
