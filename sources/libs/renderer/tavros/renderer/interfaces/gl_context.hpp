#pragma once

#include <tavros/core/prelude.hpp>

namespace tavros::renderer::interfaces
{
    class gl_context;
}

namespace tavros::renderer
{
    using gl_context_uptr = tavros::core::unique_ptr<interfaces::gl_context>;
} // namespace tavros::renderer

namespace tavros::renderer::interfaces
{
    /**
     * @brief Abstract class for managing OpenGL context.
     *
     * This class defines the interface for managing an OpenGL context,
     * including making the context current, swapping buffers, and checking
     * the current state of the context.
     */
    class gl_context
    {
    public:
        static gl_context_uptr create(handle);

    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        virtual ~gl_context() = default;

        /**
         * @brief Makes the OpenGL context current.
         *
         * This method binds the context to the current thread,
         * making it active for OpenGL calls.
         */
        virtual void make_current() = 0;

        /**
         * @brief Makes the OpenGL context inactive.
         *
         * This method unbinds the context, making it inactive for OpenGL calls.
         * It can be used to deactivate the current context.
         */
        virtual void make_inactive() = 0;

        /**
         * @brief Swaps the buffers.
         *
         * This method swaps the front and back buffers to display the rendered content
         * on the screen.
         */
        virtual void swap_buffers() = 0;

        /**
         * @brief Checks if the context is currently active.
         *
         * @return true if the context is active, false otherwise.
         */
        virtual bool is_current() = 0;
    };
} // namespace tavros::renderer::interfaces
