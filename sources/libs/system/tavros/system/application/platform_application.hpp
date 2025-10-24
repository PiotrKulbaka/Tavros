#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/math/ivec2.hpp>

namespace tavros::system
{

    class platform_application : core::noncopyable, core::nonmovable
    {
    public:
        static core::unique_ptr<platform_application> create();

    public:
        virtual ~platform_application() = default;

        virtual void run() = 0;

        virtual bool is_running() = 0;

        virtual void request_exit(int exit_code) = 0;
        virtual int  exit_code() = 0;

        virtual void poll_events() = 0;
        virtual void wait_events() = 0;

        virtual math::isize2 desktop_size() = 0;

        virtual uint64 highp_time_us() = 0;
    };

} // namespace tavros::system
