#include "catch.hpp"
#include "utils.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

// readFile is defined in utils.cpp but not declared in utils.h
extern char* readFile(char* filename);

// RAII temp file helper for file I/O tests
struct TempFile {
    char path[256];
    TempFile(const char* content = NULL) {
        snprintf(path, sizeof(path), "/tmp/postgirl_test_XXXXXX");
        int fd = mkstemp(path);
        if (content) {
            ssize_t ret = write(fd, content, strlen(content));
            (void)ret;
        }
        close(fd);
    }
    ~TempFile() { remove(path); }
};

// --- Stristr tests ---

TEST_CASE("Stristr basic matching", "[utils]") {
    SECTION("found in middle, case insensitive") {
        const char* result = Stristr("Hello World", NULL, "world", NULL);
        REQUIRE(result != NULL);
        REQUIRE(result == &"Hello World"[6]);
    }

    SECTION("not found") {
        const char* result = Stristr("Hello World", NULL, "xyz", NULL);
        REQUIRE(result == NULL);
    }

    SECTION("match at start") {
        const char* result = Stristr("Hello World", NULL, "hello", NULL);
        REQUIRE(result != NULL);
        REQUIRE(result == &"Hello World"[0]);
    }

    SECTION("match at end") {
        const char* result = Stristr("Hello World", NULL, "WORLD", NULL);
        REQUIRE(result != NULL);
    }

    SECTION("single character match") {
        const char* result = Stristr("abc", NULL, "B", NULL);
        REQUIRE(result != NULL);
        REQUIRE(*result == 'b');
    }
}

TEST_CASE("Stristr with explicit ends", "[utils]") {
    const char* haystack = "Hello World";

    SECTION("haystack_end limits search") {
        // Only search "Hello"
        const char* result = Stristr(haystack, haystack + 5, "World", NULL);
        REQUIRE(result == NULL);
    }

    SECTION("needle_end uses partial needle") {
        const char* needle = "Helloxxxxx";
        // Only use first 5 chars of needle
        const char* result = Stristr(haystack, NULL, needle, needle + 5);
        REQUIRE(result != NULL);
        REQUIRE(result == haystack);
    }
}

TEST_CASE("Stristr edge cases", "[utils]") {
    SECTION("needle longer than haystack") {
        const char* result = Stristr("Hi", NULL, "Hello World", NULL);
        REQUIRE(result == NULL);
    }

    SECTION("repeated partial matches") {
        const char* result = Stristr("aab", NULL, "ab", NULL);
        REQUIRE(result != NULL);
        REQUIRE(result == &"aab"[1]);
    }
}

// --- readFile tests ---

TEST_CASE("readFile", "[utils]") {
    SECTION("reads file content") {
        TempFile tmp("hello file content");
        char* content = readFile(tmp.path);
        REQUIRE(content != NULL);
        REQUIRE(strcmp(content, "hello file content") == 0);
        free(content);
    }

    SECTION("non-existent file returns NULL") {
        char* content = readFile((char*)"/tmp/postgirl_nonexistent_file_12345");
        REQUIRE(content == NULL);
    }
}

// --- Collection round-trip tests ---

TEST_CASE("saveCollection and loadCollection round-trip", "[utils]") {
    TempFile tmp;

    // Build a collection in memory
    pg::Vector<Collection> collections;
    Collection col;
    col.name.set("TestCollection");

    History hist;
    hist.url.set("http://example.com/api");
    hist.input_json.set("{\"key\":\"value\"}");
    hist.req_type = POST;
    hist.content_type = APPLICATION_JSON;
    hist.process_time.set("0.123s");
    hist.result.set("{\"status\":\"ok\"}");
    hist.response_code = 200;

    Argument arg;
    arg.name.set("param1");
    arg.value.set("value1");
    arg.arg_type = 0;
    hist.args.push_back(arg);

    Argument header;
    header.name.set("Authorization");
    header.value.set("Bearer token123");
    header.arg_type = 0;
    hist.headers.push_back(header);

    col.hist.push_back(hist);
    collections.push_back(col);

    // Save
    pg::String filename(tmp.path);
    saveCollection(collections, filename);

    // Load back
    pg::Vector<Collection> loaded = loadCollection(filename);

    REQUIRE(loaded.size() == 1);
    REQUIRE(strcmp(loaded[0].name.buf_, "TestCollection") == 0);
    REQUIRE(loaded[0].hist.size() == 1);

    const History& h = loaded[0].hist[0];
    REQUIRE(strcmp(h.url.buf_, "http://example.com/api") == 0);
    REQUIRE(h.req_type == POST);
    REQUIRE(h.content_type == APPLICATION_JSON);
    REQUIRE(h.response_code == 200);
    REQUIRE(strcmp(h.process_time.buf_, "0.123s") == 0);

    // Args
    REQUIRE(h.args.size() == 1);
    REQUIRE(strcmp(h.args[0].name.buf_, "param1") == 0);
    REQUIRE(strcmp(h.args[0].value.buf_, "value1") == 0);

    // Headers
    REQUIRE(h.headers.size() == 1);
    REQUIRE(strcmp(h.headers[0].name.buf_, "Authorization") == 0);
    REQUIRE(strcmp(h.headers[0].value.buf_, "Bearer token123") == 0);
}

TEST_CASE("loadCollection missing or invalid file", "[utils]") {
    SECTION("non-existent file returns empty vector") {
        pg::String filename("/tmp/postgirl_nonexistent_collections_12345");
        pg::Vector<Collection> result = loadCollection(filename);
        REQUIRE(result.size() == 0);
    }

    SECTION("invalid JSON returns empty vector") {
        TempFile tmp("this is not json");
        pg::String filename(tmp.path);
        pg::Vector<Collection> result = loadCollection(filename);
        REQUIRE(result.size() == 0);
    }

    SECTION("valid JSON without collections key returns empty vector") {
        TempFile tmp("{\"other_key\": 42}");
        pg::String filename(tmp.path);
        pg::Vector<Collection> result = loadCollection(filename);
        REQUIRE(result.size() == 0);
    }
}
