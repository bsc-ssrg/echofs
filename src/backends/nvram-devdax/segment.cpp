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


#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <libpmem.h>
#include <sys/mman.h>
#include <logger.h>
#include <efs-common.h>
#include <nvram-devdax/file.h>
#include <nvram-devdax/segment.h>

#include "utils.h"
namespace bfs = boost::filesystem;

namespace {

/** generate a name for an NVML pool using a prefix and a base_path */
bfs::path generate_pool_path(const bfs::path& subdir) {

  //  static boost::random::mt19937 rng;

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    bfs::path ppath(boost::uuids::to_string(uuid));

    return subdir / ppath;
}

}

namespace efsng {
namespace nvml_dev {

size_t segment::s_segment_size = segment::default_segment_size;


int big_pool::init (const bfs::path & subdir, size_t length) {
	m_offset = 0;
	m_length = length;
    std::lock_guard<std::mutex> lock_allocate(m_allocate_mutex);
    m_size_of_segment = segment::s_segment_size;
    m_fd = open(subdir.c_str(), O_RDWR);
    std::cerr << " FD Result for open " << m_fd << std::endl;
     //  std::min(length / NVML_MAPPINGS, s_segment_size); // 4 KB is the minimum:
    m_NVML_MAPPINGS = length / m_size_of_segment;
    m_address = mmap(NULL,m_length, PROT_READ|PROT_WRITE, MAP_SHARED,
              m_fd, 0);

    std::cerr << "MMAP result " << m_address << " - " << errno << " - " << strerror (errno) << std::endl;

    m_init = true;
	return 0;
}

big_pool::~big_pool() {

    munmap (m_address, m_length);
    close(m_fd);
}

void * big_pool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock_allocate(m_allocate_mutex);
    void * ret_address = NULL;

    unsigned int n_segments = size / m_size_of_segment;

    bool found = true;
    size_t start_offset = m_offset;
    if ( n_segments + start_offset < m_NVML_MAPPINGS) {

        for (unsigned int i = 0; i < n_segments; i++) {
            if (m_bitset.test(i+start_offset) == true) {
                found = false;
                start_offset = 0;
                break;
            }
        }
 
    } else { start_offset = 0; found = false; }

    if ( !found ) {
        found = true;
        start_offset = 0;
        for (unsigned int j = 0; j < m_NVML_MAPPINGS; j++){
            found = true;
            start_offset = j;
            if ( n_segments + start_offset < m_NVML_MAPPINGS) {
            for (unsigned int i = 0; i < n_segments; i++)   {
                if (m_bitset.test(i+start_offset)) {
                    found = false;
                    start_offset = start_offset + i + 1;
                    break;
                }
            }
            if (found == false) j = start_offset;
            else {
                break;
            }
        } else { found = false;  LOGGER_INFO ("Device is full"); break;}
    }
    }


    if ( found ) { // We reserve all of them
        ret_address = (void *)((unsigned long long)m_address + start_offset*m_size_of_segment);
        for (unsigned int i = 0; i < n_segments; i++)
            m_bitset.set(i+start_offset);
        m_offset =  n_segments + start_offset + 1;
    }
    else {
        LOGGER_INFO ("Device is full");
    }

    if ((m_offset-1)*m_size_of_segment >= m_length) m_offset = 0; // We start over again

    return ret_address;
}

void big_pool::deallocate(void * address, size_t length) {
    std::lock_guard<std::mutex> lock_allocate(m_allocate_mutex);
    for (unsigned int i = 0 ; i < length / m_size_of_segment; i++) { 
       m_bitset.reset( ((unsigned long long)address-(unsigned long long)m_address + i*m_size_of_segment) / m_size_of_segment );
   }
}

big_pool pool::m_bpool;
// we need a definition of the constant because std::min/max rely on references
// (see: http://stackoverflow.com/questions/16957458/static-const-in-c-class-undefined-reference)

pool::pool(const bfs::path& subdir)
    : m_subdir(subdir),
      m_path(),
      m_data(NULL),
      m_is_pmem(0) {
	
        if (pool::m_bpool.m_init == false)
        {
	    unsigned long long size = 1024*1024*1024;
	    size *= 600;
            pool::m_bpool.init(m_subdir, size);
        }
      }

pool::~pool() {
//    std::cerr << "Died! (" << m_data << ")\n";

    // release the mapped region
    if(m_data != NULL) {
        pool::m_bpool.deallocate(m_data, m_length);
    }
}
void pool::deallocate() {
    if (m_data != NULL) {
        pool::m_bpool.deallocate(m_data, m_length);
        m_data = NULL;
    }
}

void pool::allocate(size_t size) {

    assert(m_data == NULL);

    bfs::path pool_path; 
    void* pool_addr = NULL;
    size_t pool_length = 0;
    int is_pmem = 0;

    pool_addr = m_bpool.allocate(size);
    pool_length = size;

    m_path = pool_path;
    m_data = pool_addr;
    m_length = pool_length;
    m_is_pmem = is_pmem;
}

segment::segment(const bfs::path& subdir, off_t offset, size_t size, bool is_gap)
    : m_offset(offset), 
      m_size(size),
      m_is_gap(is_gap),
      m_pool(subdir), 
      m_is_free(false) {

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

void segment::deallocate(){
    m_pool.deallocate();
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

std::ostream& operator<<(std::ostream& os, const efsng::nvml_dev::segment& mp) {
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
