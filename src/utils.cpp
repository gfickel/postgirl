#include "utils.h"

void readIntFromIni(int& res, FILE* fid) {
    if (fscanf(fid, "\%*s %d", &res) != 1) {
        printf("Error reading int from .ini file\n");
        exit(1);
    }
}

void readStringFromIni(char* buffer, FILE* fid) {
    int str_len, ret;
    if ((ret = fscanf(fid, "%*s (%d): ", &str_len)) != 1) {
        printf("Error reading string from .ini file: %d\n", ret);
        exit(1);
    }
    fread(buffer, sizeof(char), str_len, fid);
    buffer[str_len] = '\0';
}

void printArg(const Argument& arg) {
    printf("name: %s\nvalue: %s\narg_type: %d\n\n", arg.name.buf_, arg.value.buf_, arg.arg_type);
}

void printHistory(const History& hist) {
    printf("url: %s\ninput_json: %s\nresult: %s\n", hist.url.buf_, hist.input_json.buf_, hist.result.buf_);
    printf("req_type: %d\ncontent_type: %d\nresponse_code: %d\n", (int)hist.req_type, (int)hist.content_type, hist.response_code);
    printf("process_time: %s\n", hist.process_time.buf_);
    for (int i=0; i<hist.args.size(); i++)
        printArg(hist.args[i]);
    for (int i=0; i<hist.headers.size(); i++)
        printArg(hist.headers[i]);
    printf("----------------------------------------------------------\n\n");
}

// Thanks lfzawacki: https://stackoverflow.com/questions/3463426/in-c-how-should-i-read-a-text-file-and-print-all-strings/3464656#3464656 
char* readFile(char *filename)
{
   char *buffer = NULL;
   int string_size, read_size;
   FILE *handler = fopen(filename, "r");

   if (handler)
   {
       // Seek the last byte of the file
       fseek(handler, 0, SEEK_END);
       // Offset from the first to the last byte, or in other words, filesize
       string_size = ftell(handler);
       // go back to the start of the file
       rewind(handler);

       // Allocate a string that can hold it all
       buffer = (char*) malloc(sizeof(char) * (string_size + 1) );

       // Read it all in one operation
       read_size = fread(buffer, sizeof(char), string_size, handler);

       // fread doesn't set it so put a \0 in the last position
       // and buffer is now officially a string
       buffer[string_size] = '\0';

       if (string_size != read_size)
       {
           // Something went wrong, throw away the memory and set
           // the buffer to NULL
           free(buffer);
           buffer = NULL;
       }

       // Always remember to close the file.
       fclose(handler);
    }

    return buffer;
}


pg::Vector<Collection> loadCollection(const pg::String& filename) 
{
    pg::Vector<Collection> collection_vec;
    rapidjson::Document document;
    char* json = readFile(filename.buf_);
    if (json == NULL || document.Parse(json).HasParseError()) {
        return collection_vec;
    }
    document.Parse(json);
    if (document.HasMember("collections") == false) {
        delete json;
        return collection_vec;
    }

    const rapidjson::Value& collections = document["collections"];
    for (rapidjson::SizeType i = 0; i < collections.Size(); i++) { 
        Collection curr_collection;
        curr_collection.name = pg::String(collections[i]["name"].GetString());
        const rapidjson::Value& histories = collections[i]["histories"];
        for (rapidjson::SizeType j = 0; j < histories.Size(); j++) {
            History hist;
            hist.url = pg::String(histories[j]["url"].GetString());
            hist.input_json = pg::String(histories[j]["input_json"].GetString());
            hist.req_type = (RequestType)histories[j]["request_type"].GetInt();
            hist.content_type = (ContentType)histories[j]["content_type"].GetInt();
            hist.process_time = pg::String(histories[j]["process_time"].GetString());
            hist.result = prettify(pg::String(histories[j]["result"].GetString()));
            hist.response_code = histories[j]["response_code"].GetInt();
            
            const rapidjson::Value& headers = histories[j]["headers"];
            const rapidjson::Value& arguments = histories[j]["arguments"];

            for (rapidjson::SizeType k = 0; k < headers.Size(); k++) {
                Argument header;
                header.name  = pg::String(headers[k]["name"].GetString());
                header.value = pg::String(headers[k]["value"].GetString());
                header.arg_type = headers[k]["argument_type"].GetInt();
                hist.headers.push_back(header);
            }

            for (rapidjson::SizeType k = 0; k < arguments.Size(); k++) {
                Argument arg;
                arg.name  = pg::String(arguments[k]["name"].GetString());
                arg.value = pg::String(arguments[k]["value"].GetString());
                arg.arg_type = arguments[k]["argument_type"].GetInt();
                hist.args.push_back(arg);
            }
            // printHistory(hist);
            curr_collection.hist.push_back(hist);
        }
        collection_vec.push_back(curr_collection);
    }

    delete json;
    return collection_vec;
}

