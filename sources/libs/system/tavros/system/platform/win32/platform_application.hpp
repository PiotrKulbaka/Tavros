#pragma once

#include <tavros/system/application/platform_application.hpp>

namespace tavros::system::win32
{
    void increase_windows_count();
    void decrease_windows_count();

    class platform_application final : public system::platform_application
    {
    public:
        platform_application();

        ~platform_application() override;

        void run() override;

        bool is_running() override;

        void request_exit(int exit_code) override;
        int  exit_code() override;

        void poll_events() override;
        void wait_events() override;

        math::isize2 desktop_size() override;

        uint64 highp_time_us() override;

    private:
        bool m_is_running;
        int  m_exit_code;
    };

} // namespace tavros::system::win32
