#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/input/event_args.hpp>
#include <mutex>

namespace tavros::input
{

    /**
     * @brief Double-buffered event queue for storing and processing input events.
     *
     * This class collects input events pushed from different sources and provides
     * a stable view of events for the current frame. It maintains two internal
     * buffers and swaps them each frame to safely accumulate and process events
     * without synchronization issues.
     */
    class event_queue : tavros::core::noncopyable
    {
    public:
        /// Maximum number of events stored in one buffer.
        static constexpr size_t k_max_events = 1024;

    public:
        /**
         * @brief Constructs an empty event queue.
         */
        event_queue() noexcept;

        /**
         * @brief Destroy event queue.
         */
        ~event_queue() noexcept;

        /**
         * @brief Pushes a new event into the back buffer.
         *
         * Thread-safe method that stores the event in the back buffer.
         * If the buffer is full, the event is silently discarded.
         *
         * @param e The event to add.
         */
        void push_event(const event_args& e);

        /**
         * @brief Swaps the front and back event buffers.
         *
         * This method is typically called once per frame.
         * After swapping, the previously accumulated events become
         * the active (front) queue, ready for processing.
         */
        void swap_queues();

        /**
         * @brief Returns a read-only view of the current front queue.
         *
         * Provides direct access to the array of events that should be
         * processed during the current frame.
         *
         * @return A view of the active event queue.
         */
        event_args_queue_view front_queue() const;

    private:
        core::static_vector<event_args, k_max_events> m_buffer_a{};
        core::static_vector<event_args, k_max_events> m_buffer_b{};

        struct queue_data
        {
            event_args* begin = nullptr;
            event_args* end = nullptr;
        };

        queue_data m_front;
        queue_data m_back;

        std::mutex m_mutex;
    };

} // namespace tavros::input
