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

    inline String(const char* str, int size) {
        int cap = DEFAULT_STRING_SIZE;
        if (size + 1 > cap) cap = size + 1;
        capacity_ = cap;
        buf_ = (char*)malloc((size_t)capacity_ * sizeof(char));
        memcpy(buf_, str, (size_t)size * sizeof(char)); 
        buf_[size] = '\0';
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

    inline void set(const char* str)
    {
        int size = strlen(str)+1;
        if (size >= capacity_)
            realloc(size+1);
            
        memcpy(buf_, str, (size_t)size * sizeof(char));
    }

    inline void append(const char* str, int size=-1)
    {
        if (size<0) size = strlen(str)+1;
        int curr_len = strlen(buf_);

        if (curr_len + size > capacity_) realloc(curr_len+size+1);
        memcpy(buf_+curr_len, str, size);
    }

    inline void append(String str)
    {
        append(str.buf_);
    }

    inline void realloc(int size)
    {
        if (buf_) free(buf_);
        capacity_ = size;
        buf_ = (char*)malloc((size_t)capacity_ * sizeof(char));
    }

    inline char* end()
    {
        return buf_+strlen(buf_);
    }

    inline int              capacity() const            { return capacity_; }
    inline char&            operator[](int i)           { assert(i < capacity_); return buf_[i]; }
    inline const char&      operator[](int i) const     { assert(i < capacity_); return buf_[i]; }
    inline int              length() const              { return strlen(buf_); }

    int capacity_;
    char* buf_;
};



} 
