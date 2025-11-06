#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/system/event_args.hpp>
#include <tavros/system/window_state.hpp>

namespace tavros::system
{

    class platform_window;

    /**
     * @brief Represents a high-level abstraction of a platform window.
     *
     * The window class provides a platform-independent interface for window
     * management, including creation, resizing, state changes, and event handling.
     */
    class window : core::noncopyable
    {
    public:
        /**
         * @brief Constructs a window with the specified title.
         * @param title Window title string.
         */
        window(core::string_view title);

        /**
         * @brief Destroys the window and releases platform resources.
         */
        virtual ~window();

        /**
         * @brief Creates the underlying platform-specific window.
         *
         * If the window is already created, an error is logged.
         */
        void create();

        /**
         * @brief Destroys the platform-specific window.
         *
         * If the window is not created, an error is logged.
         */
        void destroy();

        /**
         * @brief Sets the window title.
         * @param title New title text.
         */
        void set_title(core::string_view title);

        /**
         * @brief Returns the current window title.
         * @return The title string.
         */
        core::string_view title() const;

        /**
         * @brief Sets the current window state (e.g., normal, minimized, maximized).
         * @param ws Desired window state.
         */
        void set_state(window_state ws);

        /**
         * @brief Returns the current window state.
         * @return The active window state.
         */
        window_state state() const;

        /**
         * @brief Sets the position of the window on the screen.
         * @param left X coordinate of the top-left corner.
         * @param top Y coordinate of the top-left corner.
         */
        void set_position(int32 left, int32 top);

        /**
         * @brief Returns the current window position.
         * @return The (x, y) coordinates of the window.
         */
        math::ipoint2 position() const;

        /**
         * @brief Sets the outer size of the window.
         * @param width Window width in pixels.
         * @param height Window height in pixels.
         */
        void set_size(int32 width, int32 height);

        /**
         * @brief Returns the outer size of the window.
         * @return The window size as (width, height).
         */
        math::isize2 size() const;

        /**
         * @brief Sets the client area (drawable area) size.
         * @param width Client area width in pixels.
         * @param height Client area height in pixels.
         */
        void set_client_size(int32 width, int32 height);

        /**
         * @brief Returns the size of the client area.
         * @return The client area size as (width, height).
         */
        math::isize2 client_size() const;

        /**
         * @brief Enables or disables the window.
         * @param enable True to enable, false to disable.
         */
        void set_enabled(bool enable);

        /**
         * @brief Returns whether the window is currently enabled.
         * @return True if enabled, false otherwise.
         */
        bool is_enabled() const;

        /**
         * @brief Activates the window (brings it to the foreground).
         */
        void activate();

        /**
         * @brief Shows the window.
         */
        void show();

        /**
         * @brief Hides the window.
         */
        void hide();

        /**
         * @brief Closes the window.
         */
        void close();

        /**
         * @brief Returns the native platform-specific handle of the window.
         * @return Pointer to the native handle.
         */
        void* native_handle() const noexcept;

        /**
         * @brief Called when the window is about to close.
         * @param e Close event arguments.
         */
        virtual void on_close(close_event_args& e);

        /**
         * @brief Called when the window becomes active.
         */
        virtual void on_activate();

        /**
         * @brief Called when the window becomes inactive.
         */
        virtual void on_deactivate();

        /**
         * @brief Called when a drag-and-drop event occurs.
         * @param e Drop event arguments.
         */
        virtual void on_drop(drop_event_args& e);

        /**
         * @brief Called when the window is moved.
         * @param e Move event arguments.
         */
        virtual void on_move(move_event_args& e);

        /**
         * @brief Called when the window is resized.
         * @param e Resize event arguments.
         */
        virtual void on_resize(size_event_args& e);

        /**
         * @brief Called when a mouse button is pressed.
         * @param e Mouse event arguments.
         */
        virtual void on_mouse_down(mouse_event_args& e);

        /**
         * @brief Called when the mouse is moved.
         * @param e Mouse event arguments.
         */
        virtual void on_mouse_move(mouse_event_args& e);

        /**
         * @brief Called when a mouse button is released.
         * @param e Mouse event arguments.
         */
        virtual void on_mouse_up(mouse_event_args& e);

        /**
         * @brief Called when the mouse wheel is scrolled.
         * @param e Mouse event arguments.
         */
        virtual void on_mouse_wheel(mouse_event_args& e);

        /**
         * @brief Called when a key is pressed down.
         * @param e Key event arguments.
         */
        virtual void on_key_down(key_event_args& e);

        /**
         * @brief Called when a key is released.
         * @param e Key event arguments.
         */
        virtual void on_key_up(key_event_args& e);

        /**
         * @brief Called when a printable key is pressed (text input).
         * @param e Key event arguments.
         */
        virtual void on_key_press(key_event_args& e);

    private:
        // Platform-specific window implementation
        core::unique_ptr<platform_window> m_wnd;
        core::string                      m_title;
    };

} // namespace tavros::system
