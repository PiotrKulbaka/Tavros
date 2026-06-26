#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resource_manager.hpp>

namespace tavros::renderer
{

    class render_system final : core::noncopyable
    {
    public:
        render_system(core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws) noexcept;
        ~render_system() noexcept;

        void init(void* main_window_native_handle);
        void shutdown() noexcept;

        void begin_frame() noexcept;

        void end_frame() noexcept;

        void resize_backbuffer(uint32 width, uint32 height);

        rhi::framebuffer_handle gpu_backbuffer() noexcept;

        // TODO: This is a temporary accessor. Remove it.
        // The render system should eventually provide higher-level APIs for resource
        // creation and management, and the graphics device should be encapsulated.
        rhi::graphics_device* get_graphics_device() noexcept
        {
            return m_gdevice.get();
        }

        resource_manager* resource_manager() noexcept
        {
            return m_rm.get();
        }

    private:
        uint64                                  m_frame_number = 0;
        bool                                    m_is_init = false;
        core::shared_ptr<assets::asset_manager> m_am;
        core::shared_ptr<tef::workspace>        m_ws;
        core::unique_ptr<rhi::graphics_device>  m_gdevice;

        core::unique_ptr<tavros::renderer::resource_manager> m_rm;

        rhi::frame_composer* m_composer = nullptr;
    };

} // namespace tavros::renderer