void saveCollection(const pg::Vector<Collection>& collection, const pg::String& filename) 
{
	rapidjson::Document document;
	document.SetObject();
	rapidjson::Value collection_array(rapidjson::kArrayType);
 
	// must pass an allocator when the object may need to allocate memory
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    for (int i=0; i<collection.size(); i++) {
        rapidjson::Value curr_collection(rapidjson::kObjectType);
        rapidjson::Value name_str;
        name_str.SetString(collection[i].name.buf_, allocator);
        curr_collection.AddMember("name", name_str, allocator);


        rapidjson::Value history_array(rapidjson::kArrayType);
        for (int j=0; j<collection[i].hist.size(); j++) {
            rapidjson::Value curr_history(rapidjson::kObjectType);
            rapidjson::Value url_str;
            url_str.SetString(collection[i].hist[j].url.buf_, allocator);
            curr_history.AddMember("url", url_str, allocator);
            
            rapidjson::Value input_json_str;
            input_json_str.SetString(collection[i].hist[j].input_json.buf_, allocator);
            curr_history.AddMember("input_json", input_json_str, allocator);

            curr_history.AddMember("request_type", collection[i].hist[j].req_type, allocator);
            curr_history.AddMember("content_type", collection[i].hist[j].content_type, allocator);
            
            rapidjson::Value process_time_str;
            process_time_str.SetString(collection[i].hist[j].process_time.buf_, allocator);
            curr_history.AddMember("process_time", process_time_str, allocator);
            
            rapidjson::Value result_str;
            result_str.SetString(collection[i].hist[j].result.buf_, allocator);
            curr_history.AddMember("result", result_str, allocator);
            
            curr_history.AddMember("response_code", collection[i].hist[j].response_code, allocator);


            rapidjson::Value args_array(rapidjson::kArrayType);
            for (int k=0; k<collection[i].hist[j].args.size(); k++) {
                rapidjson::Value curr_arg(rapidjson::kObjectType);
                rapidjson::Value name_str;
                name_str.SetString(collection[i].hist[j].args[k].name.buf_, allocator);
                curr_arg.AddMember("name", name_str, allocator);

                rapidjson::Value value_str;
                value_str.SetString(collection[i].hist[j].args[k].value.buf_, allocator);
                curr_arg.AddMember("value", value_str, allocator);

                curr_arg.AddMember("argument_type", collection[i].hist[j].args[k].arg_type, allocator);
                args_array.PushBack(curr_arg, allocator);
            }
            curr_history.AddMember("arguments", args_array, allocator);

            rapidjson::Value headers_array(rapidjson::kArrayType);
            for (int k=0; k<collection[i].hist[j].headers.size(); k++) {
                rapidjson::Value curr_header(rapidjson::kObjectType);
                rapidjson::Value name_str;
                name_str.SetString(collection[i].hist[j].headers[k].name.buf_, allocator);
                curr_header.AddMember("name", name_str, allocator);

                rapidjson::Value value_str;
                value_str.SetString(collection[i].hist[j].headers[k].value.buf_, allocator);
                curr_header.AddMember("value", value_str, allocator);

                curr_header.AddMember("argument_type", collection[i].hist[j].headers[k].arg_type, allocator);
                headers_array.PushBack(curr_header, allocator);
            }
            curr_history.AddMember("headers", headers_array, allocator);

            history_array.PushBack(curr_history, allocator);
        }
        curr_collection.AddMember("histories", history_array, allocator);
        collection_array.PushBack(curr_collection, allocator);
    }
    document.AddMember("collections", collection_array, allocator);
 
    rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

    FILE *fp = fopen(filename.buf_, "wb");
    if (fp != NULL)
    {
        fputs(strbuf.GetString(), fp);
        fclose(fp);
    }
}
