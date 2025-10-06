#pragma once

#include <tavros/core/math.hpp>
#include <tavros/system/interfaces/window.hpp>
#include <mutex>

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
    };

    class event_queue : tavros::core::noncopyable
    {
    public:
        static constexpr size_t k_max_events = 1024;

        struct queue_view
        {
            event_info* begin = nullptr;
            event_info* end = nullptr;
        };

    public:
        event_queue() noexcept;

        void push_event(const event_info& e);

        void swap_queues();

        queue_view front_queue() const;

    private:
        std::array<event_info, k_max_events> m_buffer_a{};
        std::array<event_info, k_max_events> m_buffer_b{};

        queue_view m_front;
        queue_view m_back;

        std::mutex m_mutex;
    };

} // namespace app
