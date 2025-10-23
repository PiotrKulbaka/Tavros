#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/system/keys.hpp>
#include <mutex>
#include <array>

namespace app
{

    enum class event_type : uint8
    {
        none,
        key_down,
        key_up,
        mouse_move,
        mouse_button_down,
        mouse_button_up,
        window_resize,
        deactivate,
        activate,
    };

    struct event_info
    {
        event_type                   type = event_type::none;                                // type of the event
        tavros::math::vec2           vec_info;                                               // for mouse_move (relative mouse_move)
        tavros::system::keys         key_info = tavros::system::keys::none;                  // for key_down or key_up
        tavros::system::mouse_button mouse_button_info = tavros::system::mouse_button::none; // for mouse_button_down or mouse_button_up
        uint64                       event_time_us = 0;
    };

    using event_queue_view = tavros::core::buffer_view<event_info>;

    class event_queue : tavros::core::noncopyable
    {
    public:
        static constexpr size_t k_max_events = 1024;

    public:
        event_queue() noexcept;

        void push_event(const event_info& e);

        void swap_queues();

        event_queue_view front_queue() const;

    private:
        std::array<event_info, k_max_events> m_buffer_a{};
        std::array<event_info, k_max_events> m_buffer_b{};

        struct queue_data
        {
            event_info* begin = nullptr;
            event_info* end = nullptr;
        };

        queue_data m_front;
        queue_data m_back;

        std::mutex m_mutex;
    };

} // namespace app
