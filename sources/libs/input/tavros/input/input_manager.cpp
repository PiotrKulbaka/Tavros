#include <tavros/input/input_manager.hpp>

#include <algorithm>
#include <tavros/core/debug/assert.hpp>

#include <tavros/core/logger/logger.hpp>

#include <tavros/core/math.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace
{
    tavros::core::logger logger("input_manager");
}

namespace tavros::input
{

    input_manager::input_manager() noexcept
    {
        clear_state();
    }

    input_manager::~input_manager() noexcept
    {
    }

    void input_manager::begin_frame(uint64 frame_time_us) noexcept
    {
        if (m_is_frame_started) {
            ::logger.warning("frame already started");
        }
        m_is_frame_started = true;

        m_last_frame_time_us = m_current_frame_time_us;
        m_current_frame_time_us = frame_time_us;

        for (auto& state : m_keys) {
            state.accumulated_us = 0;
            state.is_key_down = false;
            state.is_released = false;
        }

        m_is_mouse_moved = false;
        m_is_wheel_spinned = false;
        m_raw_mouse_delta = {};
        m_smooth_mouse_delta = {};
        m_wheel_delta = {};

        m_is_wnd_resized = false;
        m_is_wnd_moved = false;
    }

    void input_manager::end_frame() noexcept
    {
        if (!m_is_frame_started) {
            ::logger.warning("frame already ended");
        }
        m_is_frame_started = false;
    }

    void input_manager::process_events(event_args_queue_view event_queue) noexcept
    {
        for (auto& e : event_queue) {
            if (!m_is_active) {
                if (event_type::activate == e.type) {
                    m_is_active = true;
                }
                continue;
            }

            switch (e.type) {
            case event_type::none:
                ::logger.warning("event `none` received");
                break;

            case event_type::mouse_down:
                on_mouse_down(e.button, e.time_us);
                break;

            case event_type::mouse_move:
                on_mouse_move(e.vec, false);
                break;

            case event_type::mouse_move_delta:
                on_mouse_move(e.vec, true);
                break;

            case event_type::mouse_up:
                on_mouse_up(e.button, e.time_us);
                break;

            case event_type::mouse_wheel:
                m_is_wheel_spinned = true;
                m_wheel_delta += e.vec;
                break;

            case event_type::key_down:
                on_key_down(e.key, e.time_us);
                break;

            case event_type::key_press:
                break;

            case event_type::key_up:
                on_key_up(e.key, e.time_us);
                break;

            case event_type::window_size:
                m_is_wnd_resized = true;
                m_wnd_size = math::isize2(static_cast<int32>(e.vec.x), static_cast<int32>(e.vec.y));
                break;

            case event_type::window_move:
                m_is_wnd_moved = true;
                m_wnd_pos = e.vec;
                break;

            case event_type::activate:
                m_is_active = true;
                break;

            case event_type::deactivate:
                m_is_active = false;
                break;

            default:
                TAV_UNREACHABLE();
                break;
            }
        }
    }

