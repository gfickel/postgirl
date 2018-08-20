#include <stdio.h>
#include "requests.h"
#include "pgstring.h"
#include "pgvector.h"


void readIntFromIni(int& res, FILE* fid);

void readStringFromIni(char* buffer, FILE* fid);

void printArg(const Argument& arg);

void printHistory(const History& hist);

History readHistory(FILE* fid);

pg::Vector<Collection> loadCollection(const pg::String& filename);

void saveCollection(const pg::Vector<Collection>& collection, const pg::String& filename);

