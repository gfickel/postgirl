#pragma once 

#include <atomic>
#include <curl/curl.h>
#include "pgstring.h"
#include "pgvector.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"


typedef enum ThreadStatus {
    IDLE     = 0,
    RUNNING  = 1,
    FINISHED = 2
} ThreadStatus;

typedef enum RequestType {
    GET     = 0,
    POST    = 1,
    DELETE  = 2,
    PATCH   = 3,
    PUT     = 4
} RequestType;

typedef enum ContentType {
    MULTIPART_FORMDATA  = 0,
    APPLICATION_JSON    = 1
} ContentType;

typedef struct Argument { 
    pg::String name;
    pg::String value;
    int arg_type; // TODO: transform this to an ENUM soon!!!!
} Argument; 


typedef struct History {
    pg::String url;
    pg::Vector<Argument> args;
    pg::Vector<Argument> headers;
    pg::String input_json;
    pg::String result;
    RequestType req_type;
    ContentType content_type;
    pg::String process_time;
    int response_code;
} History;


typedef struct Collection {
    pg::String name;
    pg::Vector<History> hist;
} Collection;


void threadRequestGetDelete(std::atomic<ThreadStatus>& thread_status, RequestType reqType,  
                      pg::String url, pg::Vector<Argument> args, pg::Vector<Argument> headers, 
                      ContentType contentType, pg::String& thread_result, int& response_code);

void threadRequestPostPatchPut(std::atomic<ThreadStatus>& thread_status, RequestType reqType,
                      pg::String url, pg::Vector<Argument> args, pg::Vector<Argument> headers, 
                      ContentType contentType, const pg::String& inputJson, 
                      pg::String& thread_result, int& response_code) ;

pg::String RequestTypeToString(RequestType req);

pg::String ContentTypeToString(ContentType ct);

pg::String prettify(pg::String input);

