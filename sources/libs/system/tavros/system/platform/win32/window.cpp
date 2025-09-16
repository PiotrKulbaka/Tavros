#include <tavros/system/platform/win32/window.hpp>

#include <tavros/system/platform/win32/utils.hpp>


#include <Windowsx.h>
#include <CommCtrl.h>
#include <hidusage.h>

#pragma comment(linker, \
                "\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' \
    version='6.0.0.0' \
    processorArchitecture='*' \
    publicKeyToken='6595b64144ccf1df' \
    language='*'\"")

using namespace tavros::system;


namespace
{
    tavros::core::logger  logger("window");
    constexpr const char* wnd_class_name = "AppClassWindow";

    LRESULT CALLBACK wnd_proc_window(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (window* wnd = reinterpret_cast<tavros::system::window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA))) {
            return wnd->process_window_message(hWnd, uMsg, wParam, lParam);
        }
        // Does not log here to avoid spam, because some messages can be sent before GWLP_USERDATA is set
        // logger.warning("Event '%s' occurred but the window was not found.", wm_message_to_str(uMsg));
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    bool register_main_windowclass()
    {
        static WNDCLASSEXA wcex;
        static bool        is_class_registered = false;

        if (is_class_registered) {
            return true;
        }

        memset(&wcex, 0, sizeof(WNDCLASSEXA));
        wcex.cbSize = sizeof(WNDCLASSEXA);
        wcex.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = wnd_proc_window;
        wcex.hInstance = GetModuleHandle(nullptr);
        wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH) COLOR_WINDOW;
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = wnd_class_name;
        wcex.hIconSm = nullptr;

        if (!RegisterClassEx(&wcex)) {
            logger.error("Register window class '%s': ", last_win_error_str());
            return false;
        }
        logger.info("Register window class '%s': succeeded", wnd_class_name);

        is_class_registered = true;
        return true;
    }

    void register_raw_inpput_mouse(HWND hWnd)
    {
        RAWINPUTDEVICE mouse;
        mouse.usUsagePage = HID_USAGE_PAGE_GENERIC;
        mouse.usUsage = HID_USAGE_GENERIC_MOUSE;
        mouse.dwFlags = 0;
        mouse.hwndTarget = hWnd;
        if (RegisterRawInputDevices(&mouse, 1, sizeof(mouse))) {
            // Don't log this, because it is not critical
            //::logger.debug("Register raw input mouse: succeeded");
        } else {
            ::logger.error("Register raw input mouse: %s", last_win_error_str());
        }
    }

    void remove_raw_inpput_mouse(HWND hWnd)
    {
        RAWINPUTDEVICE mouse;
        mouse.usUsagePage = HID_USAGE_PAGE_GENERIC;
        mouse.usUsage = HID_USAGE_GENERIC_MOUSE;
        mouse.dwFlags = RIDEV_REMOVE;
        mouse.hwndTarget = nullptr; // if hwndTarget is hWnd, it will fail with ERROR

        if (RegisterRawInputDevices(&mouse, 1, sizeof(mouse))) {
            // Don't log this, because it is not critical
            //::logger.debug("Remove raw input mouse: succeeded");
        } else {
            ::logger.error("Remove raw input mouse: %s", last_win_error_str());
        }
    }

    void destroy_window(HWND hWnd)
    {
        char name[256];
        GetWindowText(hWnd, name, sizeof(name));

        // No need to remove raw input, it is removed automatically on window destruction
        // because remove_raw_inpput_mouse() will remove raw input for all windows (for this process)
        // remove_raw_inpput_mouse(hWnd);
        if (DestroyWindow(hWnd)) {
            ::logger.info("Destroy window '%s': succeeded", name);
        } else {
            ::logger.error("Destroy window '%s': %s", name, last_win_error_str());
        }
    }
} // namespace


tavros::core::unique_ptr<tavros::system::interfaces::window> tavros::system::interfaces::window::create(tavros::core::string_view name)
{
    return tavros::core::make_unique<tavros::system::window>(name);
}

