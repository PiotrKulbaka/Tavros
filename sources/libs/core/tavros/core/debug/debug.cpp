#include <tavros/core/logger/logger.hpp>
#include <tavros/core/string.hpp>

#include <tavros/core/debug/debug.hpp>

#include <exception>
#include <cstdlib>

#include <fstream>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>

#if TAV_COMPILER_MSVC
    #include <intrin.h>
#endif

namespace
{
    tavros::core::logger logger("debug");
    tavros::core::string g_debug_path;
} // namespace

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

namespace tavros::debug
{
    void set_debug_path(core::string_view path) noexcept
    {
        g_debug_path = path;
    }

    core::string_view debug_path() noexcept
    {
        return g_debug_path;
    }

    void save_to_file(core::buffer_view<const uint8_t> data, core::string_view name, core::string_view ext) noexcept
    {
        try {
            namespace fs = std::filesystem;

            fs::path dir_path = debug_path().empty()
                                  ? fs::path(".")
                                  : fs::path(debug_path().data());

            try {
                if (!dir_path.empty()) {
                    if (dir_path.has_extension()) {
                        fs::create_directories(dir_path.parent_path());
                    } else {
                        fs::create_directories(dir_path);
                    }
                }
            } catch (const fs::filesystem_error&) {
                // Ignore errors
            }

            std::string filename;
            if (name.empty()) {
                auto now = std::chrono::system_clock::now();
                auto time_t_now = std::chrono::system_clock::to_time_t(now);
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now.time_since_epoch()
                          )
                              .count()
                        % 1000;

                std::tm tm_buf{};
#ifdef _WIN32
                localtime_s(&tm_buf, &time_t_now);
#else
                localtime_r(&time_t_now, &tm_buf);
#endif

                std::ostringstream ss;
                ss << std::put_time(&tm_buf, "%Y%m%d_%H%M%S");
                ss << "_" << std::setw(3) << std::setfill('0') << ms;

                filename = ss.str();

                std::string extension = ext.empty() ? ".bin" : (ext.front() == '.' ? std::string(ext.data(), ext.size()) : "." + std::string(ext.data(), ext.size()));

                fs::path test_path = dir_path / (filename + extension);
                int      salt = 0;
                while (fs::exists(test_path)) {
                    ++salt;
                    std::ostringstream salted_ss;
                    salted_ss << std::put_time(&tm_buf, "%Y%m%d_%H%M%S");
                    salted_ss << "_" << std::setw(3) << std::setfill('0') << ms;
                    salted_ss << "_" << salt;
                    filename = salted_ss.str();
                    test_path = dir_path / (filename + extension);
                }
            } else {
                filename = std::string(name.data(), name.size());
            }

            std::string extension = ext.empty() ? "bin" : std::string(ext.data(), ext.size());
            if (!extension.empty() && extension.front() != '.') {
                extension.insert(extension.begin(), '.');
            }

            fs::path full_path = dir_path / (filename + extension);

            std::ofstream ofs(full_path, std::ios::binary);
            if (!ofs) {
                logger.error("Failed to open debug file '{}'", full_path.string());
                return;
            }

            if (!data.empty()) {
                ofs.write(
                    reinterpret_cast<const char*>(data.data()),
                    static_cast<std::streamsize>(data.size())
                );
                if (!ofs) {
                    logger.error("Failed to write debug file '{}'", full_path.string());
                    return;
                }
            }

            logger.info("Saved debug file to '{}'", full_path.string());

        } catch (const std::exception& e) {
            logger.error("Exception in save_to_file(): {}", e.what());
        } catch (...) {
            logger.error("Unknown exception in save_to_file()");
        }
    }
} // namespace tavros::debug