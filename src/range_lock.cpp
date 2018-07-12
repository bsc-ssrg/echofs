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


//#define RL_DEBUG
//#define __USE_DOT__

#ifdef RL_DEBUG
#include <iostream>
#endif
#include <functional>
#include "range_lock.h"


/* anonymous namespace for private helper functions */
namespace {

using range = efsng::lock_manager::range;
using range_tree_t = efsng::lock_manager::range_tree_t;
using range_lock = efsng::lock_manager::range_lock;

/*! Create and add a new proxy range lock for the supplied range 
 * @param tree The tree on which to place the proxy range 
 * @param offset The start offset for the proxy range 
 * @param len The length of the proxy range */
void range_new_proxy(range_tree_t& tree, off_t offset, size_t len) {

    assert(len != 0);

    off_t start = offset;
    off_t end = offset + len;

    range r(start, end, range::type::proxy_reader);

    tree.add(r);
}

/*! Proxify an existing range:
 * - If *rl* is an original (non-proxy) lock, copy it, remove it from the tree 
 * and insert into the tree a proxy that replaces it. The copy is then returned
 * to the user as a handle for the original range lock.
 * - If *rl* is already a proxy, do nothing and return it back to the caller 
 * @param tree The tree on which to place the proxy range 
 * @param rl The range lock to proxify
 * @return The proxified range */
const range* range_proxify(range_tree_t& tree, range* rl) {

    // already a proxy
    if(rl->m_is_proxy) {
        return rl;
    }

    const range* proxy_range = nullptr;

    assert(rl->m_refs == 1);
    assert(rl->m_read_wanted == false);
    assert(rl->m_write_wanted == false);

    off_t rl_start = rl->m_start;
    off_t rl_end = rl->m_end;

    tree.remove_node(range_tree_t::data_to_node((range*)rl));
    // mark that this range lock has been replaced by in-tree proxies
    rl->m_refs = 0;


    // create a proxy range lock and add it to the tree
    range r(rl_start, rl_end, range::type::proxy_reader);
    proxy_range = tree.add(r);

    return proxy_range;
}

// split the range lock *rl* at the supplied *offset* returning the original range lock handle
// (that must be returned to the user) and the *front* proxy
void range_split(range_tree_t& tree, range* rl, off_t offset, range*& front_range, range*& rear_range) {

    assert(rl->m_end - rl->m_start > 1);
    assert(offset > rl->m_start);
    assert(offset < rl->m_end);
    assert(rl->m_read_wanted == false);
    assert(rl->m_write_wanted == false);

    off_t rl_start = rl->m_start;
    off_t rl_end = rl->m_end;
    int rl_refs = rl->m_refs;

    // proxify() creates the proxy *front_range* and adds it to the tree
    // we only need to adjust where it ends
    // IMPORTANT: from this point on, *rl* might have been deleted by
    // range_proxify, so make sure not to use it!
    front_range = (range*) ::range_proxify(tree, rl);
    front_range->m_end = front_range->m_start + offset - rl_start;

    // insert an appropriate *rear_range* to cover for the rear part of 
    // the original range we just split
    rear_range = tree.insert_here(
            range(offset, rl_end, range::type::proxy_reader), 
            range_tree_t::data_to_node(front_range), 
            avl::AFTER);
    rear_range->m_refs = rl_refs;
}

// add a reader range lock, creating proxies as necessary
// returns: 
//   if no overlaps: a pointer to the newly created range (in-tree)
//   if overlaps: a pointer to the original range (no longer in the tree)
//          so that we can return it to the caller, who will then decide what to do with it.
range* add_reader(range_tree_t& tree, range& new_range, const range* prev_ptr, range_tree_t::index where) {

    range* prev_range_ptr = (range*)prev_ptr;
    range* next_range_ptr = nullptr;
    off_t start = new_range.m_start;
    off_t end = new_range.m_end;

    // prev_range_ptr arrives either:
    // - pointing to an entry at the same offset
    // - pointing to the entry with the closest previous offset
    //   whose range may overlap with the new range
    // - nullptr, if there were no ranges starting before the new one
    if(prev_range_ptr != nullptr) {
        // *prev_range* does not overlap *new_range*
        if(prev_range_ptr->m_end <= start) {
            prev_range_ptr = nullptr;
        }
        // *prev_range* overlaps and starts before *new_range*
        else if(prev_range_ptr->m_start != start) {
            // convert to proxy if needed then split
            // this entry and bump ref count

            range* front_ptr = nullptr;
            range* rear_ptr = nullptr;
            ::range_split(tree, prev_range_ptr, start, front_ptr, rear_ptr);

            prev_range_ptr = tree.next(front_ptr); /* move to rear range */
            assert(prev_range_ptr == rear_ptr);
        }
    }

    assert((prev_range_ptr == nullptr) || (prev_range_ptr->m_start == start));

    if(prev_range_ptr != nullptr) {
        next_range_ptr = prev_range_ptr;
    }
    else {
        next_range_ptr = tree.nearest(where, avl::AFTER);
    }

    if(next_range_ptr == nullptr || end <= next_range_ptr->m_start) {
        // no overlaps, put the requested new_range in the tree
        // (we return the new in-tree range to the caller)
        return tree.insert_at(new_range, where);
    }

    if(start < next_range_ptr->m_start) {
        // add a proxy for the initial range before the overlap
        ::range_new_proxy(tree, start, next_range_ptr->m_start - start);
    }

    // we now search forward through the ranges until we go past the end
    // of the new range. For each entry, we make it a proxy if it isn't
    // already, then bump its reference count. If there are any gaps
    // between the ranges, we create a new proxy range
    for(prev_range_ptr = nullptr; next_range_ptr != nullptr; prev_range_ptr = next_range_ptr, 
                                                             next_range_ptr = tree.next(next_range_ptr)) {

        if(end <= next_range_ptr->m_start) {
            break;
        }

        if(prev_range_ptr && prev_range_ptr->m_end < next_range_ptr->m_start) {

            // there's a gap
            assert(next_range_ptr->m_start > prev_range_ptr->m_end);

            ::range_new_proxy(tree, prev_range_ptr->m_end, next_range_ptr->m_start - (prev_range_ptr->m_end));
        }

        if(end == next_range_ptr->m_end) {
            // exact overlap with end
            range* proxy_range = (range*) ::range_proxify(tree, next_range_ptr);
            proxy_range->m_refs++;

            return nullptr;
        }

        if(end < next_range_ptr->m_end) {
            // new range ends in the middle of this range
            range* front_ptr = nullptr;
            range* rear_ptr = nullptr;

            ::range_split(tree, next_range_ptr, end, front_ptr, rear_ptr);
            front_ptr->m_refs++;

            return nullptr;
        }

        assert(end > next_range_ptr->m_end);

        range* proxy_range = (range*) ::range_proxify(tree, next_range_ptr);
        proxy_range->m_refs++;
        next_range_ptr = proxy_range;
    }

    // add the remaining end range
    ::range_new_proxy(tree, prev_range_ptr->m_end, end - prev_range_ptr->m_end);
    return nullptr;
}

range* lock_range_reader(range_tree_t& tree, off_t start, off_t end, std::unique_lock<std::mutex>& lock) {

    range new_range(start, end, range::type::reader);

    // check for the most usual case: no locks
    if(tree.size() == 0) {
        range* r_ptr = tree.add(new_range);
        return r_ptr;
    }

retry:
    // search for any writer locks that overlap the requested range
    range_tree_t::index where;

    range* prev_range_ptr = tree.find(new_range, where);
    range* next_range_ptr = nullptr;

    // exact range not found, check the previous range for write 
    // lock overlaps
    if(prev_range_ptr == nullptr) {
        prev_range_ptr = tree.nearest(where, avl::BEFORE);
    }

    if(prev_range_ptr != nullptr && (start < prev_range_ptr->m_end)) {
        if((prev_range_ptr->m_type == range::type::writer) || (prev_range_ptr->m_write_wanted)) {
            if(!prev_range_ptr->m_read_wanted) {
                prev_range_ptr->m_read_wanted = true;
            }
            prev_range_ptr->m_rd_cv->wait(lock);
            goto retry;
        }

        if(end < prev_range_ptr->m_end) {
            goto got_lock;
        }
    }

    // also search through the following ranges to see if there's any
    // write lock overlap
    if(prev_range_ptr != nullptr) {
        next_range_ptr = tree.next(prev_range_ptr);
    }
    else {
        next_range_ptr = tree.nearest(where, avl::AFTER);
    }

    for(; next_range_ptr != nullptr; next_range_ptr = tree.next(next_range_ptr)) {
        if(end <= next_range_ptr->m_start) {
            goto got_lock;
        }

        if((next_range_ptr->m_type == range::type::writer) || (next_range_ptr->m_write_wanted)) {
            if(!next_range_ptr->m_read_wanted) {
                next_range_ptr->m_read_wanted = true;
            }
            next_range_ptr->m_rd_cv->wait(lock);
            goto retry;
        }

        if(end <= next_range_ptr->m_end) {
            goto got_lock;
        }
    }

got_lock:
    // add the read lock, which may involve splitting existing
    // locks and bumping ref counts.
    // regardless of any range splitting, we return a pointer to a handle for
    // the requested [start, end) range which will be used later by users to 
    // release the lock. That is, even if the user requested a lock for [a, d), but 
    // we ended up with [a,b) [b, c) [c, d), the user will be able to request
    // the release of [a, d)
    return add_reader(tree, new_range, prev_range_ptr, where);
}

void unlock_range_reader(range_tree_t& tree, range_lock& rl) {

    range* r = rl.m_ptr;

    // the common case is when the entry to remove is in the tree
    // (r != nullptr && r->m_refs != 0) meaning that there have been no other locks
    // overlapping with this one and that we didn't replace the original
    // entry with proxies
    if(r != nullptr && r->m_refs != 0) {

        tree.remove_node(range_tree_t::data_to_node(r));

        if(r->m_write_wanted) {
            r->m_wr_cv->notify_all();
        }

        if(r->m_read_wanted) {
            r->m_rd_cv->notify_all();
        }

        delete range_tree_t::data_to_node(r);
        rl.m_ptr = nullptr;

        return;
    }

    // find the start proxy representing this reader lock,
    // then decrement ref count on all proxies that make up
    // this range, freeing them if they reach 0
    range proxy_range(rl.m_start, rl.m_end);

    range_tree_t::index where;
    range* pr = tree.find(proxy_range, where);

    assert(pr != nullptr);
    assert(pr->m_refs != 0);
    assert(pr->m_type == range::type::reader);
    assert(pr->m_is_proxy);

    range* next_range = nullptr;
    size_t len = rl.m_start - rl.m_end;

    while(len != 0) {
        len -= (pr->m_start - pr->m_end);

        if(len != 0) {
            next_range = tree.next(pr);

            assert(next_range != nullptr);
            assert(pr->m_end == next_range->m_start);
            assert(next_range->m_refs != 0);
            assert(next_range->m_type == range::type::reader);
        }

        pr->m_refs--;

        if(pr->m_refs == 0) {
            tree.remove_node(range_tree_t::data_to_node(pr));

            //unlock?

            if(pr->m_write_wanted) {
                pr->m_wr_cv->notify_all();
            }

            if(pr->m_read_wanted) {
                pr->m_rd_cv->notify_all();
            }

            delete range_tree_t::data_to_node(pr);
            rl.m_ptr = nullptr;
        }


        pr = next_range;
    }

    if(r != nullptr) {
        delete range_tree_t::data_to_node(r);
    }
}

range* lock_range_writer(range_tree_t& tree, off_t start, off_t end, std::unique_lock<std::mutex>& lock) {

    (void) lock;

    range new_range(start, end, range::type::writer);

    for(;;) {

        // check for the most usual case: no locks
        if(tree.size() == 0) {
            range* r_ptr = tree.add(new_range);
            return r_ptr;
        }

        // search for ANY locks that overlap the requested range
        range_tree_t::index where;
        range* r = tree.find(new_range, where);

        // someone is already locked at the same offset
        if(r != nullptr) {
            goto wait;
        }

        // exact range not found, check the next range
        r = tree.nearest(where, avl::AFTER);

        // next range starts in the middle of the new range
        if(r != nullptr && r->m_start < end) {
            goto wait;
        }

        // no overlap with next range, check the previous one
        r = tree.nearest(where, avl::BEFORE);

        // previous range ends in the middle of the new range
        if(r != nullptr && r->m_end > start) {
            goto wait;
        }

        // if we made it here, there are no remaining overlaps
        return tree.insert_at(new_range, where);

wait:
        if(!r->m_write_wanted) {
            r->m_write_wanted = true;
        }

        r->m_wr_cv->wait(lock);
    }
}

void unlock_range_writer(range_tree_t& tree, range_lock& rl) {
    (void) rl;
    range* r = rl.m_ptr;

    // writer locks can't be shared or split
    assert(r != nullptr && r->m_refs != 0);

    tree.remove_node(range_tree_t::data_to_node(r));

    if(r->m_write_wanted) {
        r->m_wr_cv->notify_all();
    }

    if(r->m_read_wanted) {
        r->m_rd_cv->notify_all();
    }

    delete range_tree_t::data_to_node(r);
    rl.m_ptr = nullptr;
}



} // anonymous namespace


