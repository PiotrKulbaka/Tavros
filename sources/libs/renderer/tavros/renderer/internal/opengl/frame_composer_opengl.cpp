#include <tavros/renderer/internal/opengl/frame_composer_opengl.hpp>

#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

#include <tavros/core/prelude.hpp>

namespace
{
    tavros::core::logger logger("frame_composer_opengl");
} // namespace


namespace tavros::renderer
{

    core::unique_ptr<frame_composer> frame_composer_opengl::create(graphics_device_opengl* device, const frame_composer_desc& desc, void* native_handle)
    {
        TAV_ASSERT(device);

        // Try to create the context first
        auto context = context_opengl::create(desc, native_handle);
        if (!context) {
            ::logger.error("Context creation failed");
            return nullptr;
        }

        // Validate the frame composer size
        if (desc.width == 0 || desc.height == 0) {
            ::logger.error("Frame composer width and height must be greater than zero.");
            return nullptr;
        }

        // Everything is ok, so create the frame composer
        return core::make_unique<frame_composer_opengl>(device, std::move(context), desc);
    }

    frame_composer_opengl::frame_composer_opengl(graphics_device_opengl* device, core::unique_ptr<context_opengl> context, const frame_composer_desc& desc)
        : m_device(device)
        , m_context(std::move(context))
        , m_desc(desc)
    {
        TAV_ASSERT(m_context);
        TAV_ASSERT(m_device);
        TAV_ASSERT(desc.width > 0 && desc.height > 0);

        // Create default backbuffer
        framebuffer_desc backbuffer_desc;
        backbuffer_desc.color_attachment_formats.push_back(desc.color_attachment_format);
        backbuffer_desc.depth_stencil_attachment_format = desc.depth_stencil_attachment_format;
        backbuffer_desc.width = desc.width;
        backbuffer_desc.height = desc.height;
        backbuffer_desc.sample_count = 1; // For backbuffer, sample count must be 1

        m_backbuffer = {m_device->get_resources()->framebuffers.insert({backbuffer_desc, 0, true})};
        ::logger.debug("Default framebuffer with id %u for frame composer created", m_backbuffer.id);
    }

    frame_composer_opengl::~frame_composer_opengl()
    {
        // Don't destroy if has no framebuffers (because now destructor of graphics_device is called)
        if (m_device->get_resources()->framebuffers.size() != 0) {
            if (auto* desc = m_device->get_resources()->framebuffers.try_get(m_backbuffer.id)) {
                m_device->get_resources()->framebuffers.remove(m_backbuffer.id);
            } else {
                ::logger.error("Can't destroy default framebuffer for frame composer with id %u because it doesn't exist", m_backbuffer.id);
            }
        }

        m_context.reset();
    }

    void frame_composer_opengl::resize(uint32 width, uint32 height)
    {
        TAV_ASSERT(width > 0 && height > 0);

        // Update frame composer size
        m_desc.width = width;
        m_desc.height = height;

        // Update framebuffer size
        if (auto* desc = m_device->get_resources()->framebuffers.try_get(m_backbuffer.id)) {
            desc->desc.width = width;
            desc->desc.height = height;
        } else {
            ::logger.error("Can't find default framebuffer for frame composer with id %u", m_backbuffer.id);
        }
    }

    uint32 frame_composer_opengl::width() const noexcept
    {
        return m_desc.width;
    }

    uint32 frame_composer_opengl::height() const noexcept
    {
        return m_desc.height;
    }

    framebuffer_handle frame_composer_opengl::backbuffer() const noexcept
    {
        return m_backbuffer;
    }

    void frame_composer_opengl::present()
    {
        m_context->swap_buffers();
    }

    void frame_composer_opengl::begin_frame()
    {
    }

    void frame_composer_opengl::end_frame()
    {
    }

    command_list* frame_composer_opengl::create_command_list()
    {
        return nullptr;
    }

    void frame_composer_opengl::submit_command_list(command_list* list)
    {
    }

    bool frame_composer_opengl::is_frame_complete()
    {
        return true;
    }

    void frame_composer_opengl::wait_for_frame_complete()
    {
    }

} // namespace tavros::renderer
