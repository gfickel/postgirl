#include "catch.hpp"
#include <cstring>  // needed for memmove used by pg::Vector::erase()
#include "pgvector.h"

TEST_CASE("pg::Vector default constructor", "[pgvector]") {
    pg::Vector<int> v;
    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() == 0);
    REQUIRE(v.empty());
    REQUIRE(v.Data == NULL);
}

TEST_CASE("pg::Vector push_back and operator[]", "[pgvector]") {
    pg::Vector<int> v;

    SECTION("single element") {
        v.push_back(42);
        REQUIRE(v.size() == 1);
        REQUIRE(!v.empty());
        REQUIRE(v[0] == 42);
    }

    SECTION("multiple elements") {
        for (int i = 0; i < 5; i++)
            v.push_back(i * 10);

        REQUIRE(v.size() == 5);
        REQUIRE(v[0] == 0);
        REQUIRE(v[2] == 20);
        REQUIRE(v[4] == 40);
    }

    SECTION("elements survive capacity growth") {
        for (int i = 0; i < 20; i++)
            v.push_back(i);

        REQUIRE(v.size() == 20);
        for (int i = 0; i < 20; i++)
            REQUIRE(v[i] == i);
    }
}

TEST_CASE("pg::Vector pop_back", "[pgvector]") {
    pg::Vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    v.pop_back();
    REQUIRE(v.size() == 2);
    REQUIRE(v[1] == 2);

    v.pop_back();
    v.pop_back();
    REQUIRE(v.size() == 0);
    REQUIRE(v.empty());
}

TEST_CASE("pg::Vector resize", "[pgvector]") {
    pg::Vector<int> v;

    SECTION("grow from empty") {
        v.resize(10);
        REQUIRE(v.size() == 10);
        REQUIRE(v.capacity() >= 10);
    }

    SECTION("shrink") {
        v.resize(10);
        int cap = v.capacity();
        v.resize(3);
        REQUIRE(v.size() == 3);
        REQUIRE(v.capacity() == cap); // capacity doesn't shrink
    }
}

TEST_CASE("pg::Vector resize with fill value", "[pgvector]") {
    pg::Vector<int> v;
    v.resize(5, 99);
    REQUIRE(v.size() == 5);
    for (int i = 0; i < 5; i++)
        REQUIRE(v[i] == 99);
}

TEST_CASE("pg::Vector reserve", "[pgvector]") {
    pg::Vector<int> v;

    SECTION("reserve increases capacity") {
        v.reserve(100);
        REQUIRE(v.capacity() >= 100);
        REQUIRE(v.size() == 0);
    }

    SECTION("reserve then push_back") {
        v.reserve(100);
        v.push_back(42);
        REQUIRE(v[0] == 42);
        REQUIRE(v.size() == 1);
    }

    SECTION("reserve smaller does nothing") {
        v.reserve(50);
        int cap = v.capacity();
        v.reserve(10);
        REQUIRE(v.capacity() == cap);
    }
}

TEST_CASE("pg::Vector clear", "[pgvector]") {
    pg::Vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    v.clear();
    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() == 0);
    REQUIRE(v.Data == NULL);
}

// NOTE: erase tests use int only — memmove in erase() is unsafe for
// types with destructors (like pg::String) as it does bitwise copy.
TEST_CASE("pg::Vector erase single element", "[pgvector]") {
    pg::Vector<int> v;
    for (int i = 0; i < 5; i++)
        v.push_back(i * 10); // 0, 10, 20, 30, 40

    SECTION("erase first") {
        v.erase(v.begin());
        REQUIRE(v.size() == 4);
        REQUIRE(v[0] == 10);
        REQUIRE(v[3] == 40);
    }

    SECTION("erase middle") {
        v.erase(v.begin() + 2);
        REQUIRE(v.size() == 4);
        REQUIRE(v[0] == 0);
        REQUIRE(v[1] == 10);
        REQUIRE(v[2] == 30);
        REQUIRE(v[3] == 40);
    }

    SECTION("erase last") {
        v.erase(v.end() - 1);
        REQUIRE(v.size() == 4);
        REQUIRE(v[3] == 30);
    }
}

TEST_CASE("pg::Vector erase range", "[pgvector]") {
    pg::Vector<int> v;
    for (int i = 0; i < 5; i++)
        v.push_back(i * 10); // 0, 10, 20, 30, 40

    v.erase(v.begin() + 1, v.begin() + 3); // remove 10, 20
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 0);
    REQUIRE(v[1] == 30);
    REQUIRE(v[2] == 40);
}

TEST_CASE("pg::Vector contains", "[pgvector]") {
    pg::Vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    REQUIRE(v.contains(20) == true);
    REQUIRE(v.contains(99) == false);

    SECTION("empty vector") {
        pg::Vector<int> empty;
        REQUIRE(empty.contains(1) == false);
    }
}

TEST_CASE("pg::Vector iterators", "[pgvector]") {
    pg::Vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    SECTION("begin and end") {
        REQUIRE(*v.begin() == 1);
        REQUIRE(v.end() == v.begin() + 3);
    }

    SECTION("empty vector iterators") {
        pg::Vector<int> empty;
        REQUIRE(empty.begin() == empty.end());
    }

    SECTION("front and back") {
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 3);
    }

    SECTION("range-for loop") {
        int sum = 0;
        for (int x : v)
            sum += x;
        REQUIRE(sum == 6);
    }
}

TEST_CASE("pg::Vector copy constructor", "[pgvector]") {
    pg::Vector<int> original;
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    pg::Vector<int> copy(original);
    REQUIRE(copy.size() == 3);
    REQUIRE(copy[0] == 1);
    REQUIRE(copy[2] == 3);

    // Verify independence
    original[0] = 99;
    REQUIRE(copy[0] == 1);
}

TEST_CASE("pg::Vector assignment operator", "[pgvector]") {
    pg::Vector<int> a;
    a.push_back(10);
    a.push_back(20);

    pg::Vector<int> b;
    b = a;
    REQUIRE(b.size() == 2);
    REQUIRE(b[0] == 10);
    REQUIRE(b[1] == 20);

    // Verify independence
    a[0] = 99;
    REQUIRE(b[0] == 10);
}

TEST_CASE("pg::Vector _grow_capacity", "[pgvector]") {
    pg::Vector<int> v;

    SECTION("initial growth starts at 8") {
        REQUIRE(v._grow_capacity(1) == 8);
    }

    SECTION("grows by 1.5x when larger") {
        v.reserve(100);
        // 100 + 100/2 = 150
        REQUIRE(v._grow_capacity(101) == 150);
    }

    SECTION("uses requested size when larger than 1.5x") {
        v.reserve(10);
        // 10 + 10/2 = 15, but requested 20
        REQUIRE(v._grow_capacity(20) == 20);
    }
}
