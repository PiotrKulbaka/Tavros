#include <tavros/core/logger/logger.hpp>

#include <tavros/core/debug/debug_break.hpp>

#include <exception>
#include <cstdlib>

#if TAV_COMPILER_MSVC
    #include <intrin.h>
#endif

static tavros::core::logger logger("debug");

bool g_tav_enable_utnittest_asserts = false;
bool g_tav_assert_was_called = false;

#if TAV_DEBUG
    #include <tavros/core/debug/assert.hpp>
    #include <tavros/core/debug/check.hpp>
    #include <tavros/core/debug/verify.hpp>

void tav_assertion_failed_impl(const char* expr, const char* file, int line, const char* function)
{
    if (g_tav_enable_utnittest_asserts) {
        g_tav_assert_was_called = true;
    } else {
        constexpr auto size = 1024;
        char           message[size];
        std::snprintf(message, size - 1, "Assertion failed: (%s)\n  In function: %s\n  At: %s:%d", expr, function, file, line);
        logger.fatal("{}", message);
        fprintf(stderr, "%s\n", message);
        TAV_DEBUG_BREAK();
        std::abort();
    }
}

void tav_debug_break_impl()
{
    #if TAV_COMPILER_MSVC
    __debugbreak();
    #elif TAV_COMPILER_GCC
    __builtin_trap();
    #else
    std::abort();
    #endif
}

#endif // TAV_DEBUG

void tav_verification_failed_impl(const char* expr, const char* file, int line, const char* function)
{
    constexpr auto size = 1024;
    char           message[size];
    std::snprintf(message, size - 1, "Verification failed: (%s)\n  In function: %s\n  At: %s:%d", expr, function, file, line);
    logger.fatal("{}", message);
    fprintf(stderr, "%s\n", message);
    TAV_DEBUG_BREAK();
    std::abort();
}

void tav_check_failed_impl(const char* expr, const char* file, int line, const char* function)
{
    constexpr auto size = 1024;
    char           message[size];
    std::snprintf(message, size - 1, "Check failed: (%s)\n  In function: %s\n  At: %s:%d", expr, function, file, line);
    logger.fatal("{}", message);
    fprintf(stderr, "%s\n", message);
    TAV_DEBUG_BREAK();
    std::terminate();
}

void tav_unreachable_impl(const char* file, int line, const char* function)
{
    constexpr auto size = 1024;
    char           message[size];
    std::snprintf(message, size - 1, "Unreachable code:\n  In function: %s\n  At: %s:%d", function, file, line);
    logger.fatal("{}", message);
    fprintf(stderr, "%s\n", message);

    TAV_DEBUG_BREAK();
#if TAV_COMPILER_MSVC
    __assume(0);
#elif TAV_COMPILER_GCC
    __builtin_unreachable();
#else
    std::abort();
#endif
}
