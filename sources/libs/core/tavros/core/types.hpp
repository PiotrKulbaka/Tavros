#pragma once

#include <cstddef>

namespace tavros
{
    using uint8 = unsigned char;
    using int8 = signed char;
    using uint16 = unsigned short;
    using int16 = signed short;
    using uint32 = unsigned int;
    using int32 = signed int;
    using uint64 = unsigned long long;
    using int64 = long long;
    using size_t = std::size_t;

    static_assert(sizeof(uint8) == 1);
    static_assert(sizeof(int8) == 1);
    static_assert(sizeof(uint16) == 2);
    static_assert(sizeof(int16) == 2);
    static_assert(sizeof(uint32) == 4);
    static_assert(sizeof(int32) == 4);
    static_assert(sizeof(uint64) == 8);
    static_assert(sizeof(int64) == 8);
    static_assert(sizeof(size_t) == sizeof(void*));
} // namespace tavros
