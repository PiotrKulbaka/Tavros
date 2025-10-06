#pragma once

#include <tavros/system/interfaces/application.hpp>

namespace tavros::system
{

    class application : public interfaces::application, core::noncopyable, core::nonmovable
    {
    public:
        application();

        ~application() override;

        void run() override;
        void exit() override;
        bool is_runing() override;

        void poll_events() override;

        void wait_events() override;

        math::isize2 desktop_size() override;

    public:
        bool m_is_running;
    };

} // namespace tavros::system
