#pragma once

#include <tavros/core/defines.hpp>

#if TAV_DEBUG
[[noreturn]] void tav_assertion_failed_impl(const char* expr, const char* file, int line, const char* function);
    #define TAV_ASSERT(expr)                                                    \
        do {                                                                    \
            if (!(expr)) {                                                      \
                tav_assertion_failed_impl(#expr, __FILE__, __LINE__, __func__); \
            }                                                                   \
        } while (0)
#else
    #define TAV_ASSERT(expr) ((void) 0)
#endif
