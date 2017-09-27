#include "catch.hpp"

#if 0
#include <chrono>
#include <fstream>
#include <list>
#endif

#define __AVL_DEBUG__
#include <iostream>
#include <memory>
#include <condition_variable>
#include <backends/nvram-nvml/avl.hpp>

struct xrange { 
    off_t start;
    off_t end;
    std::shared_ptr<std::condition_variable> cv;

    xrange(off_t a, off_t b) {
        start = a;
        end = b;
    }

    std::string id(){
        return std::to_string(start);
    }

    int compare(const xrange& other) const {
        return (start > other.start) - (start < other.start);
    }

};

std::ostream& operator<<(std::ostream& os, const xrange& r) {

    os << "[" << r.start << "," << r.end << ")";

    return os;
}


#if 0
struct word {

    word(const std::string& str)
        : m_str(str) { }

    std::string id(){

        std::string tmp = m_str;

        std::replace(tmp.begin(), tmp.end(), '\'', '0');

        return tmp;
    }

    int compare(const word& other) const {
        
        int diff = m_str.compare(other.m_str);

        if(diff >= 1) {
            return 1;
        }

        if(diff <= -1) {
            return -1;
        }

        return 0;
    }


    std::string m_str;
};

std::ostream& operator<<(std::ostream& os, const word& w) {

    os << w.m_str;

    return os;
}
#endif

SCENARIO("elements can be added and removed", "[avltree]"){

    GIVEN("an avl tree with some items") {

        avl::avltree<xrange> t;

        t.add({10, 20});
        t.add({14, 16});
        t.add({11, 26});
        t.add({20, 35});
        t.add({40, 45});
        t.add({63, 85});
        t.add({140, 445});

//        t.display();

//        std::cout << "remove (63, 85)\n";
        t.remove({63, 85});

//        t.display();

        t.remove({14, 16});

//        t.display();

        t.remove({11, 26});
        t.remove({140, 445});
        t.remove({40, 45});
        t.remove({10, 20});
        t.remove({20, 35});
    }

    GIVEN("an avl tree with some items") {

        avl::avltree<xrange> t;

        t.add(xrange(10, 20));
        t.add(xrange(14, 16));
        t.add(xrange(11, 26));
        t.add(xrange(40, 45));

        //t.display();

        REQUIRE(t.first()->start == 10);
        REQUIRE(t.last()->start == 40);

        WHEN("searching for an existing node") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(14, 16), where);

            THEN("the node is found and returned") {
                REQUIRE(xr != nullptr);
                REQUIRE(xr->start == 14);
                REQUIRE(xr->end == 16);
                REQUIRE(where == avl::avltree<xrange>::insertion_point::null);
            }
        }

        WHEN("searching for a non-existing node") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(12, 15), where);

            THEN("the node is not found") {
                REQUIRE(xr == nullptr);
            }
        }

        WHEN("calling nearest(insertion_point, avl::BEFORE) using an invalid insertion_point") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(14, 16), where);

            xr = t.nearest(where, avl::BEFORE);

            THEN("nothing is returned") {
                REQUIRE(xr == nullptr);
            }
        }

        WHEN("calling nearest(insertion_point, avl::AFTER) using an invalid insertion_point") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(14, 16), where);

            xr = t.nearest(where, avl::AFTER);

            THEN("nothing is returned") {
                REQUIRE(xr == nullptr);
            }
        }

        WHEN("calling nearest(insertion_point, avl::BEFORE) with a valid insertion_point") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(12, 13), where);

            xr = t.nearest(where, avl::BEFORE);

            THEN("the nearest lower node is returned") {
                REQUIRE(xr != nullptr);
                REQUIRE(xr->start == 11);
                REQUIRE(xr->end == 26);
            }
        }

        WHEN("calling nearest(insertion_point, avl::BEFORE) with a valid insertion_point below first()") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(8, 13), where);

            xr = t.nearest(where, avl::BEFORE);

            THEN("nothing is returned") {
                REQUIRE(xr == nullptr);
            }
        }

        WHEN("calling nearest(insertion_point, avl::AFTER) with a valid insertion_point") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(12, 13), where);

            xr = t.nearest(where, avl::AFTER);

            THEN("the nearest higher node is returned") {
                REQUIRE(xr != nullptr);
                REQUIRE(xr->start == 14);
                REQUIRE(xr->end == 16);
            }
        }

        WHEN("calling nearest(insertion_point, avl::AFTER) with a valid insertion_point") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(16, 20), where);

            xr = t.nearest(where, avl::AFTER);

            THEN("the nearest higher node is returned") {
                REQUIRE(xr != nullptr);
                REQUIRE(xr->start == 40);
                REQUIRE(xr->end == 45);
            }
        }

        WHEN("calling nearest(insertion_point, avl::AFTER) with a valid insertion_point above last()") {
            avl::avltree<xrange>::insertion_point where;
            auto xr = t.find(xrange(50, 60), where);

            xr = t.nearest(where, avl::AFTER);

            THEN("nothing is returned") {
                REQUIRE(xr == nullptr);
            }
        }
    }
    
#if 0
    GIVEN("a tree with some items") {

        avl::avltree<word> t;

        std::ifstream fin("/usr/share/dict/words");

        if(fin) {
            std::string w;

            std::list<int64_t> deltas;

            int i = 0;
            while(fin >> w) {

                auto t1 = std::chrono::high_resolution_clock::now();

                t.add(word(w));

                auto t2 = std::chrono::high_resolution_clock::now();

                auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();


                deltas.push_back(delta);

            
            }

            int sum = 0; 
            for(auto d : deltas) {
                sum += d;
            }

            std::cout << "Total: " << sum << "ns [avg=" << sum/deltas.size() << "ns / insert]\n";

        }

        fin.close();
    }
#endif


}
