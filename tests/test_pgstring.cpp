#include "catch.hpp"
#include "pgstring.h"
#include <cstring>

TEST_CASE("pg::String default constructor", "[pgstring]") {
    pg::String s;
    REQUIRE(s.capacity() == 4098);
    REQUIRE(s.length() == 0);
    REQUIRE(s.buf_[0] == '\0');
}

TEST_CASE("pg::String capacity constructor", "[pgstring]") {
    pg::String s(100);
    REQUIRE(s.capacity() == 100);
    REQUIRE(s.length() == 0);
    REQUIRE(s.buf_[0] == '\0');
}

TEST_CASE("pg::String const char* constructor", "[pgstring]") {
    SECTION("short string fits in default capacity") {
        pg::String s("hello");
        REQUIRE(strcmp(s.buf_, "hello") == 0);
        REQUIRE(s.length() == 5);
        REQUIRE(s.capacity() == 4098);
    }

    SECTION("long string exceeding default capacity") {
        char long_str[5000];
        memset(long_str, 'A', 4999);
        long_str[4999] = '\0';
        pg::String s(long_str);
        REQUIRE(strcmp(s.buf_, long_str) == 0);
        REQUIRE(s.length() == 4999);
        REQUIRE(s.capacity() >= 5000);
    }
}

TEST_CASE("pg::String buffer+size constructor", "[pgstring]") {
    char buf[] = "hello world";
    pg::String s(buf, 12); // 11 chars + null terminator
    REQUIRE(strcmp(s.buf_, "hello world") == 0);
    REQUIRE(s.length() == 11);
}

TEST_CASE("pg::String set", "[pgstring]") {
    SECTION("set on empty string") {
        pg::String s;
        s.set("test value");
        REQUIRE(strcmp(s.buf_, "test value") == 0);
        REQUIRE(s.length() == 10);
    }

    SECTION("set replaces existing content") {
        pg::String s("first");
        s.set("second");
        REQUIRE(strcmp(s.buf_, "second") == 0);
    }

    SECTION("set with string exceeding current capacity") {
        pg::String s(10);
        s.set("this is a longer string that exceeds capacity");
        REQUIRE(strcmp(s.buf_, "this is a longer string that exceeds capacity") == 0);
    }
}

TEST_CASE("pg::String append const char*", "[pgstring]") {
    SECTION("append to empty string") {
        pg::String s;
        s.append("world");
        REQUIRE(strcmp(s.buf_, "world") == 0);
    }

    SECTION("append to existing content") {
        pg::String s("hello");
        s.append(" world");
        REQUIRE(strcmp(s.buf_, "hello world") == 0);
    }

    SECTION("append with explicit size") {
        pg::String s("hello");
        // size param includes null terminator (matches default strlen+1 behavior)
        s.append(" world!", 8);
        REQUIRE(strcmp(s.buf_, "hello world!") == 0);
    }

    SECTION("multiple appends") {
        pg::String s("a");
        s.append("b");
        s.append("c");
        REQUIRE(strcmp(s.buf_, "abc") == 0);
    }
}

TEST_CASE("pg::String append String overload", "[pgstring]") {
    pg::String s("hello");
    pg::String suffix(" world");
    s.append(suffix);
    REQUIRE(strcmp(s.buf_, "hello world") == 0);
}

TEST_CASE("pg::String operator[]", "[pgstring]") {
    pg::String s("abc");

    SECTION("read access") {
        REQUIRE(s[0] == 'a');
        REQUIRE(s[1] == 'b');
        REQUIRE(s[2] == 'c');
    }

    SECTION("write access") {
        s[0] = 'x';
        REQUIRE(s[0] == 'x');
        REQUIRE(strcmp(s.buf_, "xbc") == 0);
    }

    SECTION("const access") {
        const pg::String& cs = s;
        REQUIRE(cs[0] == 'a');
    }
}

TEST_CASE("pg::String copy constructor", "[pgstring]") {
    pg::String original("hello");
    pg::String copy(original);

    REQUIRE(strcmp(copy.buf_, "hello") == 0);
    REQUIRE(copy.length() == 5);

    // Verify deep copy: modifying original doesn't affect copy
    original.set("changed");
    REQUIRE(strcmp(copy.buf_, "hello") == 0);
}

TEST_CASE("pg::String assignment operator", "[pgstring]") {
    SECTION("basic assignment") {
        pg::String a("hello");
        pg::String b;
        b = a;
        REQUIRE(strcmp(b.buf_, "hello") == 0);

        // Verify independence
        a.set("changed");
        REQUIRE(strcmp(b.buf_, "hello") == 0);
    }

    SECTION("assign longer string to shorter capacity") {
        pg::String a(10);
        pg::String b("this string is longer than 10 chars");
        a = b;
        REQUIRE(strcmp(a.buf_, "this string is longer than 10 chars") == 0);
    }
}

TEST_CASE("pg::String length and capacity", "[pgstring]") {
    pg::String s("test");
    REQUIRE(s.length() == 4);
    REQUIRE(s.capacity() == 4098);

    s.set("longer test string");
    REQUIRE(s.length() == 18);
    // capacity unchanged since it still fits
    REQUIRE(s.capacity() == 4098);
}

TEST_CASE("pg::String end", "[pgstring]") {
    pg::String s("hello");
    REQUIRE(*s.end() == '\0');
    REQUIRE(s.end() == s.buf_ + 5);
}

TEST_CASE("pg::String realloc does not preserve data", "[pgstring]") {
    // NOTE: This documents existing behavior — realloc() frees old buffer
    // and allocates new memory WITHOUT copying existing data.
    pg::String s("important data");
    int old_capacity = s.capacity();
    s.realloc(old_capacity * 2);
    REQUIRE(s.capacity() == old_capacity * 2);
    // After realloc, old data is gone — this is known behavior
}
