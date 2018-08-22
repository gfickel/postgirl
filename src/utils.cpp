#include "utils.h"

void readIntFromIni(int& res, FILE* fid) {
    int str_len;
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

History readHistory(FILE* fid) {
    History hist;
    readStringFromIni(hist.url.buf_, fid);
    readStringFromIni(hist.input_json.buf_, fid);
    readStringFromIni(hist.result.buf_, fid);
    readIntFromIni((int&)hist.req_type, fid);
    readIntFromIni((int&)hist.content_type, fid);
    readStringFromIni(hist.process_time.buf_, fid);
    readIntFromIni(hist.response_code, fid);
    int arg_size, header_size;
    readIntFromIni(arg_size, fid);
    readIntFromIni(header_size, fid);
    for (int i=0; i<arg_size; i++) {
        Argument arg;
        fgetc(fid); fscanf(fid, "%*s");
        readStringFromIni(arg.name.buf_, fid);
        readStringFromIni(arg.value.buf_, fid);
        readIntFromIni(arg.arg_type, fid);
        hist.args.push_back(arg);
    }
    for (int i=0; i<header_size; i++) {
        Argument arg;
        fgetc(fid); fscanf(fid, "%*s");
        readStringFromIni(arg.name.buf_, fid);
        readStringFromIni(arg.value.buf_, fid);
        readIntFromIni(arg.arg_type, fid);
        hist.headers.push_back(arg);
    }
    fgetc(fid);
    return hist;
}

pg::Vector<Collection> loadCollection(const pg::String& filename) 
{
    pg::Vector<Collection> collection_vec;
    Collection empty_col;
    FILE* fid = fopen(filename.buf_, "r");
    if (fid == NULL) 
    {
        collection_vec.push_back(empty_col);
        return collection_vec;
    }
    
    long bytes_read = 0;
    char line[32*1024];

    int ret=0, str_len=0;
    while (fgets(line, sizeof(line), fid)) {
        if (line[0] == '\n') continue;
        if (line[0] == '[') { // Collections
            Collection col;
            int num_hists;
            readStringFromIni(col.name.buf_, fid);
            readIntFromIni(num_hists, fid);
            fgetc(fid);
            for (int i=0; i<num_hists; i++) {
                fscanf(fid, "%s", line);
                History hist = readHistory(fid);
                col.hist.push_back(hist);
            }
            collection_vec.push_back(col);
        }
    }
    fclose(fid);
    if (collection_vec.size() < 1)
        collection_vec.push_back(empty_col);
    return collection_vec;
}

void saveCollection(const pg::Vector<Collection>& collection, const pg::String& filename) 
{
    FILE* fid = fopen(filename.buf_, "w");
    if (fid == NULL) return;
    
    static char line[32*1028];
    for (int i=0; i<collection.size(); i++) {
        if (i == 0) {
            sprintf(line, "[%s]\n", "Collection");
            fwrite(line, sizeof(char), strlen(line), fid);
        }
        sprintf(line, "Name (%d): %s\n", collection[i].name.length(), collection[i].name.buf_);
        fwrite(line, sizeof(char), strlen(line), fid);
        sprintf(line, "Size: %d\n\n", collection[i].hist.size());
        fwrite(line, sizeof(char), strlen(line), fid);
        for (int j=0; j<collection[i].hist.size(); j++) {
            sprintf(line, "[%s]\n", "History");
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "Url (%d): %s\n", collection[i].hist[j].url.length(), collection[i].hist[j].url.buf_);
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "InputJson (%d): %s\n", collection[i].hist[j].input_json.length(), collection[i].hist[j].input_json.buf_);
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "Result (%d): %s\n", collection[i].hist[j].result.length(), collection[i].hist[j].result.buf_);
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "RequestType: %d\n", (int)collection[i].hist[j].req_type);
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "ContentType: %d\n", (int)collection[i].hist[j].content_type);
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "ProcessTime (%d): %s", collection[i].hist[j].process_time.length(), collection[i].hist[j].process_time.buf_);
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "ResponseCode: %d\n", collection[i].hist[j].response_code);
            fwrite(line, sizeof(char), strlen(line), fid);            
            sprintf(line, "ArgumentsSize: %d\n", collection[i].hist[j].args.size());
            fwrite(line, sizeof(char), strlen(line), fid);
            sprintf(line, "HeadersSize: %d\n", collection[i].hist[j].headers.size());
            fwrite(line, sizeof(char), strlen(line), fid);

            for (int k=0; k<collection[i].hist[j].args.size(); k++) {
                sprintf(line, "\n[%s]\n", "Argument");
                fwrite(line, sizeof(char), strlen(line), fid);
                sprintf(line, "Name (%d): %s\n", collection[i].hist[j].args[k].name.length(), collection[i].hist[j].args[k].name.buf_);
                fwrite(line, sizeof(char), strlen(line), fid);
                sprintf(line, "Value (%d): %s\n", collection[i].hist[j].args[k].value.length(), collection[i].hist[j].args[k].value.buf_);
                fwrite(line, sizeof(char), strlen(line), fid);
                sprintf(line, "ArgumentType: %d\n", (int)collection[i].hist[j].args[k].arg_type);
                fwrite(line, sizeof(char), strlen(line), fid);
            }

            for (int k=0; k<collection[i].hist[j].headers.size(); k++) {
                sprintf(line, "\n[%s]\n", "Header");
                fwrite(line, sizeof(char), strlen(line), fid);
                sprintf(line, "Name (%d): %s\n", collection[i].hist[j].headers[k].name.length(), collection[i].hist[j].headers[k].name.buf_);
                fwrite(line, sizeof(char), strlen(line), fid);
                sprintf(line, "Value (%d): %s\n", collection[i].hist[j].headers[k].value.length(), collection[i].hist[j].headers[k].value.buf_);
                fwrite(line, sizeof(char), strlen(line), fid);
                sprintf(line, "ArgumentType: %d\n", (int)collection[i].hist[j].headers[k].arg_type);
                fwrite(line, sizeof(char), strlen(line), fid);
            }
            sprintf(line, "\n");
            fwrite(line, sizeof(char), strlen(line), fid);
        }
    }
    fclose(fid);
}
