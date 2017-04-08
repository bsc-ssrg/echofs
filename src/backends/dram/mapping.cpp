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
#include <sys/mman.h>


#include "../../logging.h"
#include "../../efs-common.h"
#include "file.h"
#include "mapping.h"

namespace bfs = boost::filesystem;

namespace efsng {
namespace dram {

mapping::mapping(const mapping& mp) 
    : m_data(mp.m_data),
      m_offset(mp.m_offset),
      m_size(mp.m_size),
      m_bytes(mp.m_bytes),
      m_copies(mp.m_copies),
      m_pmutex(new std::mutex()) {
}

mapping::mapping(mapping&& other) noexcept
    : m_data(std::move(other.m_data)),
      m_offset(std::move(other.m_offset)),
      m_size(std::move(other.m_size)),
      m_bytes(std::move(other.m_bytes)),
      m_copies(std::move(other.m_copies)),
      m_pmutex(std::move(other.m_pmutex)) {

        // std::move will take care of resetting non-POD members,
        // let's also make sure to reset PODs, so that the moved 
        // from instance does not release the internal mapping when 
        // its destructor is called
        other.m_data = 0;
        other.m_offset = 0;
        other.m_size = 0;
        other.m_bytes = 0;

        std::cerr << "mover called!\n";

        std::cerr << "<< " << m_pmutex.get() << "\n";
}

mapping::mapping(size_t min_size){

    void* mapping_addr;

    /* ensure that the mapping is aligned to FUSE_BLOCK_SIZE, even if
     * we don't have (yet) enough data to fill it with 
     * XXX: this will provoke some waste of storage space for small files
     * if a large FUSE_BLOCK_SIZE is used */
    size_t aligned_size = efsng::xalign(min_size, efsng::FUSE_BLOCK_SIZE);


    if((mapping_addr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == NULL) {
        BOOST_LOG_TRIVIAL(error) << "Error creating RAM mapping: " << strerror(errno);
        throw std::runtime_error("");
    }
    
    /* initialize members */
    m_data = mapping_addr;
    m_offset = 0;
    m_size = aligned_size;
    m_bytes = 0; // will be filled by populate()
    // m_copies is default initialized as empty
    m_pmutex = std::unique_ptr<std::mutex>(new std::mutex());
}

mapping::~mapping() {

    std::cerr << "Died!\n";

    if(m_data != 0){
        if(munmap(m_data, m_size) == -1) {
            BOOST_LOG_TRIVIAL(error) << "Error while deleting mapping";
        }
        else{
            std::cerr << "Mapping released :P\n";
        }
    }
}

void mapping::populate(const posix::file& fdesc) {
    m_bytes = copy_data_to_ram(fdesc);
}

ssize_t mapping::copy_data_to_ram(const posix::file& fdesc){

    char* addr = (char*) m_data;
    char* buffer = (char*) malloc(DRAM_TRANSFER_SIZE*sizeof(*buffer));

    if(buffer == NULL){
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate temporary buffer: " << strerror(errno);
    }

    ssize_t total = 0;
    ssize_t n;

    while((n = read(fdesc.m_fd, buffer, DRAM_TRANSFER_SIZE)) != 0){

        if(n == -1){
            if(errno != EINTR){
                BOOST_LOG_TRIVIAL(error) << "Error while reading: " << strerror(errno);
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


//data_copy::data_copy()
//    : m_offset(0), 
//      m_data(NULL),
//      m_generation(1),
//      m_refs(0) {
//}

data_copy::data_copy(const data_copy& orig)
    : m_offset(orig.m_offset),
      m_data(orig.m_data) {

    m_generation.store(m_generation.load());
    m_refs.store(m_refs.load());
}

data_copy& data_copy::operator=(const data_copy& orig) {

    if(this != &orig) {
        m_offset = orig.m_offset;
        m_data = orig.m_data;
        m_generation.store(m_generation.load());
        m_refs.store(m_refs.load());
    }

    return *this;
}




} // namespace dram
} // namespace efsng


#ifdef __DEBUG__

std::ostream& operator<<(std::ostream& os, const efsng::dram::mapping& mp) {
    os << "mapping {" << "\n"
       << "  m_data: " << mp.m_data << "\n"
       << "  m_offset: " << mp.m_offset << "\n"
       << "  m_size: " << mp.m_size << "\n"
       << "  m_bytes: " << mp.m_bytes << "\n"
       << "  m_copies.size(): " << mp.m_copies.size() << "\n"
       << "  m_pmutex: " << mp.m_pmutex.get() << "\n"
       << "};" << "\n";

    return os;
}

#endif /* __DEBUG__ */
