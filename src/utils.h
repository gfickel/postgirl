#include <stdio.h>
#include "requests.h"
#include "pgstring.h"
#include "pgvector.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "imgui.h"


void readIntFromIni(int& res, FILE* fid);

void readStringFromIni(char* buffer, FILE* fid);

void printArg(const Argument& arg);

void printHistory(const History& hist);

History readHistory(FILE* fid);

pg::Vector<Collection> loadCollection(const pg::String& filename);

void saveCollection(const pg::Vector<Collection>& collection, const pg::String& filename);

const char* Stristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end);

void Help(const char* desc);
