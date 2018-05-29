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

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <libpmem.h>

#include <logger.h>
#include <efs-common.h>
#include <nvram-nvml/file.h>
#include <nvram-nvml/segment.h>

namespace bfs = boost::filesystem;

namespace {

/** generate a name for an NVML pool using a prefix and a base_path */
bfs::path generate_pool_path(const bfs::path& subdir) {

    static boost::random::mt19937 rng;

    boost::uuids::uuid uuid = boost::uuids::random_generator(rng)();
    bfs::path ppath(boost::uuids::to_string(uuid));

    return subdir / ppath;
}

}

namespace efsng {
namespace nvml {

// we need a definition of the constant because std::min/max rely on references
// (see: http://stackoverflow.com/questions/16957458/static-const-in-c-class-undefined-reference)
const size_t segment::s_min_size;

pool::pool(const bfs::path& subdir)
    : m_subdir(subdir),
      m_path(),
      m_data(NULL),
      m_is_pmem(0) {}

pool::~pool() {
//    std::cerr << "Died! (" << m_data << ")\n";

    // release the mapped region
    if(m_data != NULL) {
        pmem_unmap(m_data, m_length);
        bfs::path pool_path;
        pool_path = ::generate_pool_path(m_subdir);
        if(bfs::exists(pool_path)){
            if(::unlink(pool_path.c_str()) != 0){
                throw std::runtime_error(
                    logger::build_message("Error removing pool file: ", pool_path, " (", strerror(errno), ")"));
            }
        }

    }
}

void pool::allocate(size_t size) {

    assert(m_data == NULL);

    bfs::path pool_path; 
    void* pool_addr = NULL;
    size_t pool_length = 0;
    int is_pmem = 0;

    pool_path = ::generate_pool_path(m_subdir);

    /* if the segment pool already exists delete it, since we can't trust that it's the same file */
    /* TODO: we may need to change this in the future */
    if(bfs::exists(pool_path)){
        if(::unlink(pool_path.c_str()) != 0){
            throw std::runtime_error(
                    logger::build_message("Error removing pool file: ", pool_path, " (", strerror(errno), ")"));
        }
    }
    
    /* create the pool */
    if((pool_addr = pmem_map_file(pool_path.c_str(), size, PMEM_FILE_CREATE | PMEM_FILE_EXCL | PMEM_FILE_SPARSE,
                                0666, &pool_length, &is_pmem)) == NULL) {
        throw std::runtime_error(
                logger::build_message("Fatal error creating pmem file: ", pool_path, " (", strerror(errno), ")"));
    }

    /* check that the range allocated is of the required size */
    if(pool_length != size) {
        pmem_unmap(pool_addr, pool_length);
        throw std::runtime_error("File segment of different size than requested");
    }

    m_path = pool_path;
    m_data = pool_addr;
    m_length = pool_length;
    m_is_pmem = is_pmem;
}

segment::segment(const bfs::path& subdir, off_t offset, size_t size, bool is_gap)
    : m_offset(offset), 
      m_size(size),
      m_is_gap(is_gap),
      m_pool(subdir) {

    m_bytes = 0; // will be set by fill_from()

    if(!is_gap) {
        m_pool.allocate(size);
    }
}

segment::~segment() {
    pmem_drain();
}

void segment::allocate(off_t offset, size_t size) {
    m_offset = offset;
    m_size = size;
    m_is_gap = false;
    m_pool.allocate(size);
}

void segment::sync_all() {
    pmem_drain();
}

bool segment::is_pmem() const {
    return m_pool.m_is_pmem;
}

data_ptr_t segment::data() const {
    return m_pool.m_data;
}

void segment::zero_fill(off_t offset, size_t size) {

    if(m_is_gap) {
        return;
    }

    assert(offset + size <= m_size);

    if(m_pool.m_is_pmem) {
        pmem_memset_persist((void*) ((uintptr_t) m_pool.m_data + offset), 0, size);
    }
    else {
        memset((void*) ((uintptr_t) m_pool.m_data + offset), 0, size);
    }
}

size_t segment::fill_from(const posix::file& fdesc) {
    m_bytes = m_pool.m_is_pmem ? copy_data_to_pmem(fdesc) : copy_data_to_non_pmem(fdesc);

    // fill the rest of the allocated segment with zeros so that reads beyond EOF work as expected
    zero_fill(m_bytes, m_size - m_bytes);

    return m_bytes;
}

ssize_t segment::copy_data_to_pmem(const posix::file& fdesc){

    char* addr = (char*) m_pool.m_data;
    char* buffer = (char*) malloc(NVML_TRANSFER_SIZE*sizeof(*buffer));

    if(buffer == NULL){
        //BOOST_LOG_TRIVIAL(error) << "Unable to allocate temporary buffer: " << strerror(errno);
    }

    ssize_t total = 0;
    ssize_t n;

    while((n = read(fdesc.m_fd, buffer, NVML_TRANSFER_SIZE)) != 0){

        if(n == -1){
            if(errno != EINTR){
                //BOOST_LOG_TRIVIAL(error) << "Error while reading: " << strerror(errno);
                return n;
            }
            /* EINTR, repeat  */
            continue;
        }

        pmem_memcpy_nodrain(addr, buffer, n);
        addr += n;
        total += n;
    }

    /* flush caches */
    pmem_drain();

    free(buffer);

    return total;
}

ssize_t segment::copy_data_to_non_pmem(const posix::file& fdesc){

    char* addr = (char*) m_pool.m_data;
    char* buffer = (char*) malloc(NVML_TRANSFER_SIZE*sizeof(*buffer));

    if(buffer == NULL){
        //BOOST_LOG_TRIVIAL(error) << "Unable to allocate temporary buffer: " << strerror(errno);
    }

    ssize_t total = 0;
    ssize_t n;

    while((n = read(fdesc.m_fd, buffer, NVML_TRANSFER_SIZE)) != 0){

        if(n == -1){
            if(errno != EINTR){
                //BOOST_LOG_TRIVIAL(error) << "Error while reading: " << strerror(errno);
                return n;
            }
            /* EINTR, repeat  */
            continue;
        }

        memcpy(addr, buffer, n);
        addr += n;
        total += n;
    }

    free(buffer);

    return total;
}

} // namespace nvml
} // namespace efsng


#ifdef __EFS_DEBUG__

std::ostream& operator<<(std::ostream& os, const efsng::nvml::segment& mp) {
    os << "segment {" << "\n"
       << "  m_is_gap: " << mp.m_is_gap << "\n"
       << "  m_data: " << mp.m_pool.m_data << "\n"
       << "  m_offset: " << mp.m_offset << "\n"
       << "  m_size: " << mp.m_size << "\n"
       << "  m_bytes: " << mp.m_bytes << "\n"
       << "  m_is_pmem: " << mp.m_pool.m_is_pmem << "\n"
       << "};";

    return os;
}

#endif /* __EFS_DEBUG__ */
