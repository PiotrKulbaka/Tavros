#include <tavros/system/application.hpp>

#include <tavros/system/application/platform_application.hpp>

namespace tavros::system
{

    application::application()
        : m_app(platform_application::create())
    {
        m_app->run();
    }

    application::~application()
    {
        m_app = nullptr;
    }

    void application::request_exit(int exit_code)
    {
        m_app->request_exit(exit_code);
    }

    int application::run()
    {
        m_app->poll_events();
        while (m_app->is_running()) {
            m_app->wait_events();
            m_app->poll_events();
        }

        return m_app->exit_code();
    }

    math::isize2 application::desktop_size()
    {
        return m_app->desktop_size();
    }

    uint64 application::highp_time_us()
    {
        return m_app->highp_time_us();
    }

} // namespace tavros::system
