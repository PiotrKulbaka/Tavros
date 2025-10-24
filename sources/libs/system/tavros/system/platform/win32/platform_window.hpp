#pragma once

#include <tavros/system/window/platform_window.hpp>

#include <Windows.h>

namespace tavros::system::win32
{

    class platform_window final : public tavros::system::platform_window
    {
    public:
        platform_window(core::string_view name);
        ~platform_window() override;

        void set_text(core::string_view text) override;

        void          set_state(window_state ws) override;
        window_state  get_state() const override;
        void          set_position(int32 left, int32 top) override;
        math::ipoint2 get_position() const override;
        void          set_size(int32 width, int32 height, bool client_rect) override;
        math::isize2  get_size(bool client_rect) const override;

        void set_enabled(bool enable) override;
        bool is_enabled() const override;

        void activate() override;
        void show() override;
        void hide() override;
        void close() override;

        void* native_handle() const noexcept override;

        void set_events_listener(system::window* listener) override;

    public:
        LRESULT process_window_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    protected:
        HWND    m_hWnd;
        window* m_cb_listener;
    };

} // namespace tavros::system::win32
