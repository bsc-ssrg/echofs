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

#include <logging.h>
#include <efs-common.h>
#include <nvram-nvml/file.h>
#include <nvram-nvml/mapping.h>

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

mapping::mapping(const mapping& mp) 
    : m_name(mp.m_name),
      m_data(mp.m_data),
      m_offset(mp.m_offset),
      m_size(mp.m_size),
      m_bytes(mp.m_bytes),
      m_is_pmem(mp.m_is_pmem),
      m_copies(mp.m_copies),
      m_pmutex(new std::mutex()) {
}

mapping::mapping(mapping&& other) noexcept
    : m_name(std::move(other.m_name)),
      m_data(std::move(other.m_data)),
      m_offset(std::move(other.m_offset)),
      m_size(std::move(other.m_size)),
      m_bytes(std::move(other.m_bytes)),
      m_is_pmem(std::move(other.m_is_pmem)),
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
        other.m_is_pmem = 0;

        //std::cerr << "mover called!\n";
        //std::cerr << "<< " << m_pmutex.get() << "\n";
}

mapping::mapping(const bfs::path& prefix, const bfs::path& base_path, size_t min_size){

    const bfs::path abs_path = generate_pool_path(prefix, base_path);

    std::string mp_name(abs_path.string());

    std::replace(mp_name.begin(), mp_name.end(), '/', '_');

    /* if the mapping file already exists delete it, since we can't trust that it's the same file */
    /* TODO: we may need to change this in the future */
    if(bfs::exists(abs_path)){
        if(unlink(abs_path.c_str()) != 0){
            //BOOST_LOG_TRIVIAL(error) << "Error removing file " << abs_path << ": " << strerror(errno);
            throw std::runtime_error("");
        }
    }

    void* pmem_addr;
    size_t mapping_length;
    int is_pmem;

    /* ensure that the mapping is aligned to FUSE_BLOCK_SIZE, even if
     * we don't have (yet) enough data to fill it with 
     * XXX: this will provoke some waste of storage space for small files
     * if a large FUSE_BLOCK_SIZE is used */
    size_t aligned_size = efsng::xalign(min_size, efsng::FUSE_BLOCK_SIZE);

    /* create the mapping */
    if((pmem_addr = pmem_map_file(abs_path.c_str(), aligned_size, PMEM_FILE_CREATE | PMEM_FILE_EXCL,
                                  0666, &mapping_length, &is_pmem)) == NULL) {
        //BOOST_LOG_TRIVIAL(error) << "Error creating pmem file " << abs_path << ": " << strerror(errno);
        throw std::runtime_error("");
    }

    /* check that the range allocated is of the required size */
    if(mapping_length != aligned_size){
        //BOOST_LOG_TRIVIAL(error) << "File mapping of different size than requested";
        pmem_unmap(pmem_addr, mapping_length);
        throw std::runtime_error("");
    }

    /* initialize members */
    m_name = mp_name;
    m_data = pmem_addr;
    m_offset = 0;
    m_size = mapping_length;
    m_bytes = 0; // will be filled by populate()
    m_is_pmem = is_pmem;
    // m_copies is default initialized as empty
    m_pmutex = std::unique_ptr<std::mutex>(new std::mutex());
}

mapping::~mapping() {

//    std::cerr << "Died!\n";

    if(m_data != 0){
        if(pmem_unmap(m_data, m_size) == -1) {
            //BOOST_LOG_TRIVIAL(error) << "Error while deleting mapping";
        }
//        else{
//            std::cerr << "Mapping released :P\n";
//        }
    }
}

void mapping::populate(const posix::file& fdesc) {
    m_bytes = m_is_pmem ? copy_data_to_pmem(fdesc) : copy_data_to_non_pmem(fdesc);
}

/** generate a name for an NVML pool using a prefix and a base_path */
bfs::path mapping::generate_pool_path(const bfs::path& prefix, const bfs::path& base_path) const {

    static boost::random::mt19937 rng;

    boost::uuids::uuid uuid = boost::uuids::random_generator(rng)();
    bfs::path ppath(boost::uuids::to_string(uuid));

    return (base_path / prefix).string() + "-" + ppath.string();
}

ssize_t mapping::copy_data_to_pmem(const posix::file& fdesc){

    char* addr = (char*) m_data;
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

ssize_t mapping::copy_data_to_non_pmem(const posix::file& fdesc){

    char* addr = (char*) m_data;
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




} // namespace nvml
} // namespace efsng


#ifdef __EFS_DEBUG__

std::ostream& operator<<(std::ostream& os, const efsng::nvml::mapping& mp) {
    os << "mapping {" << "\n"
       << "  m_name: " << mp.m_name << "\n"
       << "  m_data: " << mp.m_data << "\n"
       << "  m_offset: " << mp.m_offset << "\n"
       << "  m_size: " << mp.m_size << "\n"
       << "  m_bytes: " << mp.m_bytes << "\n"
       << "  m_is_pmem: " << mp.m_is_pmem << "\n"
       << "  m_copies.size(): " << mp.m_copies.size() << "\n"
       << "  m_pmutex: " << mp.m_pmutex.get() << "\n"
       << "};" << "\n";

    return os;
}

#endif /* __EFS_DEBUG__ */
