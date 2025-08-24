#include <tavros/renderer/internal/opengl/command_list_opengl.hpp>

#include <tavros/core/prelude.hpp>

#include <glad/glad.h>

using namespace tavros::renderer;

namespace
{
    tavros::core::logger logger("command_list_opengl");

    GLenum to_gl_compare_func(compare_op op)
    {
        switch (op) {
        case compare_op::less:
            return GL_LESS;
        case compare_op::equal:
            return GL_EQUAL;
        case compare_op::less_equal:
            return GL_LEQUAL;
        case compare_op::greater:
            return GL_GREATER;
        case compare_op::greater_equal:
            return GL_GEQUAL;
        case compare_op::not_equal:
            return GL_NOTEQUAL;
        case compare_op::always:
            return GL_ALWAYS;
        case compare_op::off:
            return GL_NONE;
        default:
            TAV_UNREACHABLE();
        }
    }


    GLenum to_gl_stencil_op(stencil_op op)
    {
        switch (op) {
        case stencil_op::keep:
            return GL_KEEP;
        case stencil_op::zero:
            return GL_ZERO;
        case stencil_op::replace:
            return GL_REPLACE;
        case stencil_op::increment_clamp:
            return GL_INCR;
        case stencil_op::decrement_clamp:
            return GL_DECR;
        case stencil_op::invert:
            return GL_INVERT;
        case stencil_op::increment_wrap:
            return GL_INCR_WRAP;
        case stencil_op::decrement_wrap:
            return GL_DECR_WRAP;
        default:
            TAV_UNREACHABLE();
        }
    }

    struct gl_vertex_format
    {
        GLenum type;
        uint32 size;
    };

    gl_vertex_format to_gl_vertex_format(attribute_format format)
    {
        switch (format) {
        case attribute_format::u8:
            return {GL_UNSIGNED_BYTE, 1};
        case attribute_format::i8:
            return {GL_BYTE, 1};
        case attribute_format::u16:
            return {GL_UNSIGNED_SHORT, 2};
        case attribute_format::i16:
            return {GL_SHORT, 2};
        case attribute_format::u32:
            return {GL_UNSIGNED_INT, 4};
        case attribute_format::i32:
            return {GL_INT, 4};
        case attribute_format::f16:
            return {GL_HALF_FLOAT, 2};
        case attribute_format::f32:
            return {GL_FLOAT, 4};
        default:
            TAV_UNREACHABLE();
        }
    }

} // namespace

namespace tavros::renderer
{

    command_list_opengl::command_list_opengl(graphics_device_opengl* device)
        : m_device(device)
    {
        ::logger.info("command_list_opengl created");

        glGenFramebuffers(1, &m_resolve_fbo);
    }

    command_list_opengl::~command_list_opengl()
    {
        glDeleteFramebuffers(1, &m_resolve_fbo);

        ::logger.info("command_list_opengl destroyed");
    }

