#include <tavros/system/window.hpp>

#include <tavros/core/logger/logger.hpp>

#define CHECK_WINDOW_HANDLE_R(ret)                                          \
    if (!m_wnd) {                                                           \
        ::logger.error("Window '{}' not created ({}).", m_title, __func__); \
        return ret;                                                         \
    }

#define CHECK_WINDOW_HANDLE()                                               \
    if (!m_wnd) {                                                           \
        ::logger.error("Window '{}' not created ({}).", m_title, __func__); \
        return;                                                             \
    }

namespace
{
    tavros::core::logger logger("window");
}

namespace tavros::system
{

    window::window(core::string_view title)
    {
        m_title = core::string(title);
        create();
    }

    window::~window()
    {
        if (m_wnd) {
            destroy();
        }
    }

    void window::create()
    {
        if (m_wnd) {
            ::logger.error("Failed to create window: window already created");
            return;
        }

        m_wnd = platform_window::create(m_title);
        if (!m_wnd) {
            ::logger.error("Failed to create platform window '{}'", m_title);
            return;
        }

        m_wnd->set_on_close_listener([this](auto*, auto& e) { this->on_close(e); });
        m_wnd->set_on_activate_listener([this](auto*) { this->on_activate(); });
        m_wnd->set_on_deactivate_listener([this](auto*) { this->on_deactivate(); });
        m_wnd->set_on_drop_listener([this](auto*, auto& e) { this->on_drop(e); });
        m_wnd->set_on_move_listener([this](auto*, auto& e) { this->on_move(e); });
        m_wnd->set_on_resize_listener([this](auto*, auto& e) { this->on_resize(e); });
        m_wnd->set_on_mouse_down_listener([this](auto*, auto& e) { this->on_mouse_down(e); });
        m_wnd->set_on_mouse_move_listener([this](auto*, auto& e) { this->on_mouse_move(e); });
        m_wnd->set_on_mouse_up_listener([this](auto*, auto& e) { this->on_mouse_up(e); });
        m_wnd->set_on_mouse_wheel_listener([this](auto*, auto& e) { this->on_mouse_wheel(e); });
        m_wnd->set_on_key_down_listener([this](auto*, auto& e) { this->on_key_down(e); });
        m_wnd->set_on_key_up_listener([this](auto*, auto& e) { this->on_key_up(e); });
        m_wnd->set_on_key_press_listener([this](auto*, auto& e) { this->on_key_press(e); });
    }

    void window::destroy()
    {
        if (!m_wnd) {
            ::logger.error("Failed to destroy window: platform handle is null");
            return;
        }

        m_wnd->close();
        m_wnd = nullptr;
    }

    void window::set_title(core::string_view title)
    {
        CHECK_WINDOW_HANDLE();
        m_title = core::string(title);
        m_wnd->set_text(title);
    }

    core::string_view window::title() const
    {
        return m_title;
    }

    void window::set_state(window_state ws)
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->set_state(ws);
    }

    window_state window::state() const
    {
        CHECK_WINDOW_HANDLE_R(window_state::minimized);
        return m_wnd->state();
    }

    void window::set_position(int32 left, int32 top)
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->set_location(left, top);
    }

    math::ipoint2 window::position() const
    {
        CHECK_WINDOW_HANDLE_R({});
        return m_wnd->location();
    }

    void window::set_size(int32 width, int32 height)
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->set_size(width, height);
    }

    math::isize2 window::size() const
    {
        CHECK_WINDOW_HANDLE_R({});
        return m_wnd->size();
    }

    void window::set_client_size(int32 width, int32 height)
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->set_client_size(width, height);
    }

    math::isize2 window::client_size() const
    {
        CHECK_WINDOW_HANDLE_R({});
        return m_wnd->client_size();
    }

    void window::set_enabled(bool enable)
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->set_enabled(enable);
    }

    bool window::is_enabled() const
    {
        CHECK_WINDOW_HANDLE_R(false);
        return m_wnd->is_enabled();
    }

    void window::activate()
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->activate();
    }

    void window::show()
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->show();
    }

    void window::hide()
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->hide();
    }

    void window::close()
    {
        CHECK_WINDOW_HANDLE();
        m_wnd->close();
    }

    void* window::native_handle() const noexcept
    {
        CHECK_WINDOW_HANDLE_R(nullptr);
        return m_wnd->native_handle();
    }

    void window::on_close(close_event_args& e)
    {
        destroy();
    }

    void window::on_activate()
    {
    }

    void window::on_deactivate()
    {
    }

    void window::on_drop(drop_event_args& e)
    {
    }

    void window::on_move(move_event_args& e)
    {
    }

    void window::on_resize(size_event_args& e)
    {
    }

    void window::on_mouse_down(mouse_event_args& e)
    {
    }

    void window::on_mouse_move(mouse_event_args& e)
    {
    }

    void window::on_mouse_up(mouse_event_args& e)
    {
    }


    void window::on_mouse_wheel(mouse_event_args& e)
    {
    }

    void window::on_key_down(key_event_args& e)
    {
    }

    void window::on_key_up(key_event_args& e)
    {
    }

    void window::on_key_press(key_event_args& e)
    {
    }

} // namespace tavros::system
