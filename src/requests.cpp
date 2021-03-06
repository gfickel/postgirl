#include "requests.h"



struct WriteThis {
  const char *readptr;
  size_t sizeleft;
};
 
static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *wt = (struct WriteThis *)userp;
  size_t buffer_size = size*nmemb;
 
  if(wt->sizeleft) {
    /* copy as much as possible from the source to the destination */ 
    size_t copy_this_much = wt->sizeleft;
    if(copy_this_much > buffer_size)
      copy_this_much = buffer_size;
    memcpy(dest, wt->readptr, copy_this_much);
 
    wt->readptr += copy_this_much;
    wt->sizeleft -= copy_this_much;
    return copy_this_much; /* we copied this many bytes */ 
  }
 
  return 0; /* no more data left to deliver */ 
}

typedef struct MemoryStruct {
    MemoryStruct() { memory = (char*)malloc(1); size=0; };
    
    char *memory;
    size_t size;
} MemoryStruct;
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}


void threadRequestGetDelete(std::atomic<ThreadStatus>& thread_status, RequestType reqType, pg::String url,
        pg::Vector<Argument> args, pg::Vector<Argument> headers, 
                      ContentType contentTypeEnum, pg::String& thread_result, int& response_code) 
{ 
    CURLcode res;
    CURL* curl;
    curl = curl_easy_init();


    pg::String contentType = ContentTypeToString(contentTypeEnum); 

    MemoryStruct chunk;
    if (reqType == GET) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    
    if (args.size() > 0) url.append("?");
    for (int i=0; i<(int)args.size(); i++) {
        char* escaped_name = curl_easy_escape(curl , args[i].name.buf_, args[i].name.length());
        url.append(escaped_name);
        url.append("=");
        char* escaped_value = curl_easy_escape(curl , args[i].value.buf_, args[i].value.length());
        url.append(escaped_value);
        if (i < (int)args.size()-1) url.append("&");
    }

    struct curl_slist *header_chunk = NULL;
    if (contentType.length() > 0) {
        pg::String aux("Content-Type: ");
        aux.append(contentType);
        header_chunk = curl_slist_append(header_chunk, aux.buf_);
    }
    for (int i=0; i<(int)headers.size(); i++) {
        pg::String header(headers[i].name);
        if (headers[i].name.length() > 0) header.append(": ");
        header.append(headers[i].value);
        header_chunk = curl_slist_append(header_chunk, header.buf_);
    }
    if (header_chunk) {
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_chunk);
        if (res != CURLE_OK) {
            thread_result = "Problem setting header!";
            thread_status = FINISHED;
            curl_easy_cleanup(curl);
            return;
        }
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.buf_);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl);
    pg::String response_body;
    long resp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
    response_code = (int)resp_code;
    if(res != CURLE_OK) {
        thread_result = pg::String(curl_easy_strerror(res));
        if(chunk.size > 0) thread_result = pg::String(chunk.memory); 
    } else {
        thread_result = pg::String("All ok");
        if(chunk.size > 0) thread_result = prettify(chunk.memory); 
    }
    thread_status = FINISHED;
    curl_easy_cleanup(curl);
}


