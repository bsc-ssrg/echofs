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


#ifndef __EFS_COMMON_H__
#define __EFS_COMMON_H__

#include <cstdint>
#include <cassert>
#include <unistd.h>

inline int log2(uint64_t n){

    /* see http://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers */
    static const int table[64] = {
        0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61,
        51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62,
        57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
        45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5, 63 };

    assert(n > 0);

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    return table[(n * 0x03f6eaf2cd271461) >> 58];
}

namespace efsng {

using data_ptr_t = void *;

enum class operation {
    read,
    write
};

const uint64_t EFS_BLOCK_SIZE  = 0x000400000; // 4MiB
//const uint64_t FUSE_BLOCK_SIZE = 0x000400000; // 4MiB
const uint64_t FUSE_BLOCK_SIZE = 0x000001000; // 4MiB
//const uint64_t EFS_BLOCK_SIZE_BITS = log2(EFS_BLOCK_SIZE);
//const uint64_t FUSE_BLOCK_SIZE_BITS = log2(FUSE_BLOCK_SIZE);

inline off_t align(const off_t n, const size_t block_size) {
    return n & ~(block_size - 1);
}

inline off_t xalign(const off_t n, const size_t block_size) {
    return align(n + block_size, block_size);
}

inline int block_count(const off_t n, const size_t sz, const size_t block_size) {

    off_t block_start = align(n, block_size);
    off_t block_end = align(n+sz-1, block_size);

    return (block_end >> log2(block_size)) - 
           (block_start >> log2(block_size)) + 1;
}

} // namespace efsng

#endif /* __EFS_COMMON_H__ */
