#include <tavros/core/debug/profiler.hpp>

#include <tavros/core/types.hpp>
#include <tavros/core/containers/fixed_vector.hpp>
#include <chrono>
#include <mutex>

#if !!(TAV_PROFILER_ENABLED)

namespace
{
    const auto                g_start_time = std::chrono::high_resolution_clock::now();
    const thread_local uint64 g_thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

    uint64 cur_time_ns() noexcept
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now - g_start_time).count();
    }

    constexpr size_t                                                  k_max_sinks = 8;
    tavros::core::fixed_vector<tavros::profiler::sink_t, k_max_sinks> g_sinks;

    void push_perf_event(const tavros::profiler::perf_event& e) noexcept
    {
        for (const auto& sink : g_sinks) {
            sink(e);
        }
    }
} // namespace

#endif

namespace tavros::profiler
{
    void add_sink(sink_t s) noexcept
    {
#if !!(TAV_PROFILER_ENABLED)
        g_sinks.push_back(std::move(s));
#else
        TAV_UNUSED(s);
#endif // TAV_PROFILER_ENABLED
    }
} // namespace tavros::profiler

#if !!(TAV_PROFILER_ENABLED)

namespace tavros::profiler::detail
{
    void thread_name(const char* name) noexcept
    {
        perf_event e{.tag = name, .time_ns = cur_time_ns(), .thread_id = g_thread_id, .value = 0.0, .type = event_type::thread_name};
        ::push_perf_event(e);
    }

    void zone_begin(const char* tag) noexcept
    {
        perf_event e{.tag = tag, .time_ns = cur_time_ns(), .thread_id = g_thread_id, .value = 0.0, .type = event_type::zone_begin};
        ::push_perf_event(e);
    }

    void zone_end(const char* tag) noexcept
    {
        perf_event e{.tag = tag, .time_ns = cur_time_ns(), .thread_id = g_thread_id, .value = 0.0, .type = event_type::zone_end};
        ::push_perf_event(e);
    }

    void counter(const char* tag, double value) noexcept
    {
        perf_event e{.tag = tag, .time_ns = cur_time_ns(), .thread_id = g_thread_id, .value = value, .type = event_type::counter};
        ::push_perf_event(e);
    }
} // namespace tavros::profiler::detail

#endif // TAV_PROFILER_ENABLED
