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
        bool                                                      has_consumers = false;
    };

    static log_helper g_log;
} // namespace

namespace tavros::core
{

    void logger::set_severity_level(severity_level level) noexcept
    {
        g_log.level = level;
    }

    void logger::add_consumer(const consumer_type& consumer) noexcept
    {
        std::scoped_lock<std::mutex> lock(g_log.mtx);
        g_log.consumers.push_back(consumer);
        g_log.has_consumers = true;
    }

    bool logger::allowed_print(severity_level level) noexcept
    {
        return level >= g_log.level && g_log.has_consumers;
    }

    string_view logger::now_time() noexcept
    {
        static const char         alpha[] = "0123456789";
        thread_local static char  str_time[] = "00:00:00.000";
        thread_local static int64 last_ms = 0;
        thread_local static int64 last_s = 0;
        thread_local static int64 last_m = 0;
        thread_local static int64 last_h = 0;

        using namespace std::chrono;

        // Milliseconds
        auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (ms == last_ms) {
            return string_view(str_time, sizeof(str_time) - 1);
        } else {
            last_ms = ms;
            str_time[11] = alpha[(ms / 1) % 10];
            str_time[10] = alpha[(ms / 10) % 10];
            str_time[9] = alpha[(ms / 100) % 10];
        }

        // Seconds
        auto s = last_ms / 1000;
        if (s == last_s) {
            return string_view(str_time, sizeof(str_time) - 1);
        }
        last_s = s;
        s %= 60;
        str_time[7] = alpha[(s / 1) % 10];
        str_time[6] = alpha[(s / 10) % 10];


        // Minutes
        auto m = last_s / 60;
        if (m == last_m) {
            return string_view(str_time, sizeof(str_time) - 1);
        }
        last_m = m;
        m %= 60;
        str_time[4] = alpha[(m / 1) % 10];
        str_time[3] = alpha[(m / 10) % 10];

        // Hours
        auto h = last_m / 60;
        if (h == last_h) {
            return string_view(str_time, sizeof(str_time) - 1);
        }
        last_h = h;
        h %= 24;
        str_time[1] = alpha[(h / 1) % 10];
        str_time[0] = alpha[(h / 10) % 10];

        return string_view(str_time, sizeof(str_time) - 1);
    }

    void logger::print_message(severity_level level, string_view tag, string_view msg) noexcept
    {
        std::scoped_lock<std::mutex> lock(g_log.mtx);
        for (auto& consumer : g_log.consumers) {
            consumer(level, tag, msg);
        }
    }

} // namespace tavros::core