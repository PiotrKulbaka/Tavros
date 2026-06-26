#pragma once

#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>

namespace tavros::renderer::rhi
{

    /**
     * @brief The frame_composer class manages the full lifecycle of rendering a single frame.
     *
     * This interface is responsible for coordinating command queue creation, submission,
     * and synchronization for a frame. It abstracts the process of composing a frame from multiple
     * command queues, handling the backbuffer resizing and presentation.
     *
     * Typically, a frame_composer is tied to a single swapchain or rendering target.
     * It supports recording commands in parallel command queues which are then submitted
     * for execution together.
     *
     * This design allows flexible multi-threaded command recording and efficient frame presentation,
     * while managing synchronization and resource lifecycle internally.
     */
    class frame_composer
    {
    public:
        virtual ~frame_composer() = default;

        /**
         * @brief Resize the backbuffer to the specified width and height.
         *
         * Typically called when the window or rendering surface size changes.
         *
         * @param width  New width of the backbuffer in pixels.
         * @param height New height of the backbuffer in pixels.
         */
        virtual void resize(uint32 width, uint32 height) = 0;

        /**
         * @brief Get the current width of the backbuffer.
         *
         * Can be called at any time.
         *
         * @return uint32 Current backbuffer width in pixels.
         */
        virtual uint32 width() const noexcept = 0;

        /**
         * @brief Get the current height of the backbuffer.
         *
         * Can be called at any time.
         *
         * @return uint32 Current backbuffer height in pixels.
         */
        virtual uint32 height() const noexcept = 0;

        /**
         * @brief Get the current backbuffer framebuffer handle.
         *
         * The backbuffer may change after present() is called.
         * Use this to get the render target for the current frame.
         *
         * @return framebuffer_handle Handle to the current backbuffer.
         */
        virtual framebuffer_handle backbuffer() const noexcept = 0;

        /**
         * @brief Present the current backbuffer on the screen.
         *
         * This call typically triggers buffer swap and advances the internal backbuffer index.
         */
        virtual void present() = 0;
    };

} // namespace tavros::renderer::rhi
