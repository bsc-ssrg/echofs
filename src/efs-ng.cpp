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
 * Mercurium C/C++ source-to-source compiler is distributed in the hope  *
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the    *
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Mercurium C/C++ source-to-source compiler; if      *
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,       *
 * Cambridge, MA 02139, USA.                                             *
 *************************************************************************/

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <memory>
#include <cstring>
#include <iostream>
#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "config.h"
#include "metadata/files.h"
#include "metadata/dirs.h"
#include "command-line.h"

static int efsng_getattr(const char* pathname, struct stat* stbuf){

    if(lstat(pathname, stbuf) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_readlink(const char* pathname, char* buf, size_t bufsiz){

    int res = readlink(pathname, buf, bufsiz - 1);

    if(res == -1){
        return -errno;
    }

    buf[res] = '\0';

    return 0;
}

static int efsng_getdir(const char* pathname, fuse_dirh_t handle, fuse_dirfil_t filler){

    (void) pathname;
    (void) handle;
    (void) filler;

    return 0;
}

static int efsng_mknod(const char* pathname, mode_t mode, dev_t dev){

    if(mknod(pathname, mode, dev) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_mkdir(const char* pathname, mode_t mode){

    if(mkdir(pathname, mode) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_unlink(const char* pathname){

    if(unlink(pathname) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_rmdir(const char* pathname){

    if(rmdir(pathname) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_symlink(const char* oldpath, const char* newpath){

    if(symlink(oldpath, newpath) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_rename(const char* oldpath, const char* newpath){

    if(rename(oldpath, newpath) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_link(const char* oldpath, const char* newpath){

    if(link(oldpath, newpath) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_chmod(const char* pathname, mode_t mode){

    if(chmod(pathname, mode) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_chown(const char* pathname, uid_t owner, gid_t group){

    if(lchown(pathname, owner, group) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_truncate(const char* pathname, off_t length){

    if(truncate(pathname, length) == -1){
        return -errno;
    }

    return 0;
}

static int efsng_utime(const char* pathname, struct utimbuf* times){

    (void) pathname;
    (void) times;

    return 0;
}

static int efsng_open(const char* pathname, struct fuse_file_info* file_info){

    int fd = open(pathname, file_info->flags);

    if(fd == -1){
        return -errno;
    }

    int flags = 0;

    if(file_info->flags & O_RDONLY){
        flags |= O_RDONLY;
    }

    if(file_info->flags & O_WRONLY){
        flags |= O_WRONLY;
    }

    if(file_info->flags & O_RDWR){
        flags |= O_RDWR;
    }

    /* cache the inode, fd, and flags to reuse them later */
	struct stat st;
	fstat(fd, &st);

    // XXX this means that each "open()" creates a File record 
    // XXX WARNING: records ARE NOT protected by a mutex yet
    auto ptr = new efsng::File(st.st_ino, fd, flags);
    file_info->fh = (uint64_t) ptr;

    return 0;
}

static int efsng_read(const char* pathname, char* buf, size_t count, off_t offset, struct fuse_file_info* file_info){

    (void) pathname;

    auto ptr = (efsng::File*) file_info->fh;

    int fd = ptr->get_fd();

    int res = pread(fd, buf, count, offset);

    if(res == -1){
        return -errno;
    }

    return res;
}

static int efsng_write(const char* pathname, const char* buf, size_t count, off_t offset, struct fuse_file_info* file_info){

    (void) pathname;

    auto ptr = (efsng::File*) file_info->fh;

    int fd = ptr->get_fd();

    int res = pwrite(fd, buf, count, offset);

    if(res == -1){
        return -errno;
    }

    return res;
}

static int efsng_statfs(const char* pathname, struct statvfs* buf){

    int res = statvfs(pathname, buf);

    if(res == -1){
        return -errno;
    }

    return 0;
}

static int efsng_flush(const char* pathname, struct fuse_file_info* file_info){

    (void) pathname;

    auto ptr = (efsng::File*) file_info->fh;

    int fd = ptr->get_fd();

	/* This is called from every close on an open file, so call the
	   close on the underlying filesystem.	But since flush may be
	   called multiple times for an open file, this must not really
	   close the file.  This is important if used on a network
	   filesystem like NFS which flush the data/metadata on close() */
	int res = close(dup(fd));

	if (res == -1){
		return -errno;
    }

    return 0;
}

static int efsng_release(const char* pathname, struct fuse_file_info* file_info){

    (void) pathname;

    auto ptr = (efsng::File*) file_info->fh;

    int fd = ptr->get_fd();

    if(close(fd) == -1){
        return -errno;
    }

    delete ptr;

    return 0;
}

static int efsng_fsync(const char* pathname, int datasync, struct fuse_file_info* file_info){

    (void) pathname;
    (void) datasync;
    (void) file_info;

    auto ptr = (efsng::File*) file_info->fh;

    int fd = ptr->get_fd();

	int res;

#ifdef HAVE_FDATASYNC
	if(isdatasync){
		res = fdatasync(fd);
    }
	else{
		res = fsync(fd);
    }
#else
    res = fsync(fd);
#endif

	if(res == -1){
		return -errno;
    }

    return 0;
}

#ifdef HAVE_XATTR
static int efsng_setxattr(const char* pathname, const char* name, const char* value, size_t size, int flags){

    (void) pathname;
    (void) name;
    (void) value;
    (void) size;
    (void) flags;

    return 0;
}

static int efsng_getxattr(const char* pathname, const char* name, char* value, size_t size){

    (void) pathname;
    (void) name;
    (void) value;
    (void) size;

    return 0;
}

static int efsng_listxattr(const char* pathname, char* name, size_t size){

    (void) pathname;
    (void) name;
    (void) size;

    return 0;
}

static int efsng_removexattr(const char* pathname, const char* name){

    (void) pathname;
    (void) name;

    return 0;
}
#endif /* HAVE_XATTR */

static int efsng_opendir(const char* pathname, struct fuse_file_info* file_info){

    DIR* dp = opendir(pathname);

    if(dp == NULL){
        return -errno;
    }

    auto ptr = new efsng::Directory(dp, NULL, 0);
    file_info->fh = (uint64_t) ptr;

    return 0;
}

static int efsng_readdir(const char* pathname, void* buf, fuse_fill_dir_t filler, off_t offset, 
                         struct fuse_file_info* file_info){

    (void) pathname;

    auto ptr = (efsng::Directory*) file_info->fh;

    if(offset != ptr->get_offset()){
        seekdir(ptr->get_dirp(), offset);
        ptr->set_entry(NULL);
        ptr->set_offset(offset);
    }

    while(true){
        struct stat st;
        off_t next_offset;

        if(ptr->get_entry() == NULL){
            struct dirent* dirent = readdir(ptr->get_dirp());
            ptr->set_entry(dirent);

            if(dirent == NULL){
                break;
            }
        }

        memset(&st, 0, sizeof(st));
        st.st_ino = ptr->get_entry()->d_ino;
        st.st_mode = ptr->get_entry()->d_type << 12;
        next_offset = telldir(ptr->get_dirp());

        if(filler(buf, ptr->get_entry()->d_name, &st, next_offset) != 0){
            break;
        }

        ptr->set_entry(NULL);
        ptr->set_offset(0);
    }

    return 0;
}

static int efsng_releasedir(const char* pathname, struct fuse_file_info* file_info){

    (void) pathname;

    auto ptr = (efsng::Directory*) file_info->fh;

    if(closedir(ptr->get_dirp()) == -1){
        return -errno;
    }

    delete ptr;

    return 0;
}

static int efsng_fsyncdir(const char* pathname, int, struct fuse_file_info* file_info){

    (void) pathname;
    (void) file_info;

    return 0;
}

static void* efsng_init(struct fuse_conn_info *conn){

    (void) conn;

    return 0;
}

static void efsng_destroy(void *){
}

static int efsng_access(const char* pathname, int mode){

    (void) pathname;
    (void) mode;

    return 0;
}

static int efsng_create(const char* pathname, mode_t mode, struct fuse_file_info* file_info){

    (void) pathname;
    (void) mode;
    (void) file_info;

    return 0;
}

static int efsng_ftruncate(const char* pathname, off_t offset, struct fuse_file_info* file_info){

    (void) pathname;
    (void) offset;
    (void) file_info;

    return 0;
}

static int efsng_fgetattr(const char* pathname, struct stat*, struct fuse_file_info* file_info){

    (void) pathname;
    (void) file_info;

    return 0;
}

static int efsng_lock(const char* pathname, struct fuse_file_info* file_info, int cmd, struct flock* flock){

    (void) pathname;
    (void) file_info;
    (void) cmd;
    (void) flock;

    return 0;
}

static int efsng_utimens(const char* pathname, const struct timespec tv[2]){

    (void) pathname;
    (void) tv;

    return 0;
}

static int efsng_bmap(const char* pathname, size_t blocksize, uint64_t* idx){

    (void) pathname;
    (void) blocksize;
    (void) idx;

    return 0;
}

static int efsng_ioctl(const char* pathname, int cmd, void* arg, struct fuse_file_info* file_info, unsigned int flags, void* data){

    (void) pathname;
    (void) cmd;
    (void) arg;
    (void) file_info;
    (void) flags;
    (void) data;

    return 0;
}

static int efsng_poll(const char* pathname, struct fuse_file_info* file_info, struct fuse_pollhandle* ph, unsigned* reventsp){

    (void) pathname;
    (void) file_info;
    (void) ph;
    (void) reventsp;

    return 0;
}

static int efsng_write_buf(const char* pathname, struct fuse_bufvec* buf, off_t off, struct fuse_file_info* file_info){

    (void) pathname;
    (void) buf;
    (void) off;
    (void) file_info;

    return 0;
}

static int efsng_read_buf(const char* pathname, struct fuse_bufvec** bufp, size_t size, off_t off, struct fuse_file_info* file_info){

    (void) pathname;
    (void) bufp;
    (void) size;
    (void) off;
    (void) file_info;

    return 0;
}

static int efsng_flock(const char* pathname, struct fuse_file_info* file_info, int op){

    (void) pathname;
    (void) file_info;
    (void) op;

    return 0;
}

static int efsng_fallocate(const char* pathname, int, off_t, off_t, struct fuse_file_info* file_info){

    (void) pathname;
    (void) file_info;

    return 0;
}

/*********************************************************************************************************************/
/*  main point of entry                                                                                              */
/*********************************************************************************************************************/
int main (int argc, char *argv[]){

    /* 1. parse command-line arguments */
    std::shared_ptr<Arguments> user_args(new Arguments);

    for(int i=0; i<MAX_FUSE_ARGS; ++i){
        /* libfuse expects NULL args */
        user_args->fuse_argv[i] = NULL;
    }

    if(argc == 1 || !process_args(argc, argv, user_args)){
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    for(int i=0; i<user_args->fuse_argc; ++i){
        std::cerr << user_args->fuse_argv[i] << "\n";
    }

    /* 2. prepare operations */
    fuse_operations efsng_ops;
    memset(&efsng_ops, 0, sizeof(fuse_operations));

    efsng_ops.getattr = efsng_getattr;
    efsng_ops.readlink = efsng_readlink;
    efsng_ops.getdir = efsng_getdir; /* deprecated */
    efsng_ops.mknod = efsng_mknod;
    efsng_ops.mkdir = efsng_mkdir;
    efsng_ops.unlink = efsng_unlink;
    efsng_ops.rmdir = efsng_rmdir;
    efsng_ops.symlink = efsng_symlink;
    efsng_ops.rename = efsng_rename;
    efsng_ops.link = efsng_link;
    efsng_ops.chmod = efsng_chmod;
    efsng_ops.chown = efsng_chown;
    efsng_ops.truncate = efsng_truncate;
    efsng_ops.utime = efsng_utime;
    efsng_ops.open = efsng_open;
    efsng_ops.read = efsng_read;
    efsng_ops.write = efsng_write;
    efsng_ops.statfs = efsng_statfs;
    efsng_ops.flush = efsng_flush;
    efsng_ops.release = efsng_release;
    efsng_ops.fsync = efsng_fsync;
#ifdef HAVE_XATTR
    efsng_ops.setxattr = efsng_setxattr;
    efsng_ops.getxattr = efsng_getxattr;
    efsng_ops.listxattr = efsng_listxattr;
    efsng_ops.removexattr = efsng_removexattr;
#endif /* HAVE_XATTR */
    efsng_ops.opendir = efsng_opendir;
    efsng_ops.readdir = efsng_readdir;
    efsng_ops.releasedir = efsng_releasedir;
    efsng_ops.fsyncdir = efsng_fsyncdir;
    efsng_ops.init = efsng_init;
    efsng_ops.destroy = efsng_destroy;
    efsng_ops.access = efsng_access;
    efsng_ops.create = efsng_create;
    efsng_ops.ftruncate = efsng_ftruncate;
    efsng_ops.fgetattr = efsng_fgetattr;
    efsng_ops.lock = efsng_lock;
    efsng_ops.utimens = efsng_utimens;
    efsng_ops.bmap = efsng_bmap;
    efsng_ops.ioctl = efsng_ioctl;
    efsng_ops.poll = efsng_poll;
    efsng_ops.write_buf = efsng_write_buf;
    efsng_ops.read_buf = efsng_read_buf;
    efsng_ops.flock = efsng_flock;
    efsng_ops.fallocate = efsng_fallocate;

    /* 3. start the filesystem */
    int res = fuse_main(user_args->fuse_argc, 
                        const_cast<char **>(user_args->fuse_argv), 
                        &efsng_ops, 
                        (void*) NULL);

    std::cout << "(" << res << ") Bye!\n";

    return 0;
}
