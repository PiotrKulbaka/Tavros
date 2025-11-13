#pragma once

#include <cstddef>
#include <atomic>

using uint8 = uint8_t;
using int8 = int8_t;
using uint16 = uint16_t;
using int16 = int16_t;
using uint32 = uint32_t;
using int32 = int32_t;
using uint64 = uint64_t;
using int64 = int64_t;
using size_t = std::size_t;
using char32 = char32_t;

using atomic_uint8 = std::atomic_uint8_t;
using atomic_int8 = std::atomic_int8_t;
using atomic_uint16 = std::atomic_uint16_t;
using atomic_int16 = std::atomic_int16_t;
using atomic_uint32 = std::atomic_uint32_t;
using atomic_int32 = std::atomic_int32_t;
using atomic_uint64 = std::atomic_uint64_t;
using atomic_int64 = std::atomic_int64_t;
using atomic_size_t = std::atomic_size_t;
using atomic_bool = std::atomic_bool;

constexpr size_t pointer_size = sizeof(void*);

static_assert(sizeof(uint8) == 1);
static_assert(sizeof(int8) == 1);
static_assert(sizeof(uint16) == 2);
static_assert(sizeof(int16) == 2);
static_assert(sizeof(uint32) == 4);
static_assert(sizeof(int32) == 4);
static_assert(sizeof(uint64) == 8);
static_assert(sizeof(int64) == 8);
static_assert(sizeof(size_t) == sizeof(void*));

#define TAV_UNUSED(x) ((void) (x))
