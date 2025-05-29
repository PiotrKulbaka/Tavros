#pragma once

#include <tavros/system/interfaces/application.hpp>

namespace tavros::system
{

    class application : public interfaces::application, core::noncopyable, core::nonmovable
    {
    public:
        application();

        virtual ~application() override;

        virtual void run() override;
        virtual void exit() override;
        virtual bool is_runing() override;

        virtual void poll_events() override;

    public:
        bool m_is_running;
    };

} // namespace tavros::system