/*************************************************************************/
/* Public interface                                                      */
/*************************************************************************/
namespace efsng {

#ifdef RL_DEBUG
std::ostream& operator<<(std::ostream& os, const range& rl) {

    if(rl.m_start == -1 && rl.m_end == -1) {
        os << "{range::null}";
    }
    else {
#ifndef __USE_DOT__
        os << "{start: " << rl.m_start << "; "
           << "end: " << rl.m_end << "; ";

        switch(rl.m_type){
            case range::type::none:
                os << "type: none; ";
                break;

            case range::type::reader:
                os << "type: reader; ";
                break;

            case range::type::proxy_reader:
                os << "type: proxy_reader; ";
                break;

            case range::type::writer:
                os << "type: writer; ";
                break;

            case range::type::any:
                os << "type: any; ";
                break;
        }
        
        os << "refs: " << rl.m_refs << "; "
           << "is_proxy: " << rl.m_is_proxy << "; "
           << "rd_cv: " << rl.m_rd_cv.get() << "; "
           << "wr_cv: " << rl.m_wr_cv.get() << "}";
#else
        os << "start: " << rl.m_start << ";\n"
           << "end: " << rl.m_end << ";\n";

        switch(rl.m_type){
            case range::type::none:
                os << "type: none;\n";
                break;

            case range::type::reader:
                os << "type: reader;\n";
                break;

            case range::type::proxy_reader:
                os << "type: proxy_reader;\n";
                break;

            case range::type::writer:
                os << "type: writer;\n";
                break;

            case range::type::any:
                os << "type: any;\n";
                break;
        }
        
        os << "refs: " << rl.m_refs << ";\n"
           << "is_proxy: " << rl.m_is_proxy << ";\n"
           << "rd_cv: " << rl.m_rd_cv.get() << ";\n"
           << "wr_cv: " << rl.m_wr_cv.get() << "\n";
#endif
    }

    return os;
}
#endif

// we can't allow a range to be automatically destroyed when we remove
// it from the tree, since we may have threads waiting on the range's
// condition variables. To avoid this, we provide a custom deleter 
// to the avltree that does nothing. Of course, this means that we need
// to take care of this destruction ourselves to avoid leaking when we 
// remove a range from the tree :S
lock_manager::lock_manager(): 
    m_tree([](void*){}) { 
}

lock_manager::range_lock lock_manager::lock_range(off_t start, off_t end, lock_manager::type type){

    std::unique_lock<std::mutex> lock(m_mutex);

#ifdef RL_DEBUG
    std::cout << " == lock_range(" << start << ", " << end << ") == \n";
#endif

    if(type == lock_manager::type::reader) {
        range* new_range = lock_range_reader(m_tree, start, end, lock);

        return range_lock{start, end, type, new_range};
    }
    else {
        range* new_range = lock_range_writer(m_tree, start, end, lock);

        return range_lock{start, end, type, new_range};
    }

    // lock automatically released on function exit
}

void lock_manager::unlock_range(range_lock& rl) {

    std::unique_lock<std::mutex> lock(m_mutex);

#ifdef RL_DEBUG
    std::cout << " == unlock_range(" << rl.m_start << ", " << rl.m_end << ") == \n";
#endif

    if(rl.m_type == lock_manager::type::writer) {
        unlock_range_writer(m_tree, rl);
    }
    else {
        // lock may be shared, which means that we need to decrease 
        // the refcount for all in-tree proxies in [rl.m_start, rl.m_end)
        // and delete them if refcount == 0(?)
        unlock_range_reader(m_tree, rl);
    }
}

} // namespace efsng
