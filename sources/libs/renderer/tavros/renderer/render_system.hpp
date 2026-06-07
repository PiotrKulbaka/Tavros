#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::renderer
{

    class render_system final : core::noncopyable
    {
    public:
        render_system() noexcept;
        ~render_system() noexcept;

        void init(void* main_window_native_handle);
        void shutdown() noexcept;


        // TODO: This is a temporary accessor. Remove it.
        // The render system should eventually provide higher-level APIs for resource
        // creation and management, and the graphics device should be encapsulated.
        rhi::graphics_device* get_graphics_device() noexcept
        {
            return m_graphics_device.get();
        }

        // TODO: This is a temporary accessor. Remove it.
        rhi::frame_composer* get_frame_composer() noexcept
        {
            return m_composer;
        }

    private:
        void release() noexcept;

    private:
        bool                                           m_initialized = false;
        tavros::core::unique_ptr<rhi::graphics_device> m_graphics_device;
        rhi::frame_composer*                           m_composer = nullptr;
    };

} // namespace tavros::renderer
