#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>


#define DEFAULT_STRING_SIZE 4098

namespace pg {

class String {
public:
    inline String() 
    { 
        capacity_ = DEFAULT_STRING_SIZE; 
        buf_ = (char*)malloc((size_t)DEFAULT_STRING_SIZE * sizeof(char));
        buf_[0] = '\0';
    }
    inline String(int capacity) 
    { 
        capacity_ = capacity; 
        buf_ = (char*)malloc((size_t)capacity_ * sizeof(char));
        buf_[0] = '\0';
    }

    inline String(const char* str) {
        int size = strlen(str)+1;
        int cap = DEFAULT_STRING_SIZE;
        if (size > cap) cap = size;
        capacity_ = cap;
        buf_ = (char*)malloc((size_t)capacity_ * sizeof(char));
        memcpy(buf_, str, (size_t)size * sizeof(char)); 
    }

    inline String(char* str, int size) {
        int cap = DEFAULT_STRING_SIZE;
        if (size > cap) cap = size;
        capacity_ = cap;
        buf_ = (char*)malloc((size_t)capacity_ * sizeof(char));
        memcpy(buf_, str, (size_t)size * sizeof(char)); 
    }

    inline ~String()   { if(buf_) free(buf_); }
    inline String(const String& src) 
    { 
        capacity_ = DEFAULT_STRING_SIZE; 
        buf_ = (char*)malloc((size_t)capacity_ * sizeof(char));
        operator=(src);
    }

    inline String& operator=(const String& src) 
    {
        int size = strlen(src.buf_)+1;
        if (size >= capacity_) {
            capacity_ = src.capacity_;
            if (buf_) free(buf_);
            buf_ = (char*)malloc((size_t)capacity_ * sizeof(char));
        }
        memcpy(buf_, src.buf_, (size_t)size * sizeof(char));
        return *this;
    }

    inline int              capacity() const            { return capacity_; }
    inline char&            operator[](int i)           { assert(i < capacity_); return buf_[i]; }
    inline const char&      operator[](int i) const     { assert(i < capacity_); return buf_[i]; }

    int capacity_;
    char* buf_;
};



} 
