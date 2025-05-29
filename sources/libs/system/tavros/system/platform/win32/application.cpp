#include <tavros/system/platform/win32/application.hpp>

#include <Windows.h>

using namespace tavros::system;

application_uptr interfaces::application::create()
{
    return tavros::core::make_unique<tavros::system::application>();
}

application::application()
    : m_is_running(false)
{
}

application::~application()
{
}

void application::run()
{
    m_is_running = true;
}

void application::exit()
{
    m_is_running = false;
}

bool application::is_runing()
{
    return m_is_running;
}

void application::poll_events()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            exit();
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
