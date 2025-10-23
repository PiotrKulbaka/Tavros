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
        void exit() override;
        bool is_runing() override;

        void poll_events() override;

        void wait_events() override;

        math::isize2 desktop_size() override;

    private:
        bool m_is_running;
    };

} // namespace tavros::system::win32
