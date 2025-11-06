#pragma once

#include <tavros/input/event_args.hpp>
#include <tavros/core/math/ivec2.hpp>
#include <array>

namespace tavros::input
{

    /**
     * @brief Manages and tracks user input state per frame.
     *
     * The input_manager collects and processes input events (keyboard, mouse, window events),
     * maintaining their state across frames. It provides a consistent per-frame view of
     * input conditions, allowing queries such as key pressed, held, or released states.
     */
    class input_manager
    {
    public:
        /**
         * @brief Constructs an input manager instance.
         */
        input_manager() noexcept;

        /**
         * @brief Destroys the input manager and releases any associated resources.
         */
        ~input_manager() noexcept;

        /**
         * @brief Begins input tracking for the current frame.
         *
         * Should be called once at the start of a frame before processing any input events.
         *
         * @param frame_time_us Timestamp of the current frame in microseconds.
         */
        void begin_frame(uint64 frame_time_us) noexcept;

        /**
         * @brief Finalizes input tracking for the current frame.
         *
         * Should be called once per frame after processing all input events.
         * It clears transient states (pressed/released flags) and updates
         * internal deltas for smooth mouse movement and hold durations.
         */
        void end_frame() noexcept;

        /**
         * @brief Processes a batch of input events.
         *
         * Takes a view of input events and updates internal input states accordingly.
         *
         * @param event_queue A view of input events collected during this frame.
         */
        void process_events(event_args_queue_view event_queue) noexcept;

        /**
         * @brief Returns true if the specified key is in the current frame held down.
         */
        bool is_key_down(keyboard_key key) const noexcept;

        /**
         * @brief Returns true if the specified key is in the current frame released.
         */
        bool is_key_up(keyboard_key key) const noexcept;

        /**
         * @brief Returns true if the specified key is in the pressed state during this frame.
         */
        bool is_key_pressed(keyboard_key key) const noexcept;

        /**
         * @brief Returns a normalized factor [0–1] representing how long the key has been held.
         */
        float key_hold_factor(keyboard_key key) const noexcept;

        /**
         * @brief Returns true if the specified button is in the current frame held down.
         */
        bool is_mouse_button_down(mouse_button button) const noexcept;

        /**
         * @brief Returns true if the specified button is in the current frame released.
         */
        bool is_mouse_button_up(mouse_button button) const noexcept;

        /**
         * @brief Returns true if the specified button is in the pressed state during this frame.
         */
        bool is_mouse_button_pressed(mouse_button button) const noexcept;

        /**
         * @brief Returns a normalized factor [0–1] representing how long the button has been held.
         */
        float mouse_button_hold_factor(mouse_button button) const noexcept;

        /**
         * @brief Returns true if the mouse was moved this frame.
         */
        bool is_mouse_moved() const noexcept;

        /**
         * @brief Returns raw mouse movement delta for this frame.
         */
        math::vec2 get_raw_mouse_delta() const noexcept;

        /**
         * @brief Returns smoothed mouse movement delta for this frame.
         */
        math::vec2 get_smooth_mouse_delta() const noexcept;

        /**
         * @brief Returns current mouse cursor position.
         */
        math::vec2 get_mouse_pos() const noexcept;

        /**
         * @brief Returns true if the scroll wheel was used this frame.
         */
        bool is_wheel_spinned() const noexcept;

        /**
         * @brief Returns mouse wheel delta (horizontal/vertical spin).
         */
        math::vec2 get_wheel_spin_delta() const noexcept;

        /**
         * @brief Returns true if the window was resized this frame.
         */
        bool is_window_resized() const noexcept;

        /**
         * @brief Returns the current window size.
         */
        math::isize2 get_window_size() const noexcept;

        /**
         * @brief Returns true if the window was moved this frame.
         */
        bool is_window_moved() const noexcept;

        /**
         * @brief Returns the current window position.
         */
        math::vec2 get_window_pos() const noexcept;

    private:
        struct key_state
        {
            uint64 press_time_us = 0;   // Time when key was last pressed
            uint64 release_time_us = 0; // Time when key was last released
            uint64 accumulated_us = 0;  // Time accumulated during the current frame
            bool   is_key_down = false;
            bool   is_pressed = false;  // Whether the key is currently pressed
            bool   is_released = false; // Whether the key was released during the last frame
        };

        static constexpr size_t k_keyboard_size = static_cast<size_t>(keyboard_key::last_key);
        using keyboard_state = std::array<key_state, k_keyboard_size>;
        static constexpr size_t k_mouse_buttons_size = static_cast<size_t>(mouse_button::last_button);
        using mouse_buttons_state = std::array<key_state, k_mouse_buttons_size>;

        void on_key_down(keyboard_key key, uint64 time_us) noexcept;
        void on_key_up(keyboard_key key, uint64 time_us) noexcept;

        void on_mouse_down(mouse_button btn, uint64 time_us) noexcept;
        void on_mouse_up(mouse_button btn, uint64 time_us) noexcept;

        void on_mouse_move(math::vec2 pos, bool relative = false) noexcept;

        void  clear_state() noexcept;
        float get_hold_factor(const key_state& state) const noexcept;

    private:
        uint64 m_current_frame_time_us = 0; // Time of the current frame
        uint64 m_last_frame_time_us = 0;    // Time of the previous frame

        bool m_is_active = true;

        keyboard_state      m_keys; // State data per key
        mouse_buttons_state m_buttons;

        bool m_is_frame_started = false;

        math::vec2 m_raw_mouse_delta;
        math::vec2 m_smooth_mouse_delta;
        math::vec2 m_mouse_pos;
        math::vec2 m_wheel_delta;
        bool       m_is_mouse_moved = false;
        bool       m_is_wheel_spinned = false;

        bool         m_is_wnd_resized = false;
        bool         m_is_wnd_moved = false;
        math::isize2 m_wnd_size;
        math::vec2   m_wnd_pos;
    };

} // namespace tavros::input
