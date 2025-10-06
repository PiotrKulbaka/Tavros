#include <tavros/system/platform/win32/application.hpp>

#include <Windows.h>
#include <shellscalingapi.h>

void enable_high_dpi_awareness()
{
    // Windows 10 Anniversary Update or later
    if (auto hUser32 = GetModuleHandleW(L"user32.dll")) {
        if (auto setDpiAwarenessContext =
                reinterpret_cast<BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT)>(
                    GetProcAddress(hUser32, "SetProcessDpiAwarenessContext")
                )) {
            if (setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                return;
            }
        }
    }

    // Windows 8.1+
    if (auto hShcore = GetModuleHandleW(L"Shcore.dll")) {
        if (auto setProcessDpiAwareness =
                reinterpret_cast<HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS)>(
                    GetProcAddress(hShcore, "SetProcessDpiAwareness")
                )) {
            setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
            return;
        }
    }

    // Windows Vista / 7 / 8
    if (auto hUser32 = GetModuleHandleW(L"user32.dll")) {
        if (auto setProcessDPIAware =
                reinterpret_cast<BOOL(WINAPI*)()>(
                    GetProcAddress(hUser32, "SetProcessDPIAware")
                )) {
            setProcessDPIAware();
            return;
        }
    }
}

using namespace tavros::system;

application_uptr interfaces::application::create()
{
    return tavros::core::make_unique<tavros::system::application>();
}

application::application()
    : m_is_running(false)
{
    enable_high_dpi_awareness();
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

void application::wait_events()
{
    MsgWaitForMultipleObjects(0, nullptr, FALSE, INFINITE, QS_ALLINPUT);
}

tavros::math::isize2 application::desktop_size()
{
    RECT       desktopRect;
    const HWND hDesktop = GetDesktopWindow();
    if (GetWindowRect(hDesktop, &desktopRect)) {
        int32 width = desktopRect.right - desktopRect.left;
        int32 height = desktopRect.bottom - desktopRect.top;
        return {width, height};
    }
    return {0, 0};
}