    void command_list_opengl::bind_pipeline(pipeline_handle pipeline)
    {
        m_current_pipeline_id = pipeline.id;
        if (auto* p = m_device->get_resources()->pipelines.try_get(pipeline.id)) {
            auto& desc = p->desc;

            glUseProgram(p->program_obj);

            // depth test
            if (desc.depth_stencil.depth_test_enable) {
                glEnable(GL_DEPTH_TEST);

                // depth write
                glDepthMask(desc.depth_stencil.depth_write_enable ? GL_TRUE : GL_FALSE);

                // depth compare func
                glDepthFunc(to_gl_compare_func(desc.depth_stencil.depth_compare));
            } else {
                glDisable(GL_DEPTH_TEST);
            }

            // stencil test
            if (desc.depth_stencil.stencil_test_enable) {
                glEnable(GL_STENCIL_TEST);

                // stencil front
                glStencilFuncSeparate(
                    GL_FRONT,
                    to_gl_compare_func(desc.depth_stencil.stencil_front.compare),
                    desc.depth_stencil.stencil_front.reference_value,
                    desc.depth_stencil.stencil_front.read_mask
                );
                glStencilOpSeparate(
                    GL_FRONT,
                    to_gl_stencil_op(desc.depth_stencil.stencil_front.stencil_fail_op),
                    to_gl_stencil_op(desc.depth_stencil.stencil_front.depth_fail_op),
                    to_gl_stencil_op(desc.depth_stencil.stencil_front.pass_op)
                );
                glStencilMaskSeparate(GL_FRONT, desc.depth_stencil.stencil_front.write_mask);

                // stencil back
                glStencilFuncSeparate(
                    GL_BACK,
                    to_gl_compare_func(desc.depth_stencil.stencil_back.compare),
                    desc.depth_stencil.stencil_back.reference_value,
                    desc.depth_stencil.stencil_back.read_mask
                );
                glStencilOpSeparate(
                    GL_BACK,
                    to_gl_stencil_op(desc.depth_stencil.stencil_back.stencil_fail_op),
                    to_gl_stencil_op(desc.depth_stencil.stencil_back.depth_fail_op),
                    to_gl_stencil_op(desc.depth_stencil.stencil_back.pass_op)
                );
                glStencilMaskSeparate(GL_BACK, desc.depth_stencil.stencil_back.write_mask);
            } else {
                glDisable(GL_STENCIL_TEST);
            }

            // rasterizer state
            // cull face
            if (desc.rasterizer.cull == cull_face::off) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(desc.rasterizer.cull == cull_face::front ? GL_FRONT : GL_BACK);
            }

            // front face
            glFrontFace(desc.rasterizer.face == front_face::clockwise ? GL_CW : GL_CCW);

            // polygon mode
            if (desc.rasterizer.polygon == polygon_mode::lines) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            } else if (desc.rasterizer.polygon == polygon_mode::points) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            } else if (desc.rasterizer.polygon == polygon_mode::fill) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            } else {
                TAV_UNREACHABLE();
            }

            // depth clamp
            if (desc.rasterizer.depth_clamp_enable) {
                glEnable(GL_DEPTH_CLAMP);
            } else {
                glDisable(GL_DEPTH_CLAMP);
            }

            // depth bias
            if (desc.rasterizer.depth_bias_enable) {
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(desc.rasterizer.depth_bias_slope, desc.rasterizer.depth_bias);
            } else {
                glDisable(GL_POLYGON_OFFSET_FILL);
            }

            // multisample state
            // uint8 sample_count = 1; cant be initialized here
            if (desc.multisample.sample_shading_enabled) {
                glEnable(GL_SAMPLE_SHADING);
                glMinSampleShading(desc.multisample.min_sample_shading);
            } else {
                glDisable(GL_SAMPLE_SHADING);
            }
        } else {
            ::logger.error("Can't bind the pipeline with id `%u`", pipeline.id);
            glUseProgram(0);
        }
    }

    void command_list_opengl::bind_geometry(geometry_binding_handle geometry_binding)
    {
        if (auto* gb = m_device->get_resources()->geometry_bindings.try_get(geometry_binding.id)) {
            glBindVertexArray(gb->vao_obj);
        } else {
            ::logger.error("Can't bind the geometry binding with id `%u`", geometry_binding.id);
            glBindVertexArray(0);
        }
    }

    void command_list_opengl::bind_texture(uint32 slot, texture_handle texture)
    {
        if (auto* tex = m_device->get_resources()->textures.try_get(texture.id)) {
            if (!tex->desc.usage.has_flag(texture_usage::sampled)) {
                ::logger.error("Can't bind not sampled texture with id `%u`", texture.id);
                return;
            }
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, tex->texture_obj);
        } else {
            ::logger.error("Can't bind the texture with id `%u`", texture.id);
        }
    }

    void command_list_opengl::begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer)
    {
        if (m_current_render_pass.id != 0 || m_current_framebuffer.id != 0) {
            ::logger.error("Can't begin the render pass because previous render_pass is not ended");
            return;
        }

        gl_render_pass* rp = m_device->get_resources()->render_passes.try_get(render_pass.id);
        if (rp == nullptr) {
            ::logger.error("Can't begin the render pass because render pass with id `%u` does not exist", render_pass.id);
            return;
        }

        gl_framebuffer* fb = m_device->get_resources()->framebuffers.try_get(framebuffer.id);
        if (fb == nullptr) {
            ::logger.error("Can't begin the render pass because framebuffer with id `%u` does not exist", framebuffer.id);
            return;
        }

        // Validate color attachments size
        if (rp->desc.color_attachments.size() != fb->desc.color_attachment_formats.size()) {
            ::logger.error("Mismatched number of color attachments for render pass with id `%u` and framebuffer with id `%u`", render_pass.id, framebuffer.id);
        }

        // Validate color attachments format
        for (uint32 i = 0; i < rp->desc.color_attachments.size(); ++i) {
            auto rp_color_format = rp->desc.color_attachments[i].format;
            auto fb_color_format = fb->desc.color_attachment_formats[i];
            if (rp_color_format != fb_color_format) {
                ::logger.error("Mismatched color attachment format for render pass with id `%u` and framebuffer with id `%u`", render_pass.id, framebuffer.id);
                return;
            }
        }

        // Validate depth/stencil attachment format
        if (rp->desc.depth_stencil_attachment.format != fb->desc.depth_stencil_attachment_format) {
            ::logger.error("Mismatched depth/stencil attachment format for render pass with id `%u` and framebuffer with id `%u`", render_pass.id, framebuffer.id);
            return;
        }

        // bind framebuffer
        if (fb->is_default) {
            TAV_ASSERT(fb->framebuffer_obj == 0);
            TAV_ASSERT(rp->desc.color_attachments.size() == 1);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer_obj);
        }

        // Set viewport
        glViewport(0, 0, fb->desc.width, fb->desc.height);

        // Allpy load operations to the color attachments and depth/stencil attachment
        // Only clear is supported, any other load operations are ignored
        if (fb->is_default) {
            GLbitfield clear_mask = 0;
            // Clear for color buffer
            if (rp->desc.color_attachments[0].load == load_op::clear) {
                clear_mask |= GL_COLOR_BUFFER_BIT;
            }

            if (rp->desc.depth_stencil_attachment.format != pixel_format::none) {
                // Clear for depth buffer
                if (rp->desc.depth_stencil_attachment.depth_load == load_op::clear) {
                    clear_mask |= GL_DEPTH_BUFFER_BIT;
                }
                // Clear for stencil buffer
                if (rp->desc.depth_stencil_attachment.stencil_load == load_op::clear) {
                    clear_mask |= GL_STENCIL_BUFFER_BIT;
                }
            }

            // Clear buffers if needed
            if (clear_mask) {
                glClear(clear_mask);
            }
        } else {
            // Apply load operations (only clear)
            uint32 attachment_index = 0;
            for (uint32 i = 0; i < fb->color_attachments.size(); ++i) {
                auto attachment_handle = fb->color_attachments[i];
                if (auto* tex = m_device->get_resources()->textures.try_get(attachment_handle.id)) {
                    // If texture has the same sample count with framebuffer, then this texture is attachment texture
                    if (tex->desc.sample_count == fb->desc.sample_count) {
                        auto& rp_color_attachment = rp->desc.color_attachments[i];

                        // Apply load operation (only clear)
                        // Any other load operation doesn't need to be applied
                        if (load_op::clear == rp_color_attachment.load) {
                            glClearBufferfv(GL_COLOR, GL_COLOR_ATTACHMENT0 + attachment_index, rp_color_attachment.clear_value);
                        }

                        attachment_index++;
                    }
                } else {
                    ::logger.error("Can't find the texture with id `%u`", attachment_handle.id);
                }
            }

            // Apply load operations to depth/stencil attachment
            if (rp->desc.depth_stencil_attachment.format != pixel_format::none) {
                auto& rp_depth_stencil_attachment = rp->desc.depth_stencil_attachment;

                // Apply load operation to depth component
                if (rp_depth_stencil_attachment.depth_load == load_op::clear) {
                    glClearBufferfv(GL_DEPTH, 0, &rp_depth_stencil_attachment.depth_clear_value);
                }

                // Apply load operation to stencil component
                if (rp_depth_stencil_attachment.stencil_load == load_op::clear) {
                    glClearBufferuiv(GL_STENCIL, 0, &rp_depth_stencil_attachment.stencil_clear_value);
                }
            }
        }

        m_current_framebuffer = framebuffer;
        m_current_render_pass = render_pass;
    }

    void command_list_opengl::end_render_pass()
    {
        if (m_current_render_pass.id == 0 || m_current_framebuffer.id == 0) {
            ::logger.error("Can't end the render pass because render_pass is not started");
            return;
        }

        gl_render_pass* rp = m_device->get_resources()->render_passes.try_get(m_current_render_pass.id);
        if (rp == nullptr) {
            ::logger.error("Can't end the render pass correctly because render pass with id `%u` does not exist", m_current_render_pass.id);
            return;
        }

        gl_framebuffer* fb = m_device->get_resources()->framebuffers.try_get(m_current_framebuffer.id);
        if (fb == nullptr) {
            ::logger.error("Can't end the render pass correctly because framebuffer with id `%u` does not exist", m_current_framebuffer.id);
            return;
        }

        // Collect resolve attachments
        core::static_vector<GLuint, k_max_color_attachments> resolve_attachments;
        core::static_vector<gl_texture*, k_max_color_attachments> resolve_textures;
        for (uint32 i = 0; i < rp->desc.color_attachments.size(); ++i) {
            auto& rp_color_attachment = rp->desc.color_attachments[i];
            auto* tex = m_device->get_resources()->textures.try_get(fb->color_attachments[i].id);
            TAV_ASSERT(tex);

            if (rp_color_attachment.store == store_op::resolve) {
                // Resolve attachments
                auto resolve_index = rp_color_attachment.resolve_attachment_index;
                if (fb->color_attachments.size() > resolve_index) {
                    resolve_attachments.push_back(GL_COLOR_ATTACHMENT0 + i); // TODO: fix it, because it's not correct
                    resolve_textures.push_back(tex); // TODO: fix it, because it's not correct
                } else {
                    ::logger.error("Invalid resolve attachment index `%u`", resolve_index);
                    return;
                }
            }
        }


        // Check if need to resolve
        if (resolve_attachments.size() > 0) {
            // Bind for resolve and attach textures
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolve_fbo);

            for (uint32 i = 0; i < resolve_attachments.size(); ++i) {
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, resolve_attachments[i], GL_TEXTURE_2D, resolve_textures[i]->texture_obj, 0);
            }

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->framebuffer_obj);

            // Resolve
            for (uint32 i = 0; i < resolve_attachments.size(); ++i) {
                glReadBuffer(resolve_attachments[i]);
                glDrawBuffer(resolve_attachments[i]);
                glBlitFramebuffer(
                    0, 0, fb->desc.width, fb->desc.height,
                    0, 0, fb->desc.width, fb->desc.height,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST
                );
            }

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_current_framebuffer = {0};
        m_current_render_pass = {0};
    }

} // namespace tavros::renderer