    bool input_manager::is_key_down(keyboard_key key) const noexcept
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);
        return m_keys[idx].is_pressed && m_keys[idx].accumulated_us == 0;
    }

    bool input_manager::is_key_up(keyboard_key key) const noexcept
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);
        return m_keys[idx].is_released;
    }

    bool input_manager::is_key_pressed(keyboard_key key) const noexcept
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);
        return m_keys[idx].is_pressed;
    }

    float input_manager::key_hold_factor(keyboard_key key) const noexcept
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);
        return get_hold_factor(m_keys[idx]);
    }

    bool input_manager::is_mouse_button_down(mouse_button button) const noexcept
    {
        const size_t idx = static_cast<size_t>(button);
        TAV_ASSERT(idx < k_mouse_buttons_size);
        return m_buttons[idx].is_pressed && m_buttons[idx].accumulated_us == 0;
    }

    bool input_manager::is_mouse_button_up(mouse_button button) const noexcept
    {
        const size_t idx = static_cast<size_t>(button);
        TAV_ASSERT(idx < k_mouse_buttons_size);
        return m_buttons[idx].is_released;
    }

    bool input_manager::is_mouse_button_pressed(mouse_button button) const noexcept
    {
        const size_t idx = static_cast<size_t>(button);
        TAV_ASSERT(idx < k_mouse_buttons_size);
        return m_buttons[idx].is_pressed;
    }

    float input_manager::mouse_button_hold_factor(mouse_button button) const noexcept
    {
        const size_t idx = static_cast<size_t>(button);
        TAV_ASSERT(idx < k_mouse_buttons_size);
        return get_hold_factor(m_buttons[idx]);
    }

    bool input_manager::is_mouse_moved() const noexcept
    {
        return m_is_mouse_moved;
    }

    math::vec2 input_manager::get_raw_mouse_delta() const noexcept
    {
        return m_raw_mouse_delta;
    }

    math::vec2 input_manager::get_smooth_mouse_delta() const noexcept
    {
        return m_smooth_mouse_delta;
    }

    math::vec2 input_manager::get_mouse_pos() const noexcept
    {
        return m_mouse_pos;
    }

    bool input_manager::is_wheel_spinned() const noexcept
    {
        return m_is_wheel_spinned;
    }

    math::vec2 input_manager::get_wheel_spin_delta() const noexcept
    {
        return m_wheel_delta;
    }

    bool input_manager::is_window_resized() const noexcept
    {
        return m_is_wnd_resized;
    }

    math::isize2 input_manager::get_window_size() const noexcept
    {
        return m_wnd_size;
    }

    bool input_manager::is_window_moved() const noexcept
    {
        return m_is_wnd_moved;
    }

    math::vec2 input_manager::get_window_pos() const noexcept
    {
        return m_wnd_pos;
    }

    void input_manager::on_key_down(keyboard_key key, uint64 time_us) noexcept
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        auto& s = m_keys[idx];
        if (!s.is_pressed) {
            s.is_pressed = true;
            s.press_time_us = time_us;
        }
    }

    void input_manager::on_key_up(keyboard_key key, uint64 time_us) noexcept
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

            s.is_released = true;
        }
    }

    void input_manager::on_mouse_down(mouse_button btn, uint64 time_us) noexcept
    {
        const size_t idx = static_cast<size_t>(btn);
        TAV_ASSERT(idx < k_mouse_buttons_size);

        auto& s = m_buttons[idx];
        if (!s.is_pressed) {
            s.is_pressed = true;
            s.press_time_us = time_us;
        }
    }

    void input_manager::on_mouse_up(mouse_button btn, uint64 time_us) noexcept
    {
        const size_t idx = static_cast<size_t>(btn);
        TAV_ASSERT(idx < k_mouse_buttons_size);

        auto& s = m_buttons[idx];
        if (s.is_pressed) {
            s.is_pressed = false;
            s.release_time_us = time_us;

            // Add a gap us if the key was pressed within the frame
            if (s.press_time_us > m_last_frame_time_us) {
                uint64 duration = std::min(time_us, m_current_frame_time_us) - s.press_time_us;
                s.accumulated_us += duration;
            }

            s.is_released = true;
        }
    }

    void input_manager::on_mouse_move(math::vec2 pos, bool relative) noexcept
    {
        if (relative) {
            m_raw_mouse_delta += pos;
            auto accel = std::sqrt(tavros::math::length(pos)) * 0.0025f;
            m_smooth_mouse_delta += pos * accel;
        } else {
            m_mouse_pos = pos;
        }
        m_is_mouse_moved = true;
    }

    void input_manager::clear_state() noexcept
    {
        for (auto& state : m_keys) {
            state = {};
        }

        for (auto& state : m_buttons) {
            state = {};
        }

        m_last_frame_time_us = 0;
        m_current_frame_time_us = 0;
        m_raw_mouse_delta = {};
        m_smooth_mouse_delta = {};
    }

    float input_manager::get_hold_factor(const key_state& state) const noexcept
    {
        const uint64 frame_duration = m_current_frame_time_us - m_last_frame_time_us;
        if (frame_duration == 0) {
            return 0.0f;
        }

        uint64 total_us = state.accumulated_us;

        // If the key is pressed and still held, add the hold time to the current moment
        if (state.is_pressed) {
            uint64 pressed_since = std::max(state.press_time_us, m_last_frame_time_us);
            total_us += m_current_frame_time_us - pressed_since;
        }

        auto factor = static_cast<float>(total_us) / static_cast<float>(frame_duration);
        return factor > 1.0 ? 1.0 : factor;
    }

} // namespace tavros::input