window::window(tavros::core::string_view name)
    : m_hWnd(nullptr)
{
    register_main_windowclass();

    auto hWnd = CreateWindowEx(
        WS_EX_ACCEPTFILES,
        wnd_class_name,
        name.data(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (hWnd) {
        ::logger.info("Create window '%s': succeeded", name.data());
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<uint64>(this));
        m_hWnd = hWnd;
        register_raw_inpput_mouse(hWnd);
    } else {
        ::logger.error("Create window '%s': failed", name.data());
    }
}

window::~window()
{
    if (m_hWnd) {
        destroy_window(m_hWnd);
    }
}

void window::set_text(tavros::core::string_view text)
{
    if (!SetWindowText(m_hWnd, text.data())) {
        ::logger.error("Failed to set text: %s", last_win_error_str());
    }
}

tavros::system::window_state window::get_window_state()
{
    if (IsIconic(m_hWnd)) {
        return window_state::minimized;
    }
    if (IsZoomed(m_hWnd)) {
        return window_state::maximized;
    }
    return window_state::normal;
}

void window::set_window_state(tavros::system::window_state ws)
{
    switch (ws) {
    case tavros::system::window_state::minimized:
        ShowWindow(m_hWnd, SW_MINIMIZE);
        break;
    case tavros::system::window_state::maximized:
        ShowWindow(m_hWnd, SW_MAXIMIZE);
        break;
    case tavros::system::window_state::normal:
        ShowWindow(m_hWnd, SW_NORMAL);
        break;
    default:
        ::logger.error("Unknown window_state %d", static_cast<int32>(ws));
        break;
    }
    UpdateWindow(m_hWnd);
}

tavros::math::isize2 window::get_window_size()
{
    RECT rect;
    if (GetWindowRect(m_hWnd, &rect)) {
        int32 width = rect.right - rect.left;
        int32 height = rect.bottom - rect.top;
        return math::isize2(width, height);
    }
    ::logger.error("Failed to get window size: %s", last_win_error_str());
    return math::isize2();
}

void window::set_window_size(int32 width, int32 height)
{
    if (!SetWindowPos(m_hWnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER)) {
        ::logger.error("Failed to set window size: %s", last_win_error_str());
    }
}

tavros::math::ipoint2 window::get_location()
{
    RECT rect;
    if (GetWindowRect(m_hWnd, &rect)) {
        POINT pt{.x = rect.left, .y = rect.top};
        HWND  parent = GetParent(m_hWnd);
        ScreenToClient(parent, &pt);
        return math::ipoint2(pt.x, pt.y);
    }
    ::logger.error("Failed to get location: %s", last_win_error_str());
    return math::ipoint2();
}

void window::set_location(int32 left, int32 top)
{
    if (!SetWindowPos(m_hWnd, nullptr, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER)) {
        ::logger.error("Failed to set location: %s", last_win_error_str());
    }
}

tavros::math::isize2 window::get_client_size()
{
    RECT rect;
    if (GetClientRect(m_hWnd, &rect)) {
        int32 width = rect.right - rect.left;
        int32 height = rect.bottom - rect.top;
        return math::isize2(width, height);
    }
    ::logger.error("Failed to get size: %s", last_win_error_str());
    return math::isize2();
}

void window::set_client_size(int32 width, int32 height)
{
    RECT rect{.left = 0, .top = 0, .right = width, .bottom = height};
    AdjustWindowRect(&rect, GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
    if (!SetWindowPos(m_hWnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER)) {
        ::logger.error("Failed to set size: %s", last_win_error_str());
    }
}

bool window::is_enabled()
{
    return IsWindowEnabled(m_hWnd);
}

void window::set_enabled(bool enable)
{
    EnableWindow(m_hWnd, enable);
}

void window::activate()
{
    SetActiveWindow(m_hWnd);
    EnableWindow(m_hWnd, TRUE);
    UpdateWindow(m_hWnd);
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);
}

void window::show()
{
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
}

void window::hide()
{
    ShowWindow(m_hWnd, SW_HIDE);
}

void window::close()
{
    PostMessage(m_hWnd, WM_CLOSE, 0, 0);
}

void window::set_on_close_listener(close_callback cb)
{
    m_on_close_cb = std::move(cb);
}

void window::set_on_activate_listener(event_callback cb)
{
    m_on_activate_cb = std::move(cb);
}

void window::set_on_deactivate_listener(event_callback cb)
{
    m_on_deactivate_cb = std::move(cb);
}

void window::set_on_drop_listener(drop_callback cb)
{
    m_on_drop_cb = std::move(cb);
}

void window::set_on_move_listener(move_callback cb)
{
    m_on_move_cb = std::move(cb);
}

void window::set_on_resize_listener(size_callback cb)
{
    m_on_resize_cb = std::move(cb);
}

void window::set_on_mouse_down_listener(mouse_callback cb)
{
    m_on_mouse_down_cb = std::move(cb);
}

void window::set_on_mouse_move_listener(mouse_callback cb)
{
    m_on_mouse_move_cb = std::move(cb);
}

void window::set_on_mouse_up_listener(mouse_callback cb)
{
    m_on_mouse_up_cb = std::move(cb);
}

void window::set_on_mouse_wheel_listener(mouse_callback cb)
{
    m_on_mouse_wheel_cb = std::move(cb);
}

void window::set_on_key_down_listener(key_callback cb)
{
    m_on_key_down_cb = std::move(cb);
}

void window::set_on_key_up_listener(key_callback cb)
{
    m_on_key_up_cb = std::move(cb);
}

void window::set_on_key_press_listener(key_callback cb)
{
    m_on_key_press_cb = std::move(cb);
}

void window::on_close(close_event_args& e)
{
    if (m_on_close_cb) {
        m_on_close_cb(this, e);
    }
}

void window::on_activate()
{
    if (m_on_activate_cb) {
        m_on_activate_cb(this);
    }
}

void window::on_deactivate()
{
    if (m_on_deactivate_cb) {
        m_on_deactivate_cb(this);
    }
}

void window::on_drop(drop_event_args& e)
{
    if (m_on_drop_cb) {
        m_on_drop_cb(this, e);
    }
}

void window::on_move(move_event_args& e)
{
    if (m_on_move_cb) {
        m_on_move_cb(this, e);
    }
}

void window::on_resize(size_event_args& e)
{
    if (m_on_resize_cb) {
        m_on_resize_cb(this, e);
    }
}

void window::on_mouse_down(mouse_event_args& e)
{
    if (m_on_mouse_down_cb) {
        m_on_mouse_down_cb(this, e);
    }
}

void window::on_mouse_move(mouse_event_args& e)
{
    if (m_on_mouse_move_cb) {
        m_on_mouse_move_cb(this, e);
    }
}

void window::on_mouse_up(mouse_event_args& e)
{
    if (m_on_mouse_up_cb) {
        m_on_mouse_up_cb(this, e);
    }
}

void window::on_mouse_wheel(mouse_event_args& e)
{
    if (m_on_mouse_wheel_cb) {
        m_on_mouse_wheel_cb(this, e);
    }
}

void window::on_key_down(key_event_args& e)
{
    if (m_on_key_down_cb) {
        m_on_key_down_cb(this, e);
    }
}

void window::on_key_up(key_event_args& e)
{
    if (m_on_key_up_cb) {
        m_on_key_up_cb(this, e);
    }
}

void window::on_key_press(key_event_args& e)
{
    if (m_on_key_press_cb) {
        m_on_key_press_cb(this, e);
    }
}

handle window::get_handle() const
{
    return reinterpret_cast<handle>(m_hWnd);
}

long window::process_window_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    move_event_args  mvargs;
    size_event_args  szargs;
    mouse_event_args margs;
    key_event_args   kargs;

    switch (uMsg) {
    case WM_CLOSE: {
        close_event_args args{.cancel = false};
        on_close(args);
        if (!args.cancel) {
            if (m_hWnd) {
                destroy_window(m_hWnd);
            }
            return 0;
        }
    } break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            on_deactivate();
        } else {
            on_activate();
        }
        return 0;

    case WM_INPUT: {
        RAWINPUT raw;
        UINT     dwSize = sizeof(raw);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
            ::logger.error("GetRawInputData does not return correct size!");
        }

        if (raw.header.dwType == RIM_TYPEMOUSE) {
            RAWMOUSE& mouse = raw.data.mouse;
            if (mouse.usFlags == MOUSE_MOVE_RELATIVE && (mouse.lLastX != 0 || mouse.lLastY != 0)) {
                auto margs = mouse_event_args{
                    .button = mouse_button::none,
                    .is_double_click = false,
                    .is_relative_move = true,
                    .delta = 0,
                    .pos = math::point2(static_cast<float>(mouse.lLastX), static_cast<float>(mouse.lLastY))
                };
                on_mouse_move(margs);
                return 0;
            }
        }
    } break;

    case WM_DROPFILES: {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);

        // The best way to store pointers to strings and the strings themselves
        // in the same block of memory. So, first we calculate the total size of the
        // strings and their number.
        // At the beginning of the memory block, null-terminated pointers to the
        // null-terminated strings will be stored, and at the end of the memory block,
        // the strings themselves.

        uint32 number = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
        size_t strings_size = 0; // Total size of null-terminated strings

                                 // Calculate the total size
        for (auto i = 0; i < number; ++i) {
            // Size of the string and +1 for a null symbol
            strings_size += DragQueryFile(hDrop, i, nullptr, 0) + 1;
        }

        // Size of the array of pointers and +1 for a last null pointer
        size_t strings_pointers_size = sizeof(char*) * (number + 1);
        size_t total_size = strings_size + strings_pointers_size;
        char*  mem_block = static_cast<char*>(malloc(total_size));

        const char** files = reinterpret_cast<const char**>(mem_block);
        char*        cur_str = mem_block + strings_pointers_size;

        // Filling strings
        auto remaining_size = strings_size;
        for (auto i = 0; i < number; ++i) {
            auto num_copied_chars = DragQueryFile(hDrop, i, cur_str, remaining_size) + 1;
            files[i] = const_cast<const char*>(cur_str);
            remaining_size -= num_copied_chars;
            cur_str += num_copied_chars;
        }
        files[number] = nullptr;

        drop_event_args dargs = {.files = files};
        on_drop(dargs);

        free(mem_block);
        DragFinish(hDrop);
    }
        return 0;

    case WM_DESTROY:
        m_hWnd = nullptr;
        return 0;

    case WM_MOVE:
        mvargs = {.pos = create_point2(lParam)};
        on_move(mvargs);
        return 0;

    case WM_SIZE:
        szargs = {.size = create_size2(lParam)};
        on_resize(szargs);
        return 0;

    case WM_MOUSEMOVE:
        margs = create_mouse_event_args(lParam, mouse_button::none);
        on_mouse_move(margs);
        return 0;

    case WM_LBUTTONDOWN:
        margs = create_mouse_event_args(lParam, mouse_button::left);
        on_mouse_down(margs);
        return 0;

    case WM_LBUTTONUP:
        margs = create_mouse_event_args(lParam, mouse_button::left);
        on_mouse_up(margs);
        return 0;

    case WM_LBUTTONDBLCLK:
        margs = create_mouse_event_args(lParam, mouse_button::left, true);
        on_mouse_down(margs);
        return 0;

    case WM_RBUTTONDOWN:
        margs = create_mouse_event_args(lParam, mouse_button::right);
        on_mouse_down(margs);
        return 0;

    case WM_RBUTTONUP:
        margs = create_mouse_event_args(lParam, mouse_button::right);
        on_mouse_up(margs);
        return 0;

    case WM_RBUTTONDBLCLK:
        margs = create_mouse_event_args(lParam, mouse_button::right, true);
        on_mouse_down(margs);
        return 0;

    case WM_MBUTTONDOWN:
        margs = create_mouse_event_args(lParam, mouse_button::middle);
        on_mouse_down(margs);
        return 0;

    case WM_MBUTTONUP:
        margs = create_mouse_event_args(lParam, mouse_button::middle);
        on_mouse_up(margs);
        return 0;

    case WM_MBUTTONDBLCLK:
        margs = create_mouse_event_args(lParam, mouse_button::middle, true);
        on_mouse_down(margs);
        return 0;

    case WM_XBUTTONDOWN:
        margs = create_mouse_event_args(lParam, create_mouse_x_button(wParam));
        on_mouse_down(margs);
        return TRUE;

    case WM_XBUTTONUP:
        margs = create_mouse_event_args(lParam, create_mouse_x_button(wParam));
        on_mouse_up(margs);
        return TRUE;

    case WM_XBUTTONDBLCLK:
        margs = create_mouse_event_args(lParam, create_mouse_x_button(wParam), true);
        on_mouse_down(margs);
        return TRUE;

    case WM_MOUSEWHEEL:
        margs = create_mouse_event_args(lParam, mouse_button::none, false, GET_WHEEL_DELTA_WPARAM(wParam));
        on_mouse_wheel(margs);
        return 0;

    case WM_SYSKEYDOWN:
        kargs = create_key_event_args(wParam, lParam);
        if (kargs.key == keys::k_F4) {
            close();
        } else {
            on_key_down(kargs);
        }
        return 0;

    case WM_SYSKEYUP:
        kargs = create_key_event_args(wParam, lParam);
        on_key_up(kargs);
        return 0;

    case WM_KEYDOWN:
        kargs = create_key_event_args(wParam, lParam);
        on_key_down(kargs);
        return 0;

    case WM_KEYUP:
        kargs = create_key_event_args(wParam, lParam);
        on_key_up(kargs);
        return 0;

    case WM_CHAR:
        kargs = create_char_event_args(wParam, lParam);
        on_key_press(kargs);
        return 0;
    }

    return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
