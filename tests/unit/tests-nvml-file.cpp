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
#include <stdint.h>

#include <efs-common.h>
#include <backends/nvram-nvml/file.h>

SCENARIO("file creation", "[nvml::file]"){
    GIVEN("a file with one mapping and a capacity <= FUSE_BLOCK_SIZE") {
        WHEN("mapping starts at offset 0 and has a requested size of FUSE_BLOCK_SIZE") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            THEN("mappings offset is 0 and size is FUSE_BLOCK_SIZE") {
                REQUIRE(f.m_mappings.front().m_offset == 0);
                REQUIRE(f.m_mappings.front().m_size == efsng::FUSE_BLOCK_SIZE);
            }
        }
    }

    GIVEN("a file with one mapping and a requested size of 1.5*FUSE_BLOCK_SIZE") {
        WHEN("mapping starts at offset 0 and has a capacity of 2*FUSE_BLOCK_SIZE") {

            size_t min_size0 = 1.5*efsng::FUSE_BLOCK_SIZE;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            THEN("mappings offset is 0 and sizes is FUSE_BLOCK_SIZE") {

                size_t mapping_size0 = efsng::FUSE_BLOCK_SIZE*(min_size0 / efsng::FUSE_BLOCK_SIZE + 1);
            
                REQUIRE(f.m_mappings.front().m_offset == 0);
                REQUIRE(f.m_mappings.back().m_size == mapping_size0);
            }
        }
    }

    GIVEN("a file with two mappings") {
        WHEN("several mappings are added to a file") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            f.add(std::move(mp2));

            THEN("mappings offsets are consecutive") {

                size_t mapping_size0 = efsng::FUSE_BLOCK_SIZE*(min_size0 / efsng::FUSE_BLOCK_SIZE + 1);
                size_t mapping_size1 = efsng::FUSE_BLOCK_SIZE*(min_size1 / efsng::FUSE_BLOCK_SIZE + 1);
            
                REQUIRE(f.m_mappings.front().m_offset == 0);
                REQUIRE(f.m_mappings.back().m_offset == 0+mapping_size0);

                REQUIRE(f.m_mappings.front().m_size == mapping_size0);
                REQUIRE(f.m_mappings.back().m_size == mapping_size1);
            }
        }
    }
}

