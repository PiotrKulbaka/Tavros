#pragma once

#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/command_list.hpp>


namespace tavros::renderer
{

    /**
     * @brief The frame_composer class manages the full lifecycle of rendering a single frame.
     *
     * This interface is responsible for coordinating command list creation, submission,
     * and synchronization for a frame. It abstracts the process of composing a frame from multiple
     * command lists, handling the backbuffer resizing and presentation.
     *
     * Typically, a frame_composer is tied to a single swapchain or rendering target.
     * It supports recording commands in parallel command lists which are then submitted
     * for execution together.
     *
     * The workflow usually follows this pattern:
     *  - call begin_frame() to start a new frame
     *  - create one or more command lists via create_command_list()
     *  - submit each command list with submit_command_list()
     *  - call end_frame() to signal command recording completion
     *  - call present() to present the rendered backbuffer
     *  - query frame completion via is_frame_complete() or wait_for_frame_complete()
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
         * This method must NOT be called between begin_frame() and end_frame().
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
         * Must be called between begin_frame() and end_frame().
         * This call typically triggers buffer swap and advances the internal backbuffer index.
         */
        virtual void present() = 0;

        /**
         * @brief Begin recording a new frame.
         *
         * Prepares the frame composer to start accepting new command lists.
         * Must be called before any command lists are created or submitted for the frame.
         */
        virtual void begin_frame() = 0;

        /**
         * @brief End recording of the current frame.
         *
         * Signals that no more command lists will be submitted for this frame.
         * Must be called after all command lists have been submitted.
         */
        virtual void end_frame() = 0;

        /**
         * @brief Create a new command list for the current frame.
         *
         * Command lists created by this method are used to record rendering or compute commands.
         * The lifetime of the command list is tied to the current frame.
         *
         * @return command_list* Pointer to a new command list object, or nullptr if no resources are available.
         */
        virtual command_list* create_command_list() = 0;

        /**
         * @brief Submit a completed command list for execution.
         *
         * This method indicates that the command list has finished recording
         * and is ready to be executed by the GPU.
         *
         * @param list Pointer to the command list to submit.
         */
        virtual void submit_command_list(command_list* list) = 0;

        /**
         * @brief Check asynchronously if the last submitted frame has finished rendering.
         *
         * Can be called at any time. Returns true if the GPU has completed rendering the last frame.
         *
         * @return true if the last frame is complete.
         * @return false if the last frame is still in progress.
         */
        virtual bool is_frame_complete() = 0;

        /**
         * @brief Block the calling thread until the last frame finishes rendering.
         *
         * This is a blocking call and should be used sparingly to avoid stalls.
         */
        virtual void wait_for_frame_complete() = 0;
    };

} // namespace tavros::renderer
