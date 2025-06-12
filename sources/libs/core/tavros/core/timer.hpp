#pragma once

#include <tavros/core/types.hpp>
#include <chrono>

namespace tavros::core
{

    /**
     * @brief A simple timer class for measuring elapsed time with pause/unpause support.
     *
     * This timer is useful for measuring durations in various time units.
     * It can be paused and resumed without losing the already accumulated time.
     */
    class timer
    {
    public:
        using clock = std::chrono::steady_clock;
        using time = std::chrono::time_point<clock>;
        using duration = time::duration;

    public:
        /**
         * @brief Constructs and starts the timer immediately.
         */
        timer() noexcept;

        /**
         * @brief Starts or restarts the timer, resetting the accumulated time.
         */
        void start() noexcept;

        /**
         * @brief Pauses the timer. Has no effect if already paused.
         */
        void pause() noexcept;

        /**
         * @brief Resumes the timer from pause. Has no effect if not paused.
         */
        void unpause() noexcept;

        /**
         * @brief Checks whether the timer is currently paused.
         * @return true if paused, false otherwise.
         */
        bool is_paused() const noexcept;

        /**
         * @brief Returns the elapsed time since the last start, excluding any paused duration.
         *
         * The returned value is cast to the specified duration type.
         * For example: `elapsed<std::chrono::milliseconds>()` returns milliseconds as uint64.
         *
         * @tparam T Chrono duration type, such as std::chrono::microseconds, milliseconds, seconds.
         * @return Elapsed time in the requested unit.
         */
        template<typename T>
        uint64 elapsed() const noexcept
        {
            return std::chrono::duration_cast<T>(elapsed_duration()).count();
        }

        /**
         * @brief Returns the raw elapsed duration since the last start.
         * If the timer is paused, returns the accumulated time.
         * Otherwise, includes the time since the last unpause.
         *
         * @return Duration since start.
         */
        duration elapsed_duration() const noexcept;

    private:
        time     m_start;
        duration m_accumulated;
        bool     m_is_paused;
    };

} // namespace tavros::core