SCENARIO("offset range lookup", "[nvml::file]"){
    GIVEN("a file with 1 mappings") {
        WHEN("range starts at offset 0") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            off_t range_start = 0;
            off_t range_end = 0 + min_size0/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == f.m_mappings.front().m_data);
                REQUIRE(bmap.front().m_size == min_size0/2);
            }
        }

        WHEN("range starts at offset != 0") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            off_t range_start = 100;
            off_t range_end = 100 + min_size0/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset + a delta") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t) f.m_mappings.front().m_data + range_start));
                REQUIRE(bmap.front().m_size == min_size0/2);
            }
        }

        WHEN("range starts at offset 0 and mapping has less data than requested") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            off_t range_start = 0;
            off_t range_end = 0 + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == f.m_mappings.front().m_data);
                REQUIRE(bmap.front().m_size == 10000);
            }
        }

        WHEN("range starts at offset != 0 and mapping has less data than requested") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            size_t delta = 100;
            off_t range_start = 0 + delta;
            off_t range_end = 0 + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset + a delta") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t) f.m_mappings.front().m_data + delta));
                REQUIRE(bmap.front().m_size == 10000 - delta);
            }
        }

        WHEN("range starts at offset 0 and goes beyond the mapping (full data)") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            off_t range_start = 0;
            off_t range_end = efsng::FUSE_BLOCK_SIZE + 10000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == f.m_mappings.front().m_data);
                REQUIRE(bmap.front().m_size == efsng::FUSE_BLOCK_SIZE);
            }
        }

        WHEN("range starts at offset != 0 and goes beyond the mapping (full data)") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            size_t delta = 100;
            off_t range_start = 0 + delta;
            off_t range_end = efsng::FUSE_BLOCK_SIZE + 10000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset + a delta") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t) f.m_mappings.front().m_data + delta));
                REQUIRE(bmap.front().m_size == efsng::FUSE_BLOCK_SIZE - delta);
            }
        }

        WHEN("range starts at offset 0 and goes beyond the full mapping (partial data)") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            off_t range_start = 0;
            off_t range_end = efsng::FUSE_BLOCK_SIZE + 10000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == f.m_mappings.front().m_data);
                REQUIRE(bmap.front().m_size == 10000);
            }
        }

        WHEN("range starts at offset != 0 and goes beyond the mapping (partial data)") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            size_t delta = 100;
            off_t range_start = 0 + delta;
            off_t range_end = efsng::FUSE_BLOCK_SIZE + 10000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset + a delta") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t) f.m_mappings.front().m_data + delta));
                REQUIRE(bmap.front().m_size == 10000 - delta);
            }
        }

        WHEN("range starts beyond the mapping's available bytes") {

            size_t min_size0 = 10;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);

            size_t delta = 15000;
            off_t range_start = 0 + delta;
            off_t range_end = efsng::FUSE_BLOCK_SIZE + 10000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("no buffers returned") {
                REQUIRE(bmap.size() == 0);
            }
        }
    }
    GIVEN("a file with 2 mappings") {
        WHEN("range starts at 2nd mapping's origin") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = mp2_offset;
            off_t range_end = mp2_offset + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == mp2_data);
                REQUIRE(bmap.front().m_size == 10000);
            }
        }

        WHEN("range starts at offset > 2nd mapping's base offset") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = 100;
            off_t range_start = mp2_offset + delta;
            off_t range_end = mp2_offset + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t)mp2_data + delta));
                REQUIRE(bmap.front().m_size == 10000 - delta);
            }
        }

        WHEN("range starts at 2nd mapping's base offset and mapping has less data than requested") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = 10000;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));


            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = mp2_offset;
            off_t range_end = mp2_offset + mp2_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == mp2_data);
                REQUIRE(bmap.front().m_size == 10000);
            }
        }

        WHEN("range starts at offset > 2nd mapping's base offset and mapping has less data than requested") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = 10000;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = 100;
            off_t range_start = mp2_offset + delta;
            off_t range_end = mp2_offset + mp2_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t)mp2_data + delta));
                REQUIRE(bmap.front().m_size == 10000 - delta);
            }
        }

        WHEN("range starts at 2nd mapping's base offset and goes beyond the mapping (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));


            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = mp2_offset;
            off_t range_end = mp2_offset + efsng::FUSE_BLOCK_SIZE + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == mp2_data);
                REQUIRE(bmap.front().m_size == efsng::FUSE_BLOCK_SIZE);
            }
        }

        WHEN("range starts at offset > 2nd mapping's base offset and goes beyond the mapping (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = 100;
            off_t range_start = mp2_offset + delta;
            off_t range_end = mp2_offset + efsng::FUSE_BLOCK_SIZE + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t)mp2_data + delta));
                REQUIRE(bmap.front().m_size == efsng::FUSE_BLOCK_SIZE - delta);
            }
        }

        WHEN("range starts at 2nd mapping's base offset and goes beyond the mapping (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));


            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = mp2_offset;
            off_t range_end = mp2_offset + efsng::FUSE_BLOCK_SIZE + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == mp2_data);
                REQUIRE(bmap.front().m_size == 10000);
            }
        }

        WHEN("range starts at offset > 2nd mapping's base offset and goes beyond the mapping (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = 100;
            off_t range_start = mp2_offset + delta;
            off_t range_end = mp2_offset + efsng::FUSE_BLOCK_SIZE + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t)mp2_data + delta));
                REQUIRE(bmap.front().m_size == 10000 - delta);
            }
        }

        /* range_lookups spanning two mappings */
        WHEN("range starts at 1st mapping's origin and ends at 2nd mapping's end (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = 0;
            off_t range_end = mp2_offset + mp2_size;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size + mp2_size);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {mp1_data, mp1_size},
                    {mp2_data, mp2_size}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts at 1st mapping's origin and ends at 2nd mapping's end (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = 0;
            off_t range_end = mp2_offset + mp2_size;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size + 10000);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {mp1_data, mp1_size},
                    {mp2_data, 10000}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts at 1st mapping's origin and ends mid-2nd mapping (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = 0;
            off_t range_end = mp2_offset + mp2_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size + mp2_size/2);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {mp1_data, mp1_size},
                    {mp2_data, mp2_size/2}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts at 1st mapping's origin and ends mid-2nd mapping (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            off_t range_start = 0;
            off_t range_end = mp2_offset + mp2_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size + 10000);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {mp1_data, mp1_size},
                    {mp2_data, 10000}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts mid-1st mapping and ends at 2nd mapping's end (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = mp1_size/2;
            off_t range_start = 0 + delta;
            off_t range_end = mp2_offset + mp2_size;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size - delta + mp2_size);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp1_data + delta), mp1_size - delta},
                    {mp2_data, mp2_size}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts mid-1st mapping and ends at 2nd mapping's end (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = mp1_size/2;
            off_t range_start = 0 + delta;
            off_t range_end = mp2_offset + mp2_size;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size - delta + mp2_bytes);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp1_data + delta), mp1_size - delta},
                    {mp2_data, mp2_bytes}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }
