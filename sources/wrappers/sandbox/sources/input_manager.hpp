#pragma once

#include <tavros/system/keys.hpp>
#include <tavros/core/math.hpp>
#include <array>

namespace app
{

    class input_manager
    {
    public:
        input_manager();

        ~input_manager() noexcept = default;

        /**
         * @brief Clears all key states.
         * Called for example when the window is deactivated,
         * to ensure that key states are valid when the window becomes active again.
         */
        void clear_state();

        /**
         * @brief Called at the beginning of each frame.
         * Updates time markers and resets accumulated frame-specific data.
         * @param frame_time_us start frame time
         */
        void on_frame_started(uint64 frame_time_us);

        /**
         * @brief Called when a key is pressed.
         * @param key Key identifier
         * @param time_us Time in microseconds when the event occurred
         */
        void on_key_press(tavros::system::keys key, uint64 time_us);

        /**
         * @brief Called when a key is released.
         * @param key Key identifier
         * @param time_us Time in microseconds when the event occurred
         */
        void on_key_release(tavros::system::keys key, uint64 time_us);

        void on_mouse_move(tavros::math::vec2 delta, uint64 time_us);

        /**
         * @brief Returns how long the key was pressed during the last frame, in normalized [0..1] form.
         * @param key Key identifier
         * @return 0.0 if not pressed at all, 1.0 if pressed the entire frame, otherwise partial
         */
        double key_pressed_factor(tavros::system::keys key);

        bool is_key_pressed(tavros::system::keys key);

        bool is_key_released(tavros::system::keys key);

        tavros::math::vec2 get_raw_mouse_delta();

        tavros::math::vec2 get_smooth_mouse_delta();

    private:
        static constexpr size_t k_keyboard_size = static_cast<size_t>(tavros::system::keys::k_last_key);

        struct key_state
        {
            uint64 press_time_us = 0;   // Time when key was last pressed
            uint64 release_time_us = 0; // Time when key was last released
            uint64 accumulated_us = 0;  // Time accumulated during the current frame
            bool   is_pressed = false;  // Whether the key is currently pressed
            bool   is_released = false; // Whether the key was released during the last frame
        };

        uint64                                 m_current_frame_time_us = 0; // Time of the current frame
        uint64                                 m_last_frame_time_us = 0;    // Time of the previous frame
        std::array<key_state, k_keyboard_size> m_keys;                      // State data per key
        tavros::math::vec2                     m_raw_mouse_delta;
        tavros::math::vec2                     m_smooth_mouse_delta;
        uint64                                 m_last_mouse_time_us = 0;
        uint64                                 m_mouse_time_us = 0;
    };

} // namespace app
