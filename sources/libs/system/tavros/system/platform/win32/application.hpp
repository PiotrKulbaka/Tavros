#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/system/application.hpp>
#include <tavros/system/window.hpp>

namespace tavros::system::win32
{
    void increase_windows_count();
    void decrease_windows_count();

    class application : public tavros::system::application
    {
    public:
        application();

        ~application() override;

        int  run() override;
        void exit(int exit_code) override;
        void poll_events() override;
        void wait_events() override;

        math::isize2 desktop_size() override;

    private:
        bool m_is_running;
        int  m_exit_code;
    };

} // namespace tavros::system::win32