void threadRequestPostPatchPut(std::atomic<ThreadStatus>& thread_status, RequestType reqType,
                      pg::String url, pg::Vector<Argument> args, pg::Vector<Argument> headers, 
                      ContentType contentTypeEnum, const pg::String& inputJson, 
                      pg::String& thread_result, int& response_code) 
{ 
    CURL *curl;
    CURLcode res;
    MemoryStruct chunk;
    
    pg::String contentType = ContentTypeToString(contentTypeEnum); 

    if (args.size() == 0 && inputJson.length() == 0) {
        thread_result = "No argument passed for POST";
        thread_status = FINISHED;
        return;
    }

    struct WriteThis wt;
    if (contentTypeEnum == APPLICATION_JSON) {
        wt.readptr = inputJson.buf_;
        wt.sizeleft = inputJson.length();
    } 

    /* In windows, this will init the winsock stuff */ 
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    /* Check for errors */ 
    if(res != CURLE_OK) {
        thread_result = curl_easy_strerror(res);
        thread_status = FINISHED;
        return;
    }

    curl_mime *form = NULL;
    curl_mimepart *field = NULL;

    /* get a curl handle */ 
    curl = curl_easy_init();
    if(curl) {
        pg::Vector<int> files_idx;
        pg::Vector<int> args_idx;
        for (int i=0; i<args.size(); i++) {
            if (args[i].arg_type == 1) {
                files_idx.push_back(i);
            } else {
                args_idx.push_back(i);
            }
        }

        for (int i=0; i<files_idx.size(); i++) {
            if (form == NULL) {
                /* Create the form */ 
                form = curl_mime_init(curl);
            }
         
            /* Fill in the file upload field */ 
            field = curl_mime_addpart(form);
            curl_mime_name(field, args[files_idx[i]].name.buf_);
            curl_mime_filedata(field, args[files_idx[i]].value.buf_);
        }

        if (args_idx.size() > 0) url.append("?");
        for (int i=0; i<args_idx.size(); i++) {
            int curr_idx = args_idx[i];
            char* escaped_name = curl_easy_escape(curl , args[curr_idx].name.buf_, args[curr_idx].name.length());
            url.append(escaped_name);
            url.append("=");
            char* escaped_value = curl_easy_escape(curl , args[curr_idx].value.buf_, args[curr_idx].value.length());
            url.append(escaped_value);
            if (i < (int)args_idx.size()-1) url.append("&");
        }
     
        struct curl_slist *header_chunk = NULL;
        if (contentType.length() > 0) {
            pg::String aux("Content-Type: ");
            aux.append(contentType);
            header_chunk = curl_slist_append(header_chunk, aux.buf_);
        }
        for (int i=0; i<(int)headers.size(); i++) {
            pg::String header(headers[i].name);
            if (headers[i].name.length() > 0) header.append(": ");
            header.append(headers[i].value);
            header_chunk = curl_slist_append(header_chunk, header.buf_);
        }
        if (header_chunk) {
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_chunk);
            if (res != CURLE_OK) {
                thread_result = "Problem setting header!";
                thread_status = FINISHED;
                curl_easy_cleanup(curl);
                return;
            }
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.buf_);


        if (reqType == POST) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        } else if (reqType == PATCH) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        } else {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        }
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &wt);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        if (form != NULL) {
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        }
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)wt.sizeleft);

        res = curl_easy_perform(curl);
        long resp_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
        response_code = (int)resp_code;

        if(res != CURLE_OK) {
            thread_result = pg::String(curl_easy_strerror(res));
            if(chunk.size > 0) thread_result = prettify(chunk.memory); 
        } else {
            thread_result = pg::String("All ok");
            if(chunk.size > 0) thread_result = prettify(chunk.memory); 
        }
        /* always cleanup */ 
        curl_easy_cleanup(curl);
        if (form != NULL) curl_mime_free(form);
    }
    thread_status = FINISHED;
}



pg::String RequestTypeToString(RequestType req) {
    switch(req) {
        case GET:       return pg::String("GET");
        case POST:      return pg::String("POST");
        case DELETE:    return pg::String("DELETE");
        case PATCH:     return pg::String("PATCH");
        case PUT:       return pg::String("PUT");
    }
    return pg::String("UNDEFINED");
}


pg::String ContentTypeToString(ContentType ct) {
    switch(ct) {
        case MULTIPART_FORMDATA:   return pg::String("multipart/form-data");
        case APPLICATION_JSON:     return pg::String("application/json");
    }
    return pg::String("<NONE>");
}


pg::String prettify(pg::String input) {
    rapidjson::Document document;
    if (document.Parse(input.buf_).HasParseError()) {
        return input;
    }
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    document.Accept(writer);    // Accept() traverses the DOM and generates Handler events.
    return pg::String(sb.GetString());
}


