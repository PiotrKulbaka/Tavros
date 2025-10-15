#pragma once

#include "input_manager.hpp"

#include <algorithm>
#include <tavros/core/debug/assert.hpp>

namespace app
{

    input_manager::input_manager()
    {
        clear_state();
    }

    void input_manager::clear_state()
    {
        for (auto& state : m_keys) {
            state.press_time_us = 0;
            state.release_time_us = 0;
            state.accumulated_us = 0;
            state.is_pressed = false;
        }

        m_last_frame_time_us = 0;
        m_current_frame_time_us = 0;
        m_raw_mouse_delta.set(0.0f, 0.0f);
    }

    void input_manager::on_frame_started(uint64 frame_time_us)
    {
        m_last_frame_time_us = m_current_frame_time_us;
        m_current_frame_time_us = frame_time_us;

        for (auto& state : m_keys) {
            state.accumulated_us = 0;
        }

        m_raw_mouse_delta.set(0.0f, 0.0f);
    }

    void input_manager::on_key_press(tavros::system::keys key, uint64 time_us)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        auto& s = m_keys[idx];
        if (!s.is_pressed) {
            s.is_pressed = true;
            s.press_time_us = time_us;
        }
    }

    void input_manager::on_key_release(tavros::system::keys key, uint64 time_us)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        auto& s = m_keys[idx];
        if (s.is_pressed) {
            s.is_pressed = false;
            s.release_time_us = time_us;

            // Add a gap us if the key was pressed within the frame
            if (s.press_time_us > m_last_frame_time_us) {
                uint64 duration = std::min(time_us, m_current_frame_time_us) - s.press_time_us;
                s.accumulated_us += duration;
            }
        }
    }

    void input_manager::on_mouse_move(tavros::math::vec2 delta, uint64 time_us)
    {
        TAV_UNUSED(time_us);

        m_raw_mouse_delta += delta;
    }

    double input_manager::key_pressed_factor(tavros::system::keys key)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        const auto& s = m_keys[idx];

        const uint64 frame_duration = m_current_frame_time_us - m_last_frame_time_us;
        if (frame_duration == 0) {
            return 0.0f;
        }

        uint64 total_us = s.accumulated_us;

        // If the key is pressed and still held, add the hold time to the current moment
        if (s.is_pressed) {
            uint64 pressed_since = std::max(s.press_time_us, m_last_frame_time_us);
            total_us += m_current_frame_time_us - pressed_since;
        }

        double factor = static_cast<double>(total_us) / static_cast<double>(frame_duration);
        return factor > 1.0 ? 1.0 : factor;
    }

    bool input_manager::is_key_pressed(tavros::system::keys key)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        return m_keys[idx].is_pressed;
    }

    tavros::math::vec2 input_manager::get_raw_mouse_delta()
    {
        return m_raw_mouse_delta;
    }

    tavros::math::vec2 input_manager::get_smooth_mouse_delta()
    {
        auto  speed = tavros::math::length(m_raw_mouse_delta);
        float accel = std::clamp(std::pow(speed * 0.03f, 1.25f), 0.35f, 8.0f);
        return m_raw_mouse_delta * accel;
    }

} // namespace app