//XXX
        WHEN("range starts mid-1st mapping and ends mid-2nd mapping (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = mp1_size/2;
            off_t range_start = 0 + delta;
            off_t range_end = mp2_offset + mp2_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size - delta + mp2_size/2);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp1_data + delta), mp1_size - delta},
                    {mp2_data, mp2_size/2}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts mid-1st mapping and ends mid-2nd mapping (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            size_t delta = mp1_size/2;
            off_t range_start = 0 + delta;
            off_t range_end = mp2_offset + mp2_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp1_size - delta + 10000);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp1_data + delta), mp1_size - delta},
                    {mp2_data, 10000}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }
    }

    GIVEN("a file with 3 mappings") {
        WHEN("range starts at 3rd mapping's origin") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 2);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            off_t range_start = mp3_offset;
            off_t range_end = mp3_offset + mp3_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == mp3_data);
                REQUIRE(bmap.front().m_size == mp3_size/2);
            }
        }

        WHEN("range starts at offset > 3rd mapping's origin") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 2);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            size_t delta = 100;
            off_t range_start = mp3_offset + delta;
            off_t range_end = mp3_offset + mp3_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t)mp3_data + delta));
                REQUIRE(bmap.front().m_size == mp3_size/2 - delta);
            }
        }

        WHEN("range starts at 3rd mapping's origin less data than requested") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 2);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            off_t range_start = mp3_offset;
            off_t range_end = mp3_offset + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == mp3_data);
                REQUIRE(bmap.front().m_size == 10000);
            }
        }

        WHEN("range starts at offset > 3rd mapping's base offset and mapping has less data than requested") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            std::advance(it, 2);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            size_t delta = 100;
            off_t range_start = mp3_offset + delta;
            off_t range_end = mp3_offset + 15000;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("one buffer is returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 1);
                REQUIRE(bmap.front().m_data == (void*)((uintptr_t)mp3_data + delta));
                REQUIRE(bmap.front().m_size == 10000 - delta);
            }
        }

        /* range_lookups spanning three mappings */
        WHEN("range starts mid-1st mapping and ends mid-3rd mapping (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            size_t delta = mp1_size/2;
            off_t range_start = 0 + delta;
            off_t range_end = mp3_offset + mp3_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 3);
                REQUIRE(bmap.m_size == mp1_size - delta + mp2_size + mp3_size/2);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp1_data + delta), mp1_size - delta},
                    {mp2_data, mp2_size},
                    {mp3_data, mp3_size/2}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts mid-1st mapping and ends mid-3rd mapping (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            size_t delta = mp1_size/2;
            off_t range_start = 0 + delta;
            off_t range_end = mp3_offset + mp3_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 3);
                REQUIRE(bmap.m_size == mp1_size - delta + mp2_size + 10000);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp1_data + delta), mp1_size - delta},
                    {mp2_data, mp2_size},
                    {mp3_data, 10000}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts mid-2nd mapping and ends mid-3rd mapping (full data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = efsng::FUSE_BLOCK_SIZE;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            size_t delta = mp2_size/2;
            off_t range_start = mp2_offset + delta;
            off_t range_end = mp3_offset + mp3_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp2_size - delta + mp3_size/2);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp2_data + delta), mp2_size - delta},
                    {mp3_data, mp3_size/2}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }

        WHEN("range starts mid-2nd mapping and ends mid-3rd mapping (partial data)") {

            size_t min_size0 = 10;
            size_t min_size1 = 20;
            size_t min_size2 = 30;

            // we don't want to keep the files after the test ends
            auto type = efsng::nvml::mapping::type::temporary;

            efsng::nvml::mapping mp("a", "/tmp/", min_size0, type);
            efsng::nvml::mapping mp2("b", "/tmp/", min_size1, type);
            efsng::nvml::mapping mp3("c", "/tmp/", min_size2, type);

            // fake the used size of the mappings given that we are not using real data
            // but we want them to behave as if we did
            mp.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp2.m_bytes = efsng::FUSE_BLOCK_SIZE;
            mp3.m_bytes = 10000;

            struct stat stbuf;
            efsng::nvml::file f("a", mp, &stbuf);
            f.add(std::move(mp2));
            f.add(std::move(mp3));

            auto it = f.m_mappings.begin();

            // fetch relevant data
            void* mp1_data = it->m_data;
            off_t mp1_offset = it->m_offset;
            size_t mp1_size = it->m_size;
            size_t mp1_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp2_data = it->m_data;
            off_t mp2_offset = it->m_offset;
            size_t mp2_size = it->m_size;
            size_t mp2_bytes = it->m_bytes;

            std::advance(it, 1);
            void* mp3_data = it->m_data;
            off_t mp3_offset = it->m_offset;
            size_t mp3_size = it->m_size;
            size_t mp3_bytes = it->m_bytes;

            size_t delta = mp2_size/2;
            off_t range_start = mp2_offset + delta;
            off_t range_end = mp3_offset + mp3_size/2;
            efsng::backend::buffer_map bmap;

            f.range_lookup(range_start, range_end, bmap);

            THEN("two buffers are returned with address equal to the mapping's base offset") {
                REQUIRE(bmap.size() == 2);
                REQUIRE(bmap.m_size == mp2_size - delta + 10000);

                int i = 0;
                std::vector<std::pair<void*, size_t>> exp = {
                    {(void*)((uintptr_t)mp2_data + delta), mp2_size - delta},
                    {mp3_data, 10000}
                };

                for(const auto& b : bmap){
                    REQUIRE(b.m_data == exp[i].first);
                    REQUIRE(b.m_size == exp[i].second);
                    ++i;
                }
            }
        }
    }
}
