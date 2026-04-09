#include "catch.hpp"
#include "requests.h"
#include <cstring>

TEST_CASE("RequestTypeToString", "[requests]") {
    REQUIRE(strcmp(RequestTypeToString(GET).buf_, "GET") == 0);
    REQUIRE(strcmp(RequestTypeToString(POST).buf_, "POST") == 0);
    REQUIRE(strcmp(RequestTypeToString(DELETE).buf_, "DELETE") == 0);
    REQUIRE(strcmp(RequestTypeToString(PATCH).buf_, "PATCH") == 0);
    REQUIRE(strcmp(RequestTypeToString(PUT).buf_, "PUT") == 0);
}

TEST_CASE("ContentTypeToString", "[requests]") {
    REQUIRE(strcmp(ContentTypeToString(MULTIPART_FORMDATA).buf_, "multipart/form-data") == 0);
    REQUIRE(strcmp(ContentTypeToString(APPLICATION_JSON).buf_, "application/json") == 0);
}

TEST_CASE("prettify valid JSON", "[requests]") {
    SECTION("simple object") {
        pg::String input("{\"name\":\"test\",\"value\":42}");
        pg::String result = prettify(input);
        // Should contain newlines (prettified)
        REQUIRE(strchr(result.buf_, '\n') != NULL);
        // Should contain the original values
        REQUIRE(strstr(result.buf_, "\"name\"") != NULL);
        REQUIRE(strstr(result.buf_, "\"test\"") != NULL);
        REQUIRE(strstr(result.buf_, "42") != NULL);
    }

    SECTION("nested object") {
        pg::String input("{\"a\":{\"b\":{\"c\":1}}}");
        pg::String result = prettify(input);
        REQUIRE(strchr(result.buf_, '\n') != NULL);
        REQUIRE(strstr(result.buf_, "\"c\"") != NULL);
    }

    SECTION("empty object") {
        pg::String input("{}");
        pg::String result = prettify(input);
        REQUIRE(strstr(result.buf_, "{}") != NULL);
    }

    SECTION("empty array") {
        pg::String input("[]");
        pg::String result = prettify(input);
        REQUIRE(strstr(result.buf_, "[]") != NULL);
    }
}

TEST_CASE("prettify invalid JSON", "[requests]") {
    pg::String input("not json at all");
    pg::String result = prettify(input);
    REQUIRE(strcmp(result.buf_, "not json at all") == 0);
}

TEST_CASE("prettify empty string", "[requests]") {
    pg::String input("");
    pg::String result = prettify(input);
    // Should not crash, returns input as-is since it's invalid JSON
    REQUIRE(result.length() == 0);
}

TEST_CASE("prettify idempotence", "[requests]") {
    pg::String input("{\"key\":\"value\",\"num\":123}");
    pg::String first = prettify(input);
    pg::String second = prettify(first);
    REQUIRE(strcmp(first.buf_, second.buf_) == 0);
}
