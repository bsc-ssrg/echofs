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

#ifndef __RANGE_LOCK_HPP__
#define __RANGE_LOCK_HPP__

#include <memory>
#include <condition_variable>
#include <sys/types.h>
#include "avl.hpp"

namespace efsng { 

/*! This class contains the code to implement POSIX file range locking, which
 * is directly inspired by the ZFS range locking implementation but adapted
 * for C++ and our particular semantics.
 *
 * The interface is as follows:
 *
 * lock_manager lm;\n
 * range_lock rl = lm.lock_range(start, end, lock_type);\n
 * lm.unlock_range(rl);
 *
 * An AVL tree is used to maintain the state of the existing ranges that are 
 * locked for exclusive (lock_manager::type::writer) or shared 
 * (lock_manager::type::reader) use. Note that the starting offset of a range 
 * acts as a key for sorting and searching the AVL tree.
 *
 */
struct lock_manager {

    // forward declaration
    struct range;

    /*! Typedef for convenience */
    using range_tree_t = avl::avltree<range, std::function<void(void*)>>;

    /*! Available lock types */
    enum class type { 
        reader, /*!< Lock range for a reading operation (i.e. shared) */
        writer  /*!< Lock a range for a writing operation (i.e. exclusive) */
    };

    /*! Range descriptor */
    struct range {

        /*! Types of ranges for internal use */
        enum class type { 
            none,           /*!< Range not initialized */
            reader,         /*!< Reader range */
            proxy_reader,   /*!< Proxy reader range */
            writer,         /*!< Writer range */
            any             /*!< Undetermined (useful for lookups) */
        };

        /*! Construct a range of undetermined type
         * @param start range's start offset
         * @param end range's end offset
         * @param type range's type
         **/
        range(off_t start, off_t end) 
         : m_start(start),
           m_end(end),
           m_type(type::any),
           m_refs(1),
           m_is_proxy(false),
           m_read_wanted(false),
           m_write_wanted(false),
           m_rd_cv(nullptr),
           m_wr_cv(nullptr) { }

        /*! Construct a range of a desired type 
         * @param start range's start offset
         * @param end range's end offset
         * @param type range's type
         **/
        range(off_t start, off_t end, range::type type)
         : m_start(start),
           m_end(end),
           m_type( type == range::type::proxy_reader ? range::type::reader : type ),
           m_refs(1),
           m_is_proxy(type == range::type::proxy_reader),
           m_read_wanted(false),
           m_write_wanted(false),
           m_rd_cv(new std::condition_variable()),
           m_wr_cv(new std::condition_variable()) { }

        /*! Compare two ranges
         * @param other target range to compare againts
         * @return -1 if this < other; 0 if this == other; +1 if this > other
         */
        int compare(const range& other) const {
            return (m_start > other.m_start) - (m_start < other.m_start);
        }

#ifdef RL_DEBUG
        std::string id() const {
            return std::to_string(m_start);
        }

        friend std::ostream& operator<<(std::ostream& os, const range& rl);
#endif

        using condvar_t = std::shared_ptr<std::condition_variable>;

        off_t       m_start;        /*!< File range start offset */
        off_t       m_end;          /*!< File range end offset */
        range::type m_type;         /*!< Range type */
        int         m_refs;         /*!< Reference count in tree */
        bool        m_is_proxy;     /*!< Is this range acting as a proxy? */
        bool        m_read_wanted;  /*!< Are there readers (potentially) waiting? */
        bool        m_write_wanted; /*!< Are there writers (potentially) waiting? */
        condvar_t   m_rd_cv;        /*!< Condition var for waiting readers */
        condvar_t   m_wr_cv;        /*!< Condition var for waiting writers */
    };

    /*! Range lock handle returned to the user */
    struct range_lock {
        ~range_lock() {
            //XXX this could surely be replaced by a shared_ptr
            // but we would need to fully rework the AVL tree.
            // maybe in the future :(
            if(m_ptr != nullptr && m_ptr->m_refs == 0) {
                // node removed from tree but still alive
                delete range_tree_t::data_to_node(m_ptr);
                m_ptr = nullptr;
            }
        }

        off_t   m_start;    /*!< File range start offset */
        off_t   m_end;      /*!< File range end offset */  
        type    m_type;     /*!< Range type */             
        range*  m_ptr;      /*!< Pointer to allocated range descriptor */
    };

    /*! Constructor */
    lock_manager();

    /*! Lock a range (start, end) as either shared (reader) or exclusive (writer). 
     * @param start range lock's start offset
     * @param end range lock's end offset
     * @param type the range lock's type
     * @return Lock handle for the new range lock */
    lock_manager::range_lock lock_range(off_t start, off_t end, lock_manager::type type);

    /*! Unlock the range described by a range lock handle
     * @param rl valid handle descriptor returned by a 
     * previous lock_range() call*/
    void unlock_range(range_lock& rl);

    std::mutex      m_mutex;    /*!< Protect changes to range_tree */
    range_tree_t    m_tree;     /*!< Tree of range locks */
};

} // namespace efsng

#endif /* __RANGE_LOCK_HPP__ */
