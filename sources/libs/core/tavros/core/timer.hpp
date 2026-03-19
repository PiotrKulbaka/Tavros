#pragma once

#include <tavros/core/types.hpp>
#include <chrono>

namespace tavros::core
{

    /**
     * @brief Measures elapsed time with pause/resume support.
     */
    class timer
    {
    public:
        using clock = std::chrono::steady_clock;
        using time_point = std::chrono::time_point<clock>;
        using duration = time_point::duration;

    public:
        /**
         * @brief Constructs and starts the timer immediately.
         */
        timer() noexcept
        {
            restart();
        }

        ~timer() noexcept = default;

        /**
         * @brief Starts or restarts the timer, resetting the accumulated time.
         */
        void restart() noexcept
        {
            m_start = clock::now();
            m_accumulated = duration(0);
            m_paused = false;
        }

        /**
         * @brief Pauses the timer. No-op if already paused.
         */
        void pause() noexcept
        {
            if (!m_paused) {
                m_accumulated += clock::now() - m_start;
                m_paused = true;
            }
        }

        /**
         * @brief Resumes the timer. No-op if not paused.
         */
        void unpause() noexcept
        {
            if (m_paused) {
                m_start = clock::now();
                m_paused = false;
            }
        }

        /**
         * @brief Returns true if the timer is currently paused.
         */
        [[nodiscard]] bool is_paused() const noexcept
        {
            return m_paused;
        }

        /**
         * @brief Returns elapsed time cast to duration type @p T, excluding paused intervals.
         *
         * @tparam T A @c std::chrono duration type (e.g. @c std::chrono::milliseconds).
         */
        template<typename T>
        T elapsed() const noexcept
        {
            return std::chrono::duration_cast<T>(elapsed_duration());
        }

        /**
         * @brief Returns elapsed time in seconds as @c double, excluding paused intervals.
         */
        [[nodiscard]] double elapsed_seconds() const noexcept
        {
            return elapsed<std::chrono::duration<double>>().count();
        }

        /**
         * @brief Returns the raw elapsed duration, excluding paused intervals.
         */
        duration elapsed_duration() const noexcept
        {
            return m_paused ? m_accumulated : m_accumulated + (clock::now() - m_start);
        }

    private:
        time_point m_start;
        duration   m_accumulated;
        bool       m_paused;
    };

} // namespace tavros::core
