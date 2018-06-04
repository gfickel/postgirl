// Original code from Omar Cornut's Dear ImGui (https://github.com/ocornut/imgui)

// This is just an extention of imgui ImVector class. Basicly I wanted to be used more like a std::vector,
// so I'm droping some performance by not using memcpy, however I don't have to deal the the huge stack traces 
// nor the increased compilation time.

#pragma once
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>  


namespace pg {

template<typename T>
class Vector
{
public:
    int                         Size;
    int                         Capacity;
    T*                          Data;

    typedef T                   value_type;
    typedef value_type*         iterator;
    typedef const value_type*   const_iterator;

    inline Vector()           { Size = Capacity = 0; Data = NULL; }
    inline ~Vector()          { if (Data) delete [] Data; }
    inline Vector(const Vector<T>& src)                     { Size = Capacity = 0; Data = NULL; operator=(src); }
    inline Vector& operator=(const Vector<T>& src)          { clear(); resize(src.Size); for (int i=0; i<src.Size; i++) Data[i] = src.Data[i]; return *this; }

    inline bool                 empty() const                   { return Size == 0; }
    inline int                  size() const                    { return Size; }
    inline int                  capacity() const                { return Capacity; }
    inline value_type&          operator[](int i)               { assert(i < Size); return Data[i]; }
    inline const value_type&    operator[](int i) const         { assert(i < Size); return Data[i]; }

    inline void                 clear()                         { if (Data) { Size = Capacity = 0; delete [] Data; Data = NULL; } }
    inline iterator             begin()                         { return Data; }
    inline const_iterator       begin() const                   { return Data; }
    inline iterator             end()                           { return Data + Size; }
    inline const_iterator       end() const                     { return Data + Size; }
    inline value_type&          front()                         { assert(Size > 0); return Data[0]; }
    inline const value_type&    front() const                   { assert(Size > 0); return Data[0]; }
    inline value_type&          back()                          { assert(Size > 0); return Data[Size - 1]; }
    inline const value_type&    back() const                    { assert(Size > 0); return Data[Size - 1]; }

    inline int          _grow_capacity(int sz) const            { int new_capacity = Capacity ? (Capacity + Capacity/2) : 8; return new_capacity > sz ? new_capacity : sz; }
    inline void         resize(int new_size)                    { if (new_size > Capacity) reserve(_grow_capacity(new_size)); Size = new_size; }
    inline void         resize(int new_size,const value_type& v){ if (new_size > Capacity) reserve(_grow_capacity(new_size)); if (new_size > Size) for (int n = Size; n < new_size; n++) Data[n] = v; Size = new_size; }
    inline void         reserve(int new_capacity)
    {
        if (new_capacity <= Capacity)
            return;
        value_type* new_data = (value_type*)new value_type[new_capacity];
        if (Data)
        {
            for (int i=0; i<Size; i++)
                new_data[i] = Data[i];
            delete [] Data;
        }
        Data = new_data;
        Capacity = new_capacity;
    }

    // NB: It is forbidden to call push_back/push_front/insert with a reference pointing inside the Vector data itself! e.g. v.push_back(v[10]) is forbidden.
    inline void         push_back(const value_type& v)                  { if (Size == Capacity) reserve(_grow_capacity(Size + 1)); Data[Size] = v; Size++;}
    inline void         pop_back()                                      { assert(Size > 0); Size--; }
    inline iterator     erase(const_iterator it)                        { assert(it >= Data && it < Data+Size); const ptrdiff_t off = it - Data; memmove(Data + off, Data + off + 1, ((size_t)Size - (size_t)off - 1) * sizeof(value_type)); Size--; return Data + off; }
    inline iterator     erase(const_iterator it, const_iterator it_last){ assert(it >= Data && it < Data+Size && it_last > it && it_last <= Data+Size); const ptrdiff_t count = it_last - it; const ptrdiff_t off = it - Data; memmove(Data + off, Data + off + count, ((size_t)Size - (size_t)off - count) * sizeof(value_type)); Size -= (int)count; return Data + off; }
    inline bool         contains(const value_type& v) const             { const T* data = Data;  const T* data_end = Data + Size; while (data < data_end) if (*data++ == v) return true; return false; }
};

}
