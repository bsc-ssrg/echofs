#include "catch.hpp"

#include <backends/nvram-nvml/range_lock.h>
#include <backends/nvram-nvml/avl.hpp>

#include <thread>
#include <chrono>
#include <ostream>
#include <string.h>

#define MAXBUF 128

using lock_manager = efsng::nvml::lock_manager;
using range = lock_manager::range;

SCENARIO("basic range locking", "[lock_manager_basic]"){

    struct exp_range {
        off_t start;
        off_t end;
        range::type type;
        int refs;
        bool is_proxy;
    };

    auto verify_tree = [] (lock_manager::range_tree_t& tree, const exp_range expected[]) {

        int i = 0;
        auto r = tree.first();

        while(r) {
            REQUIRE(r->m_start == expected[i].start);
            REQUIRE(r->m_end == expected[i].end);
            REQUIRE(r->m_type == expected[i].type);
            REQUIRE(r->m_refs == expected[i].refs);
            REQUIRE(r->m_is_proxy == expected[i].is_proxy);

            r = tree.next(r);
            ++i;
        }
    };



    GIVEN("a sequence of reader range locks") {
        WHEN("a lock is added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);

            THEN("an appropriate range lock is constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 1);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(mgr.m_tree.size() == 1);
            }
        }

        // front overlap 
        WHEN("overlapping locks are added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
            auto rl1 = mgr.lock_range(8, 17, lock_manager::type::reader);

            //mgr.m_tree.display();

            THEN("appropriate range locks proxies are constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 0);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(rl1.m_ptr == nullptr);
                REQUIRE(rl1.m_start == 8);
                REQUIRE(rl1.m_end == 17);

                // there must be proxy locks for:
                // [8,10), [10,17), [17, 25)
                REQUIRE(mgr.m_tree.size() == 3);

                exp_range expected[3] = { 
                    {8,  10, range::type::reader, 1, true}, 
                    {10, 17, range::type::reader, 2, true}, 
                    {17, 25, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);
            }
        }

        // mid overlap 
        WHEN("overlapping locks are added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
            auto rl1 = mgr.lock_range(14, 22, lock_manager::type::reader);

            //mgr.m_tree.display();

            THEN("appropriate range locks proxies are constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 0);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(rl1.m_ptr == nullptr);
                REQUIRE(rl1.m_start == 14);
                REQUIRE(rl1.m_end == 22);

                // there must be proxy locks for:
                // [10,14), [14,22), [22,25)
                const int exp_sz = 3;
                REQUIRE(mgr.m_tree.size() == exp_sz);

                exp_range expected[exp_sz] = { 
                    {10, 14, range::type::reader, 1, true}, 
                    {14, 22, range::type::reader, 2, true}, 
                    {22, 25, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);
            }
        }

        // mid overlap 
        WHEN("overlapping locks are added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
            auto rl1 = mgr.lock_range(14, 22, lock_manager::type::reader);

            //mgr.m_tree.display();

            THEN("appropriate range locks proxies are constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 0);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(rl1.m_ptr == nullptr);
                REQUIRE(rl1.m_start == 14);
                REQUIRE(rl1.m_end == 22);

                // there must be proxy locks for:
                // [10,14), [14,22), [22,25)
                const int exp_sz = 3;
                REQUIRE(mgr.m_tree.size() == exp_sz);

                exp_range expected[exp_sz] = { 
                    {10, 14, range::type::reader, 1, true}, 
                    {14, 22, range::type::reader, 2, true}, 
                    {22, 25, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);
            }
        }

        // mid overlap 
        WHEN("overlapping locks are added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
            auto rl1 = mgr.lock_range(14, 22, lock_manager::type::reader);

            //mgr.m_tree.display();

            THEN("appropriate range locks proxies are constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 0);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(rl1.m_ptr == nullptr);
                REQUIRE(rl1.m_start == 14);
                REQUIRE(rl1.m_end == 22);

                // there must be proxy locks for:
                // [10,14), [14,22), [22,25)
                const int exp_sz = 3;
                REQUIRE(mgr.m_tree.size() == exp_sz);

                exp_range expected[exp_sz] = { 
                    {10, 14, range::type::reader, 1, true}, 
                    {14, 22, range::type::reader, 2, true}, 
                    {22, 25, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);

            }
        }

        // back overlap 
        WHEN("overlapping locks are added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
            auto rl1 = mgr.lock_range(22, 30, lock_manager::type::reader);

            //mgr.m_tree.display();

            THEN("appropriate range locks proxies are constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 0);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(rl1.m_ptr == nullptr);
                REQUIRE(rl1.m_start == 22);
                REQUIRE(rl1.m_end == 30);

                // there must be proxy locks for:
                // [10,22), [22,25), [25, 30)
                const int exp_sz = 3;
                REQUIRE(mgr.m_tree.size() == exp_sz);

                exp_range expected[exp_sz] = { 
                    {10, 22, range::type::reader, 1, true}, 
                    {22, 25, range::type::reader, 2, true}, 
                    {25, 30, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);
            }
        }

        // absorption overlap 
        WHEN("overlapping locks are added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
            auto rl1 = mgr.lock_range(8, 30, lock_manager::type::reader);

            //mgr.m_tree.display();

            THEN("appropriate range locks proxies are constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 0);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(rl1.m_ptr == nullptr);
                REQUIRE(rl1.m_start == 8);
                REQUIRE(rl1.m_end == 30);

                // there must be proxy locks for:
                // [8,10), [10,25), [25, 30)
                const int exp_sz = 3;
                REQUIRE(mgr.m_tree.size() == exp_sz);

                exp_range expected[exp_sz] = { 
                    { 8, 10, range::type::reader, 1, true}, 
                    {10, 25, range::type::reader, 2, true}, 
                    {25, 30, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);
            }
        }

        // join overlap 
        WHEN("overlapping locks are added") {
            lock_manager mgr;
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
            auto rl1 = mgr.lock_range(40, 48, lock_manager::type::reader);
            auto rl2 = mgr.lock_range(15, 50, lock_manager::type::reader);

            //mgr.m_tree.display();

            THEN("appropriate range locks proxies are constructed and returned") {
                REQUIRE(rl0.m_ptr != nullptr);
                REQUIRE(rl0.m_start == 10);
                REQUIRE(rl0.m_end == 25);

                REQUIRE(rl0.m_ptr->m_start == 10);
                REQUIRE(rl0.m_ptr->m_end == 25);
                REQUIRE(rl0.m_ptr->m_type == range::type::reader);
                REQUIRE(rl0.m_ptr->m_refs == 0);
                REQUIRE(rl0.m_ptr->m_is_proxy == false);
                REQUIRE(rl0.m_ptr->m_read_wanted == false);
                REQUIRE(rl0.m_ptr->m_write_wanted == false);
                REQUIRE(rl0.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl0.m_ptr->m_wr_cv.get() != nullptr);

                REQUIRE(rl1.m_ptr != nullptr);
                REQUIRE(rl1.m_start == 40);
                REQUIRE(rl1.m_end == 48);
                REQUIRE(rl1.m_ptr->m_type == range::type::reader);
                REQUIRE(rl1.m_ptr->m_refs == 0);
                REQUIRE(rl1.m_ptr->m_is_proxy == false);
                REQUIRE(rl1.m_ptr->m_read_wanted == false);
                REQUIRE(rl1.m_ptr->m_write_wanted == false);
                REQUIRE(rl1.m_ptr->m_rd_cv.get() != nullptr);
                REQUIRE(rl1.m_ptr->m_wr_cv.get() != nullptr);


                // there must be proxy locks for:
                // [8,10), [10,25), [25, 30)
                const int exp_sz = 5;
                REQUIRE(mgr.m_tree.size() == exp_sz);

                exp_range expected[exp_sz] = { 
                    {10, 15, range::type::reader, 1, true}, 
                    {15, 25, range::type::reader, 2, true}, 
                    {25, 40, range::type::reader, 1, true},
                    {40, 48, range::type::reader, 2, true},
                    {48, 50, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);
            }
        }

        WHEN("reader locks are added and deleted") {
            lock_manager mgr;

            THEN("the number of nodes is consistent") {
                auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
                REQUIRE(mgr.m_tree.size() == 1);
                mgr.unlock_range(rl0);
                REQUIRE(mgr.m_tree.size() == 0);

                auto rl1 = mgr.lock_range(40, 48, lock_manager::type::reader);
                REQUIRE(mgr.m_tree.size() == 1);
                mgr.unlock_range(rl1);
                REQUIRE(mgr.m_tree.size() == 0);

                auto rl2 = mgr.lock_range(15, 50, lock_manager::type::reader);
                REQUIRE(mgr.m_tree.size() == 1);
                mgr.unlock_range(rl2);
                REQUIRE(mgr.m_tree.size() == 0);
            }
        }

        WHEN("reader locks are added and deleted") {
            lock_manager mgr;

            THEN("the number of nodes is consistent") {
                auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
                REQUIRE(mgr.m_tree.size() == 1);

                auto rl1 = mgr.lock_range(40, 48, lock_manager::type::reader);
                REQUIRE(mgr.m_tree.size() == 2);

                auto rl2 = mgr.lock_range(15, 50, lock_manager::type::reader);

                // [10,15), [15,25), [25, 40), [40,48), [48,50)
                REQUIRE(mgr.m_tree.size() == 5);

                exp_range expected[5] = { 
                    {10, 15, range::type::reader, 1, true}, 
                    {15, 25, range::type::reader, 2, true}, 
                    {25, 40, range::type::reader, 1, true},
                    {40, 48, range::type::reader, 2, true},
                    {48, 50, range::type::reader, 1, true}
                };

                verify_tree(mgr.m_tree, expected);

                mgr.unlock_range(rl2);

                // [10,15), [15,25), [40,48)
                REQUIRE(mgr.m_tree.size() == 3);

                mgr.unlock_range(rl0);

                // [40,48)
                REQUIRE(mgr.m_tree.size() == 1);
                mgr.unlock_range(rl1);

                REQUIRE(mgr.m_tree.size() == 0);
            }
        }
    }

#if 0
    GIVEN("a sequence of reader range locks") {

        lock_manager mgr;

        WHEN("") {
            auto rl0 = mgr.lock_range(10, 25, lock_manager::type::reader);
//            mgr.m_tree.display();

            auto rlx = mgr.lock_range(20, 45, lock_manager::type::writer);
            mgr.m_tree.display();


            return;

            auto rl1 = mgr.lock_range(15, 18, lock_manager::type::reader);
//            mgr.m_tree.display();

            auto rl2 = mgr.lock_range(16, 26, lock_manager::type::reader);
//            mgr.m_tree.display();


            mgr.unlock_range(rl2);
            mgr.unlock_range(rl0);
//            mgr.m_tree.display();

            auto rl3 = mgr.lock_range(17, 18, lock_manager::type::reader);
//            mgr.m_tree.display();

            auto rl4 = mgr.lock_range(40, 45, lock_manager::type::reader);
//            mgr.m_tree.display();

//            rl0.release();

            mgr.unlock_range(rl4);
            mgr.unlock_range(rl3);
            mgr.unlock_range(rl1);


            THEN("") {
            }
        }

    }
#endif

}

SCENARIO("concurrent range locking", "[lock_manager_concurrent]"){

    GIVEN("a sequence of concurrent range locks") {

        WHEN("a reader tries to lock a range held by a writer") {

            char shared_buf[MAXBUF];
            memset(shared_buf, 'A', sizeof(shared_buf));

            char rd_buf[MAXBUF];
            memset(rd_buf, 'X', sizeof(rd_buf));

            off_t wr_start, wr_end, rd_start, rd_end;

            wr_start = 10;
            wr_end = 25;
            rd_start = 15;
            rd_end = 20;

            lock_manager mgr;

            std::thread wr([&] (){

                auto rl = mgr.lock_range(wr_start, wr_end, lock_manager::type::writer);
                
                for(int i=wr_start; i<wr_end; ++i) {
                    shared_buf[i] = 'B';
                }

                mgr.unlock_range(rl);
            });

            std::thread rd([&] (){

                // ensure that the writer gets to the lock first
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                auto rl = mgr.lock_range(rd_start, rd_end, lock_manager::type::reader);

                for(int i=rd_start; i<rd_end; ++i) {
                    rd_buf[i-rd_start] = shared_buf[i];
                }

                // wait a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                mgr.unlock_range(rl);
            });

            // wait for threads to complete
            wr.join();
            rd.join();

            THEN("reader waits until the writer is finished") {

                char exp_rd_buf[rd_end - rd_start];
                memset(exp_rd_buf, 'B', sizeof(exp_rd_buf));

                REQUIRE(memcmp(rd_buf, exp_rd_buf, sizeof(exp_rd_buf)) == 0);
            }
        }

        WHEN("a writer tries to lock a range held by a reader") {

            char shared_buf[MAXBUF];
            memset(shared_buf, 'A', sizeof(shared_buf));

            char rd_buf[MAXBUF];
            memset(rd_buf, 'X', sizeof(rd_buf));

            off_t wr_start, wr_end, rd_start, rd_end;

            wr_start = 10;
            wr_end = 25;
            rd_start = 15;
            rd_end = 20;

            lock_manager mgr;

            std::thread wr([&] (){

                // ensure that the reader gets to the lock first
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                auto rl = mgr.lock_range(wr_start, wr_end, lock_manager::type::writer);
                
                for(int i=wr_start; i<wr_end; ++i) {
                    shared_buf[i] = 'B';
                }

                mgr.unlock_range(rl);
            });

            std::thread rd([&] (){

                auto rl = mgr.lock_range(rd_start, rd_end, lock_manager::type::reader);

                for(int i=rd_start; i<rd_end; ++i) {
                    rd_buf[i-rd_start] = shared_buf[i];
                }

                // wait a bit to simulate reading
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                mgr.unlock_range(rl);
            });

            // wait for threads to complete
            wr.join();
            rd.join();

            THEN("writer waits until the reader is finished") {

                char exp_rd_buf[rd_end - rd_start];
                memset(exp_rd_buf, 'A', sizeof(exp_rd_buf));

                REQUIRE(memcmp(rd_buf, exp_rd_buf, sizeof(exp_rd_buf)) == 0);

                char exp_wr_buf[wr_end - wr_start];
                memset(exp_wr_buf, 'B', sizeof(exp_wr_buf));

                REQUIRE(memcmp(shared_buf + wr_start, exp_wr_buf, sizeof(exp_wr_buf)) == 0);
            }
        }

        WHEN("a writer tries to lock a range held by another writer") {

            char shared_buf[MAXBUF];
            memset(shared_buf, 'A', sizeof(shared_buf));

            off_t wr1_start, wr1_end, wr2_start, wr2_end;

            wr1_start = 10;
            wr1_end = 25;
            wr2_start = 15;
            wr2_end = 20;

            lock_manager mgr;

            std::thread wr([&] (){

                auto rl = mgr.lock_range(wr1_start, wr1_end, lock_manager::type::writer);
                
                for(int i=wr1_start; i<wr1_end; ++i) {
                    shared_buf[i] = 'B';
                }

                mgr.unlock_range(rl);
            });

            std::thread rd([&] (){

                // ensure that writer 1 gets to the lock first
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                auto rl = mgr.lock_range(wr2_start, wr2_end, lock_manager::type::writer);

                for(int i=wr2_start; i<wr2_end; ++i) {
                    shared_buf[i] = 'C';
                }

                mgr.unlock_range(rl);
            });

            // wait for threads to complete
            wr.join();
            rd.join();

            THEN("writer 2 waits until writer 1 is finished") {

                char exp_buf[MAXBUF];

                memset(exp_buf, 'A', sizeof(exp_buf));
                memset(&exp_buf[wr1_start], 'B', wr1_end - wr1_start);
                memset(&exp_buf[wr2_start], 'C', wr2_end - wr2_start);

                REQUIRE(memcmp(shared_buf, exp_buf, sizeof(exp_buf)) == 0);
            }
        }
    }
}
