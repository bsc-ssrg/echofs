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


#include "catch.hpp"

#include <iostream>

#include <efs-common.h>
#include <backends/nvram-nvml/write-log.h>

SCENARIO("construction of entries", "[wlog]"){

    GIVEN("a base offset and a size") {
        WHEN("offset = 0 AND size < FUSE_BLOCK_SIZE") {

            size_t sz = std::rand()%efsng::FUSE_BLOCK_SIZE-1;

            efsng::nvml::wlog::entry e(0, sz);

            THEN("m_num_blocks equals 1"){
                REQUIRE(e.m_wr_offset == 0);
                REQUIRE(e.m_wr_size == sz);
                REQUIRE(e.m_num_blocks == 1);
            }
        }

        WHEN("offset = 0 AND size == FUSE_BLOCK_SIZE") {

            size_t sz = efsng::FUSE_BLOCK_SIZE;

            efsng::nvml::wlog::entry e(0, sz);

            THEN("m_num_blocks equals 1"){
                REQUIRE(e.m_wr_offset == 0);
                REQUIRE(e.m_wr_size == sz);
                REQUIRE(e.m_num_blocks == 1);
            }
        }

        WHEN("offset = 0 AND size > FUSE_BLOCK_SIZE") {

            size_t sz = efsng::FUSE_BLOCK_SIZE + std::rand();

            efsng::nvml::wlog::entry e(0, sz);

            int nblocks = (sz / efsng::FUSE_BLOCK_SIZE) + 1;

            THEN("m_num_blocks equals (size / FUSE_BLOCK_SIZE) + 1"){
                REQUIRE(e.m_wr_offset == 0);
                REQUIRE(e.m_wr_size == sz);
                REQUIRE(e.m_num_blocks == nblocks);
            }
        }

        WHEN("offset != 0 AND size < FUSE_BLOCK_SIZE") {

            off_t off = std::rand();
            off_t aligned_delta = off % efsng::FUSE_BLOCK_SIZE;
            size_t sz = std::rand()%(efsng::FUSE_BLOCK_SIZE - aligned_delta - 1);

            efsng::nvml::wlog::entry e(off, sz);

            int nblocks = (sz / efsng::FUSE_BLOCK_SIZE) + 1;

            THEN("m_num_blocks equals 1"){
                REQUIRE(efsng::FUSE_BLOCK_SIZE == efsng::FUSE_BLOCK_SIZE);
                REQUIRE(e.m_wr_offset == off);
                REQUIRE(e.m_wr_size == sz);
                REQUIRE(e.m_num_blocks == 1);
            }
        }

        WHEN("offset != 0 AND size == FUSE_BLOCK_SIZE") {

            size_t off = std::rand();
            size_t sz = efsng::FUSE_BLOCK_SIZE;

            efsng::nvml::wlog::entry e(off, sz);

            THEN("m_num_blocks equals 2"){
                REQUIRE(e.m_wr_offset == off);
                REQUIRE(e.m_wr_size == sz);
                REQUIRE(e.m_num_blocks == 2);
            }
        }

        WHEN("offset != 0 AND size > FUSE_BLOCK_SIZE") {

            off_t off = std::rand();
            size_t sz = efsng::FUSE_BLOCK_SIZE + std::rand();

            efsng::nvml::wlog::entry e(off, sz);

            int nblocks = (sz / efsng::FUSE_BLOCK_SIZE) + 1;

            THEN("m_num_blocks equals (size / FUSE_BLOCK_SIZE) + 1"){
                REQUIRE(e.m_wr_offset == off);
                REQUIRE(e.m_wr_size == sz);
                REQUIRE(e.m_num_blocks == nblocks);
            }
        }

    }
}

SCENARIO("construction of write log", "[wlog]"){

    GIVEN("several offsets and sizes") {
        WHEN("offset = 0 AND size < FUSE_BLOCK_SIZE") {

            size_t sz = std::rand()%efsng::FUSE_BLOCK_SIZE-1;

            efsng::nvml::wlog::log log;

            log.emplace_back(24, 46);
            log.emplace_back(24, 46);
            log.emplace_back(24, 46);
            log.emplace_back(24, 46);

            THEN("m_num_blocks equals 1"){
            }
        }
    }
}
