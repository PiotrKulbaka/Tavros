#pragma once

#include <tavros/core/defines.hpp>

[[noreturn]] void tav_check_failed_impl(const char* expr, const char* file, int line, const char* function);

#define TAV_CHECK(expr)                                                 \
    do {                                                                \
        if (!(expr)) {                                                  \
            tav_check_failed_impl(#expr, __FILE__, __LINE__, __func__); \
        }                                                               \
    } while (0)
