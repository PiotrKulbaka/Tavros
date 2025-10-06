#include "event_queue.hpp"

namespace app
{

    event_queue::event_queue() noexcept
    {
        m_front = {m_buffer_a.data(), m_buffer_a.data()};
        m_back = {m_buffer_b.data(), m_buffer_b.data()};
    }

    void event_queue::push_event(const event_info& e)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const size_t                count = static_cast<size_t>(m_back.end - m_back.begin);
        if (count < k_max_events) {
            *m_back.end++ = e;
        } else {
            tavros::core::logger::print(
                tavros::core::severity_level::warning,
                "event_queue",
                "Event queue overflow — call swap_queues() to flush events"
            );
        }
    }

    void event_queue::swap_queues()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::swap(m_front, m_back);
        m_back.end = m_back.begin;
    }

    event_queue::queue_view event_queue::front_queue() const
    {
        return m_front;
    }

} // namespace app
