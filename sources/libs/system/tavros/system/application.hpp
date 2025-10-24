#pragma once

#include <tavros/core/singleton.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/math/ivec2.hpp>

namespace tavros::system
{

    class platform_application;

    /**
     * @brief Represents the main application singleton.
     *
     * The application class provides a high-level interface for controlling
     * the lifecycle of the program. It manages initialization, event handling,
     * and shutdown through an internal platform-specific implementation.
     *
     * This class follows the singleton pattern — only one instance can exist
     * during the lifetime of the program.
     */
    class application : public core::singleton<application>
    {
        friend class core::singleton<application>;

    protected:
        /**
         * @brief Constructs the application singleton.
         *
         * The constructor is protected to prevent direct instantiation.
         * Use application::instance() to access the global application object.
         */
        application();

        ~application();

    public:
        /**
         * @brief Requests application termination with a given exit code.
         *
         * This method signals the application to exit gracefully.
         * @param exit_code Exit code to return upon shutdown.
         */
        void request_exit(int exit_code);

        /**
         * @brief Runs the main application loop.
         *
         * This method starts the platform-specific event loop
         * and blocks until the application terminates.
         * @return Exit code returned upon completion.
         */
        int run();

        /**
         * @brief Returns the current desktop resolution.
         *
         * Queries the platform-specific implementation for the current
         * desktop screen size.
         * @return A 2D integer vector representing the desktop width and height.
         */
        math::isize2 desktop_size();

        /**
         * @brief Returns a high-precision timestamp in microseconds since program start.
         *
         * This function provides a monotonic, high-resolution timer value suitable
         * for profiling, frame timing, and measuring small time intervals.
         * The value is measured from the start of the program and is independent
         * of system clock changes.
         *
         * @return Current timestamp in microseconds as a 64-bit unsigned integer.
         */
        uint64 highp_time_us();

    private:
        // Platform-specific application implementation
        core::unique_ptr<platform_application> m_app;
    };

} // namespace tavros::system
