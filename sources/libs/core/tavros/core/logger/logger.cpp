#include <tavros/core/logger/logger.hpp>

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/debug/assert.hpp>

#include <atomic>
#include <mutex>
#include <cstdio>
#include <thread>
#include <chrono>

namespace
{
    struct log_helper
    {
        tavros::core::severity_level                              level = tavros::core::severity_level::debug;
        tavros::core::vector<tavros::core::logger::consumer_type> consumers;
        std::mutex                                                mtx;
        std::atomic_bool                                          has_consumers = false;
    };

    static log_helper g_log;

    const char* now_time()
    {
        static const char           alpha[] = "0123456789";
        thread_local static char    str_time[] = "00:00:00.000";
        thread_local static int64_t last_ms = 0;
        thread_local static int64_t last_s = 0;
        thread_local static int64_t last_m = 0;
        thread_local static int64_t last_h = 0;

        using namespace std::chrono;

        // Milliseconds
        auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (ms == last_ms) {
            return str_time;
        } else {
            last_ms = ms;
            str_time[11] = alpha[(ms / 1) % 10];
            str_time[10] = alpha[(ms / 10) % 10];
            str_time[9] = alpha[(ms / 100) % 10];
        }

        // Seconds
        auto s = last_ms / 1000;
        if (s == last_s) {
            return str_time;
        } else {
            last_s = s;
            s %= 60;
            str_time[7] = alpha[(s / 1) % 10];
            str_time[6] = alpha[(s / 10) % 10];
        }

        // Minutes
        auto m = last_s / 60;
        if (m == last_m) {
            return str_time;
        } else {
            last_m = m;
            m %= 60;
            str_time[4] = alpha[(m / 1) % 10];
            str_time[3] = alpha[(m / 10) % 10];
        }

        // Hours
        auto h = last_m / 60;
        if (h == last_h) {
            return str_time;
        } else {
            last_h = h;
            h %= 24;
            str_time[1] = alpha[(h / 1) % 10];
            str_time[0] = alpha[(h / 10) % 10];
        }

        return str_time;
    }

    void log_vprint(tavros::core::severity_level level, tavros::core::string_view tag, const char* fmt, va_list args)
    {
        TAV_ASSERT(tag.data());
        TAV_ASSERT(fmt);

        if (level < g_log.level || !g_log.has_consumers.load(std::memory_order_relaxed)) {
            return;
        }

        static constexpr auto size = 4096;
        char                  message[size];
        static const char*    level_strings[] = {
            "debug",
            "info",
            "warning",
            "error"
        };

        const auto* now_s = now_time();
        const auto* lvl_s = level_strings[static_cast<uint32_t>(level)];

        auto len = std::snprintf(message, size - 1, "[tavros|%s|%s][%s] ", now_s, lvl_s, tag.data());
        if (len < size) {
            len += std::vsnprintf(message + len, static_cast<size_t>(size - len - 1), fmt, args);
        }

        std::scoped_lock<std::mutex> lock(g_log.mtx);
        for (auto& consumer : g_log.consumers) {
            consumer(level, tag, tavros::core::string_view(message, static_cast<size_t>(len)));
        }
    }

    inline void log_print(tavros::core::severity_level level, tavros::core::string_view tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log_vprint(level, tag, fmt, args);
        va_end(args);
    }
} // namespace

using namespace tavros::core;

logger::logger(string_view tag) noexcept
    : m_tag(tag)
{
}

void logger::debug(const char* fmt, ...)
{
    log_print(severity_level::debug, m_tag, fmt);
}

void logger::info(const char* fmt, ...)
{
    log_print(severity_level::info, m_tag, fmt);
}

void logger::warning(const char* fmt, ...)
{
    log_print(severity_level::warning, m_tag, fmt);
}

void logger::error(const char* fmt, ...)
{
    log_print(severity_level::error, m_tag, fmt);
}

string_view logger::tag() const
{
    return m_tag;
}

void logger::print(severity_level level, const char* tag, const char* fmt, ...)
{
    log_print(level, tag, fmt);
}

void logger::vprint(severity_level level, const char* tag, const char* fmt, va_list args)
{
    log_vprint(level, tag, fmt, args);
}

void logger::set_severity_level(severity_level level)
{
    g_log.level = level;
}

void logger::add_consumer(const consumer_type& consumer)
{
    std::scoped_lock<std::mutex> lock(g_log.mtx);
    g_log.consumers.push_back(consumer);
    g_log.has_consumers.store(true, std::memory_order_relaxed);
}
