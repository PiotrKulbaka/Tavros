#include <tavros/renderer/internal/opengl/frame_composer_opengl.hpp>

#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>
#include <tavros/renderer/internal/opengl/type_conversions.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace
{
    tavros::core::logger logger("frame_composer_opengl");
} // namespace


namespace tavros::renderer::rhi
{

    core::unique_ptr<frame_composer> frame_composer_opengl::create(graphics_device_opengl* device, const frame_composer_create_info& info, void* native_handle)
    {
        TAV_ASSERT(device);

        // Validate the frame composer size
        if (info.width == 0 || info.height == 0) {
            ::logger.error("Frame composer width and height must be greater than zero.");
            return nullptr;
        }

        // Try to create the context first
        auto context = context_opengl::create(info, native_handle);
        if (!context) {
            ::logger.error("OpenGL context creation failed");
            return nullptr;
        }

        // Everything is ok, so create the frame composer
        return core::make_unique<frame_composer_opengl>(device, std::move(context), info);
    }

    frame_composer_opengl::frame_composer_opengl(graphics_device_opengl* device, core::unique_ptr<context_opengl> context, const frame_composer_create_info& info)
        : m_device(device)
        , m_context(std::move(context))
        , m_info(info)
    {
        TAV_ASSERT(m_context);
        TAV_ASSERT(m_device);
        TAV_ASSERT(info.width > 0 && info.height > 0);

        // Create default backbuffer
        framebuffer_create_info backbuffer_info;
        backbuffer_info.width = info.width;
        backbuffer_info.height = info.height;
        backbuffer_info.has_depth_stencil_attachment = info.depth_stencil_attachment_format != pixel_format::none;
        backbuffer_info.sample_count = 1; // For backbuffer, sample count must be 1

        m_internal_command_queue = core::make_unique<command_queue_opengl>(device);

        m_backbuffer = m_device->get_resources()->create(gl_framebuffer{backbuffer_info, 0, true, info.color_attachment_format, info.depth_stencil_attachment_format});
        ::logger.debug("Frame composer framebuffer {} created", m_backbuffer);

        m_fence = m_device->create_fence();
    }

    frame_composer_opengl::~frame_composer_opengl()
    {
        m_device->destroy_fence(m_fence);

        // Don't destroy if has no framebuffers (because now destructor of graphics_device is called)
        auto& pool = m_device->get_resources()->get_pool<gl_framebuffer>();
        if (pool.size() != 0) {
            if (auto* fb = m_device->get_resources()->try_get(m_backbuffer)) {
                m_device->get_resources()->remove(m_backbuffer);
                ::logger.debug("Frame composer framebuffer {} destroyed", m_backbuffer);
            } else {
                ::logger.error("Cannot destroy frame composer framebuffer {} because it does not exist", m_backbuffer);
            }
        }

        m_internal_command_queue = nullptr;

        m_context.reset();
    }

    void frame_composer_opengl::resize(uint32 width, uint32 height)
    {
        auto w = width > 0 ? width : 1;
        auto h = height > 0 ? height : 1;

        // Update frame composer size
        m_info.width = w;
        m_info.height = h;

        // Update framebuffer size
        if (auto* fb = m_device->get_resources()->try_get(m_backbuffer)) {
            fb->info.width = w;
            fb->info.height = h;
        } else {
            ::logger.error("Cannot find frame composer framebuffer {}", m_backbuffer);
        }
    }

    uint32 frame_composer_opengl::width() const noexcept
    {
        return m_info.width;
    }

    uint32 frame_composer_opengl::height() const noexcept
    {
        return m_info.height;
    }

    framebuffer_handle frame_composer_opengl::backbuffer() const noexcept
    {
        return m_backbuffer;
    }

    void frame_composer_opengl::present()
    {
        if (m_frame_started) {
            ::logger.warning("Frame is not completed yet, but trying to present it");
        }
        m_context->swap_buffers();
    }

    void frame_composer_opengl::begin_frame()
    {
        if (m_frame_started) {
            ::logger.error("Frame already started");
        }
        m_frame_started = true;
    }

    void frame_composer_opengl::end_frame()
    {
        if (!m_frame_started) {
            ::logger.error("Frame not started");
        }
        m_frame_started = false;
    }

    command_queue* frame_composer_opengl::create_command_queue()
    {
        return m_internal_command_queue.get();
    }

    void frame_composer_opengl::submit_command_queue(command_queue* queue)
    {
        TAV_UNUSED(queue);
        queue->signal_fence(m_fence);
    }

    bool frame_composer_opengl::is_frame_complete()
    {
        return !m_frame_started && m_device->is_fence_signaled(m_fence);
    }

    void frame_composer_opengl::wait_for_frame_complete()
    {
        m_device->wait_for_fence(m_fence);
    }

} // namespace tavros::renderer::rhi
