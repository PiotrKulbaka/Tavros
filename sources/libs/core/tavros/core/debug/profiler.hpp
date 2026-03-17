#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/core/types.hpp>

#include <functional>

namespace tavros::profiler
{
    enum class event_type : uint8_t
    {
        thread_name,
        zone_begin,
        zone_end,
        counter,
    };

    struct alignas(64) perf_event
    {
        const char* tag;
        uint64      time_ns;
        uint64      thread_id;
        double      value; // for counter
        event_type  type;
    };

    static_assert(sizeof(perf_event) == 64);
    static_assert(alignof(perf_event) == 64);

    using sink_t = std::function<void(const perf_event&)>;
    void add_sink(sink_t sink) noexcept;

} // namespace tavros::profiler

#ifndef TAV_PROFILER_ENABLED
    #define TAV_PROFILER_ENABLED 0
#endif // TAV_PROFILER_ENABLED

#if !!(TAV_PROFILER_ENABLED)

namespace tavros::profiler::detail
{
    void thread_name(const char* name) noexcept;
    void zone_begin(const char* tag) noexcept;
    void zone_end(const char* tag) noexcept;
    void counter(const char* tag, double value) noexcept;

    struct scope : core::noncopyable, core::nonmovable
    {
        const char* tag;
        bool        ended;

        explicit scope(const char* tag) noexcept
            : tag(tag)
            , ended(false)
        {
            zone_begin(tag);
        }

        ~scope() noexcept
        {
            end();
        }

        void end() noexcept
        {
            if (!ended) {
                zone_end(tag);
                ended = true;
            }
        }
    };
} // namespace tavros::profiler::detail

    #define TAV_PROFILE_CONCAT_2(a, b) a##b
    #define TAV_PROFILE_CONCAT(a, b)   TAV_PROFILE_CONCAT_2(a, b)

    #define TAV_PROFILE_THREAD_NAME(name) \
        ::tavros::profiler::detail::thread_name(name)

    #define TAV_PROFILE_SCOPE(tag) \
        ::tavros::profiler::detail::scope TAV_PROFILE_CONCAT(_profiler_scope_, __LINE__)(tag)

    #define TAV_PROFILE_BEGIN(varname, tag) \
        ::tavros::profiler::detail::scope TAV_PROFILE_CONCAT(_profiler_block_, varname)(tag)

    #define TAV_PROFILE_END(varname) \
        TAV_PROFILE_CONCAT(_profiler_block_, varname).end()

    #define TAV_PROFILE_COUNTER(tag, value) \
        ::tavros::profiler::detail::counter(tag, static_cast<double>(value))

#else

    #define TAV_PROFILE_THREAD_NAME(name)
    #define TAV_PROFILE_SCOPE(tag)
    #define TAV_PROFILE_BEGIN(varname, tag)
    #define TAV_PROFILE_END(varname)
    #define TAV_PROFILE_COUNTER(tag, value)

#endif // TAVROS_PROFILER_ENABLED
