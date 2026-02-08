#include <tavros/renderer/internal/opengl/command_queue_opengl.hpp>

#include <tavros/renderer/internal/opengl/type_conversions.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>
#include <tavros/renderer/internal/opengl/gl_check.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/math/functions/basic_math.hpp>

#include <glad/glad.h>

using namespace tavros::renderer::rhi;

namespace
{
    tavros::core::logger logger("command_queue_opengl");

    GLboolean to_gl_bool(bool value) noexcept
    {
        return value ? GL_TRUE : GL_FALSE;
    }
} // namespace

namespace tavros::renderer::rhi
{

    command_queue_opengl::command_queue_opengl(graphics_device_opengl* device)
        : m_device(device)
    {
        ::logger.debug("command_queue_opengl created");

        GL_CALL(glGenFramebuffers(1, &m_resolve_fbo));
    }

    command_queue_opengl::~command_queue_opengl()
    {
        GL_CALL(glDeleteFramebuffers(1, &m_resolve_fbo));

        ::logger.debug("command_queue_opengl destroyed");
    }

    void command_queue_opengl::begin()
    {
        if (m_started) {
            ::logger.error("Command queue already begun");
            return;
        }
        m_started = true;
    }

    void command_queue_opengl::end()
    {
        if (!m_started) {
            ::logger.error("Command queue not begun");
            return;
        }
        m_started = false;
    }

    void command_queue_opengl::bind_pipeline(pipeline_handle pipeline)
    {
        m_current_pipeline = {};

        auto* p = m_device->get_resources()->try_get(pipeline);
        if (!p) {
            ::logger.error("Failed to bind pipeline {}: not found", pipeline);
            GL_CALL(glUseProgram(0));
            GL_CALL(glBindVertexArray(0));
            return;
        }

        auto& info = p->info;

        auto* pass = m_device->get_resources()->try_get(m_current_render_pass);
        if (!pass) {
            ::logger.error("Failed to bind pipeline {}: render pass {} not found", pipeline, m_current_render_pass);
            GL_CALL(glUseProgram(0));
            GL_CALL(glBindVertexArray(0));
            return;
        }

        auto& pass_info = pass->info;
        if (pass_info.color_attachments.size() != info.blend_states.size()) {
            ::logger.error(
                "Failed to bind pipeline {}: mismatch between color attachments in current render pass {} and blend states in pipeline {}",
                pipeline,
                fmt::styled_param(pass_info.color_attachments.size()),
                fmt::styled_param(info.blend_states.size())
            );
            GL_CALL(glUseProgram(0));
            GL_CALL(glBindVertexArray(0));
            return;
        }

        m_current_pipeline = pipeline;

        // Enable/disable blending and color maks
        bool need_enable_blending = false;
        for (size_t i = 0; i < info.blend_states.size(); ++i) {
            auto& blend_state = info.blend_states[i];
            auto  gl_attachment_index = static_cast<GLuint>(i);
            if (blend_state.blend_enabled) {
                // Convert to gl format
                auto src_color_factor = to_gl_blend_factor(blend_state.src_color_factor);
                auto dst_color_factor = to_gl_blend_factor(blend_state.dst_color_factor);
                auto color_blend_op = to_gl_blend_op(blend_state.color_blend_op);
                auto src_alpha_factor = to_gl_blend_factor(blend_state.src_alpha_factor);
                auto dst_alpha_factor = to_gl_blend_factor(blend_state.dst_alpha_factor);
                auto alpha_blend_op = to_gl_blend_op(blend_state.alpha_blend_op);

                // Set params
                if (gl_attachment_index == 0) {
                    // Also enable blending for default framebuffer
                    GL_CALL(glBlendFuncSeparate(src_color_factor, dst_color_factor, src_alpha_factor, dst_alpha_factor));
                    GL_CALL(glBlendEquationSeparate(color_blend_op, alpha_blend_op));
                }
                // Enable blending for i attachment
                GL_CALL(glBlendFuncSeparatei(gl_attachment_index, src_color_factor, dst_color_factor, src_alpha_factor, dst_alpha_factor));
                GL_CALL(glBlendEquationSeparatei(gl_attachment_index, color_blend_op, alpha_blend_op));

                need_enable_blending = true;
            } else {
                // Disable blending for i attachment
                GL_CALL(glBlendFunci(gl_attachment_index, GL_ONE, GL_ZERO));
                GL_CALL(glBlendEquationi(gl_attachment_index, GL_FUNC_ADD));
            }

            // Enable color mask
            auto r_color_enabled = to_gl_bool(blend_state.mask.has_flag(color_mask::red));
            auto g_color_enabled = to_gl_bool(blend_state.mask.has_flag(color_mask::green));
            auto b_color_enabled = to_gl_bool(blend_state.mask.has_flag(color_mask::blue));
            auto a_color_enabled = to_gl_bool(blend_state.mask.has_flag(color_mask::alpha));
            if (gl_attachment_index == 0) {
                // Also enable for default framebuffer
                GL_CALL(glColorMask(r_color_enabled, g_color_enabled, b_color_enabled, a_color_enabled));
            }
            GL_CALL(glColorMaski(gl_attachment_index, r_color_enabled, g_color_enabled, b_color_enabled, a_color_enabled));
        }

        if (need_enable_blending) {
            GL_CALL(glEnable(GL_BLEND));
        } else {
            GL_CALL(glDisable(GL_BLEND));
        }

        // depth test
        if (info.depth_stencil.depth_test_enable) {
            GL_CALL(glEnable(GL_DEPTH_TEST));

            // depth write
            auto depth_write = to_gl_bool(info.depth_stencil.depth_write_enable);
            GL_CALL(glDepthMask(depth_write));

            // depth compare func
            auto depth_compare = to_gl_compare_func(info.depth_stencil.depth_compare);
            GL_CALL(glDepthFunc(depth_compare));
        } else {
            GL_CALL(glDisable(GL_DEPTH_TEST));
        }

        // stencil test
        if (info.depth_stencil.stencil_test_enable) {
            GL_CALL(glEnable(GL_STENCIL_TEST));

            // stencil front
            GL_CALL(glStencilFuncSeparate(
                GL_FRONT,
                to_gl_compare_func(info.depth_stencil.stencil_front.compare),
                info.depth_stencil.stencil_front.reference_value,
                info.depth_stencil.stencil_front.read_mask
            ));
            GL_CALL(glStencilOpSeparate(
                GL_FRONT,
                to_gl_stencil_op(info.depth_stencil.stencil_front.stencil_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_front.depth_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_front.pass_op)
            ));
            GL_CALL(glStencilMaskSeparate(GL_FRONT, info.depth_stencil.stencil_front.write_mask));

            // stencil back
            GL_CALL(glStencilFuncSeparate(
                GL_BACK,
                to_gl_compare_func(info.depth_stencil.stencil_back.compare),
                info.depth_stencil.stencil_back.reference_value,
                info.depth_stencil.stencil_back.read_mask
            ));
            GL_CALL(glStencilOpSeparate(
                GL_BACK,
                to_gl_stencil_op(info.depth_stencil.stencil_back.stencil_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_back.depth_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_back.pass_op)
            ));
            GL_CALL(glStencilMaskSeparate(GL_BACK, info.depth_stencil.stencil_back.write_mask));
        } else {
            GL_CALL(glDisable(GL_STENCIL_TEST));
        }

        // rasterizer state
        // cull face
        if (info.rasterizer.cull == cull_face::off) {
            GL_CALL(glDisable(GL_CULL_FACE));
        } else {
            GL_CALL(glEnable(GL_CULL_FACE));
            GL_CALL(glCullFace(to_gl_cull_face(info.rasterizer.cull)));
        }

        // front face
        GL_CALL(glFrontFace(to_gl_face(info.rasterizer.face)));

        // polygon mode
        GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, to_gl_polygon_mode(info.rasterizer.polygon)));

        // depth clamp
        if (info.rasterizer.depth_clamp_enable) {
            GL_CALL(glEnable(GL_DEPTH_CLAMP));
            GL_CALL(glDepthRange(info.rasterizer.depth_clamp_near, info.rasterizer.depth_clamp_far));
        } else {
            GL_CALL(glDisable(GL_DEPTH_CLAMP));
        }

        // depth bias
        auto gl_polygon_offset = to_gl_polygon_offset(info.rasterizer.polygon);
        if (info.rasterizer.depth_bias_enable) {
            GL_CALL(glEnable(gl_polygon_offset));
            GL_CALL(glPolygonOffset(info.rasterizer.depth_bias_factor, info.rasterizer.depth_bias));
        } else {
            GL_CALL(glDisable(gl_polygon_offset));
        }

        // Scissor test

        if (info.rasterizer.scissor_enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }

        // multisample state
        if (info.multisample.sample_shading_enabled) {
            GL_CALL(glEnable(GL_SAMPLE_SHADING));
            GL_CALL(glMinSampleShading(info.multisample.min_sample_shading));
        } else {
            GL_CALL(glDisable(GL_SAMPLE_SHADING));
        }

        GL_CALL(glUseProgram(p->program_obj));

        GL_CALL(glBindVertexArray(p->vao_obj));
    }

    void command_queue_opengl::bind_vertex_buffers(core::buffer_view<bind_buffer_info> buffers)
    {
        auto* p = m_device->get_resources()->try_get(m_current_pipeline);
        if (!p) {
            if (!m_current_pipeline) {
                ::logger.error("Failed to bind vertex buffers: no pipeline is bound");
            } else {
                ::logger.error("Failed to bind vertex buffers: pipeline {} not found", m_current_pipeline);
            }
            return;
        }

        if (buffers.size() != p->info.bindings.size()) {
            ::logger.error(
                "Failed to bind vertex buffers: buffers size {} mismatch with pipeline bindings size {}",
                fmt::styled_param(buffers.size()),
                fmt::styled_param(p->info.bindings.size())
            );
            return;
        }

        GLuint attrib_i = 0;
        for (auto& buf : buffers) {
            auto* b = m_device->get_resources()->try_get(buf.buffer);
            if (!b) {
                ::logger.error(
                    "Failed to bind vertex buffers: buffer {} not found",
                    buf.buffer
                );
                return;
            }

            if (b->info.usage != buffer_usage::vertex) {
                ::logger.error(
                    "Failed to bind vertex buffers: buffer {} not an vertex buffer",
                    buf.buffer
                );
                return;
            }

            auto& binding = p->info.bindings[static_cast<size_t>(attrib_i)];

            // Enable the vertex buffer
            GL_CALL(glBindVertexBuffer(
                attrib_i,
                b->buffer_obj,
                static_cast<GLintptr>(buf.base_offset),
                static_cast<GLsizei>(binding.stride)
            ));

            ++attrib_i;
        }
    }

    void command_queue_opengl::bind_index_buffer(buffer_handle buffer, index_buffer_format format)
    {
        auto* b = m_device->get_resources()->try_get(buffer);
        if (!b) {
            ::logger.error(
                "Failed to bind index buffer: buffer {} not found",
                buffer
            );
            return;
        }

        if (b->info.usage != buffer_usage::index) {
            ::logger.error(
                "Failed to bind index buffer: buffer {} not an index buffer",
                buffer
            );
            return;
        }

        m_current_index_buffer = buffer;
        m_current_index_buffer_format = format;

        GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->buffer_obj));
    }

    void command_queue_opengl::bind_shader_binding(shader_binding_handle shader_binding)
    {
        auto* sb = m_device->get_resources()->try_get(shader_binding);
        if (!sb) {
            ::logger.error("Failed to bind shader binding {}: shader binding not found", shader_binding);
            return;
        }

        auto& info = sb->info;

        // Bind textures and samplers
        for (uint32 i = 0; i < info.texture_bindings.size(); ++i) {
            auto& binding = info.texture_bindings[i];

            // Bind texture
            if (auto* t = m_device->get_resources()->try_get(binding.texture)) {
                if (!t->info.usage.has_flag(texture_usage::sampled)) {
                    ::logger.error("Failed to bind shader binding {}: texture {} is not sampled", shader_binding, binding.texture);
                    return;
                }
                GL_CALL(glActiveTexture(GL_TEXTURE0 + binding.binding));
                GL_CALL(glBindTexture(t->target, t->texture_obj));
            } else {
                ::logger.error("Failed to bind shader binding {}: texture {} not found", shader_binding, binding.texture);
                return;
            }

            // Bind sampler
            if (auto* s = m_device->get_resources()->try_get(binding.sampler)) {
                GL_CALL(glBindSampler(binding.binding, s->sampler_obj));
            } else {
                ::logger.error("Failed to bind shader binding {}: sampler {} not found", shader_binding, binding.sampler);
                return;
            }
        }

        // Bind buffers (UBO or SSBO buffers)
        for (uint32 i = 0; i < info.buffer_bindings.size(); ++i) {
            auto& binding = info.buffer_bindings[i];

            auto buf_h = binding.buffer;
            if (auto* b = m_device->get_resources()->try_get(buf_h)) {
                TAV_ASSERT(b->gl_target == GL_UNIFORM_BUFFER || b->gl_target == GL_SHADER_STORAGE_BUFFER);

                if (binding.size == 0) {
                    // Bind the entire buffer
                    GL_CALL(glBindBufferBase(b->gl_target, binding.binding, b->buffer_obj));
                } else {
                    // Bind a range of the buffer (subbuffer)
                    GL_CALL(glBindBufferRange(
                        b->gl_target,
                        binding.binding,
                        b->buffer_obj,
                        static_cast<GLintptr>(binding.offset),
                        static_cast<GLsizeiptr>(binding.size)
                    ));
                }
            } else {
                ::logger.error("Failed to bind shader binding {}: buffer {} not found", shader_binding, buf_h);
                return;
            }
        }
    }

    void command_queue_opengl::begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer)
    {
        if (m_current_render_pass || m_current_framebuffer) {
            ::logger.error("Failed to begin render pass {}: previous render pass {} not ended", render_pass, m_current_render_pass);
            return;
        }

        gl_render_pass* rp = m_device->get_resources()->try_get(render_pass);
        if (!rp) {
            ::logger.error("Failed to begin render pass {}: render pass not found", render_pass);
            return;
        }

        gl_framebuffer* fb = m_device->get_resources()->try_get(framebuffer);
        if (!fb) {
            ::logger.error("Failed to begin render pass {}: framebuffer {} not found", render_pass, framebuffer);
            return;
        }

        if (fb->is_default) {
            TAV_ASSERT(fb->framebuffer_obj == 0);
            TAV_ASSERT(fb->info.color_attachments.size() == 0);
            TAV_ASSERT(fb->info.sample_count == 1);
            TAV_ASSERT(rp->info.color_attachments.size() == 1);

            if (rp->info.color_attachments.size() != 1) {
                ::logger.error("Failed to begin render pass {}: number of color attachments for default framebuffer should be 1", render_pass);
                return;
            }

            if (rp->info.color_attachments[0].format != fb->default_fb_color_format) {
                ::logger.error(
                    "Failed to begin render pass {}: color attachment format {} does not match default framebuffer format {}",
                    render_pass,
                    rp->info.color_attachments[0].format,
                    fb->default_fb_color_format
                );
                return;
            }

            if (rp->info.depth_stencil_attachment.format != fb->default_fb_ds_format) {
                ::logger.error("Failed to begin render pass {}: depth/stencil attachment format {} does not match default framebuffer format {}", render_pass, rp->info.depth_stencil_attachment.format, fb->default_fb_ds_format);
                return;
            }

            GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

            GLbitfield clear_mask = 0;

            // Clear for color buffer
            if (rp->info.color_attachments[0].load == load_op::clear) {
                clear_mask |= GL_COLOR_BUFFER_BIT;
                auto color = rp->info.color_attachments[0].clear_value;
                GL_CALL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
                GL_CALL(glClearColor(color[0], color[1], color[2], color[3]));
            }

            if (rp->info.depth_stencil_attachment.format != pixel_format::none) {
                // Clear for depth buffer
                if (rp->info.depth_stencil_attachment.depth_load == load_op::clear) {
                    clear_mask |= GL_DEPTH_BUFFER_BIT;
                    GL_CALL(glDepthMask(GL_TRUE));
                    GL_CALL(glClearDepth(rp->info.depth_stencil_attachment.depth_clear_value));
                }
                // Clear for stencil buffer
                if (rp->info.depth_stencil_attachment.stencil_load == load_op::clear) {
                    clear_mask |= GL_STENCIL_BUFFER_BIT;
                    GL_CALL(glStencilMaskSeparate(GL_FRONT, 0xffffffff));
                    GL_CALL(glStencilMaskSeparate(GL_BACK, 0xffffffff));
                    GL_CALL(glClearStencil(rp->info.depth_stencil_attachment.stencil_clear_value));
                }
            }

            // Clear buffers if needed
            if (clear_mask) {
                GL_CALL(glClear(clear_mask));
            }

            m_current_framebuffer = framebuffer;
            m_current_render_pass = render_pass;

            return;
        }


        // Validate color attachments size
        if (rp->info.color_attachments.size() != fb->info.color_attachments.size()) {
            ::logger.error(
                "Failed to begin render pass {}: number of color attachments {} does not match framebuffer {} {}",
                render_pass,
                fmt::styled_param(rp->info.color_attachments.size()),
                framebuffer,
                fmt::styled_param(fb->info.color_attachments.size())
            );
            return;
        }


        // Validate attachments
        for (size_t i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto& attachment = rp->info.color_attachments[i];
            if (attachment.store == store_op::resolve) {
                // Validate that the resolve target attachment texture is single-sampled
                auto resolve_h = attachment.resolve_target;
                if (auto* dst_tex = m_device->get_resources()->try_get(resolve_h)) {
                    if (dst_tex->info.sample_count != 1) {
                        ::logger.error("Failed to begin render pass {}: resolve texture {} must be single-sampled", render_pass, resolve_h);
                        return;
                    }
                } else {
                    ::logger.error("Failed to begin render pass {}: resolve texture {} not found", render_pass, resolve_h);
                    return;
                }

                auto source_texture_h = fb->info.color_attachments[i];
                if (auto* tex = m_device->get_resources()->try_get(source_texture_h)) {
                    if (attachment.format != tex->info.format) {
                        ::logger.error("Failed to begin render pass {}: color attachment format mismatch with framebuffer {}", render_pass, framebuffer);
                        return;
                    }
                } else {
                    ::logger.error("Failed to begin render pass {}: color attachment texture {} not found", render_pass, source_texture_h);
                    return;
                }
            }
        }

        // Validate depth/stencil attachment format
        bool required_depth_stencil = rp->info.depth_stencil_attachment.format != pixel_format::none;
        bool provided_depth_stencil = fb->info.has_depth_stencil_attachment;
        if (required_depth_stencil != provided_depth_stencil) {
            ::logger.error(
                "Failed to begin render pass {}: depth/stencil is required but framebuffer {} doesnt has a depth/stencil",
                render_pass,
                framebuffer
            );
            return;
        }


        gl_texture* ds_attachment = nullptr;
        if (provided_depth_stencil) {
            ds_attachment = m_device->get_resources()->try_get(fb->info.depth_stencil_attachment);
            if (!ds_attachment) {
                ::logger.error("Failed to begin render pass {}: depth/stencil attachment {} not found", render_pass, fb->info.depth_stencil_attachment);
                return;
            }

            if (ds_attachment->info.format != rp->info.depth_stencil_attachment.format) {
                ::logger.error(
                    "Failed to begin render pass {}: framebuffer depth/stencil attachment format {} mismatch with renderpass {}",
                    render_pass,
                    ds_attachment->info.format,
                    rp->info.depth_stencil_attachment.format
                );
                return;
            }
        }


        // bind framebuffer
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer_obj));

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            ::logger.error("Failed to begin render pass {}: framebuffer {} is not complete", render_pass, framebuffer);
            return;
        }

        // Set viewport and scissor
        {
            auto w = static_cast<GLsizei>(fb->info.width);
            auto h = static_cast<GLsizei>(fb->info.height);
            GL_CALL(glViewport(0, 0, w, h));
            GL_CALL(glScissor(0, 0, w, h));
        }

        // Allpy load operations to the color attachments and depth/stencil attachment (only clear)
        auto color_attachments_size = static_cast<uint32>(fb->info.color_attachments.size());
        for (uint32 i = 0; i < color_attachments_size; ++i) {
            auto& rp_color_attachment = rp->info.color_attachments[i];

            // Apply load operation (only clear)
            // Any other load operation doesn't need to be applied
            if (load_op::clear == rp_color_attachment.load) {
                auto gl_attachment_index = static_cast<GLuint>(i);
                GL_CALL(glColorMaski(gl_attachment_index, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
                GL_CALL(glClearBufferfv(GL_COLOR, static_cast<GLint>(i), rp_color_attachment.clear_value));
            }
        }

        // Apply load operations to depth/stencil attachment
        if (provided_depth_stencil) {
            auto& rp_ds_attachment = rp->info.depth_stencil_attachment;

            // Apply load operation to depth component
            if (load_op::clear == rp_ds_attachment.depth_load) {
                GL_CALL(glDepthMask(GL_TRUE));
                GL_CALL(glClearBufferfv(GL_DEPTH, 0, &rp_ds_attachment.depth_clear_value));
            }

            // Apply load operation to stencil component
            if (load_op::clear == rp_ds_attachment.stencil_load) {
                GL_CALL(glStencilMaskSeparate(GL_FRONT, 0xffffffff));
                GL_CALL(glStencilMaskSeparate(GL_BACK, 0xffffffff));
                GL_CALL(glClearBufferiv(GL_STENCIL, 0, &rp_ds_attachment.stencil_clear_value));
            }
        }

        m_current_framebuffer = framebuffer;
        m_current_render_pass = render_pass;
    }

    void command_queue_opengl::end_render_pass()
    {
        if (!m_current_render_pass || !m_current_framebuffer) {
            ::logger.error("Failed to end render pass: render pass not started");
            return;
        }

        auto* rp = m_device->get_resources()->try_get(m_current_render_pass);
        if (!rp) {
            ::logger.error("Failed to end render pass {}: render pass not found", m_current_render_pass);
            return;
        }

        auto* fb = m_device->get_resources()->try_get(m_current_framebuffer);
        if (fb == nullptr) {
            ::logger.error("Failed to end render pass {}: framebuffer {} not found", m_current_render_pass, m_current_framebuffer);
            return;
        }

        if (fb->is_default) {
            TAV_ASSERT(rp->info.color_attachments[0].store != store_op::resolve);

            GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            m_current_framebuffer = framebuffer_handle();
            m_current_render_pass = render_pass_handle();
            GL_CALL(glBindVertexArray(0));

            return;
        }

        // Collect resolve attachments
        struct blit_info
        {
            GLuint      attachment = 0;
            gl_texture* dst = nullptr;
        };
        core::static_vector<blit_info, k_max_color_attachments> blit_data;
        for (uint32 i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto& rp_attachment = rp->info.color_attachments[i];

            if (store_op::resolve == rp_attachment.store) {
                // Find resolve resolve destination textures
                auto* dst = m_device->get_resources()->try_get(rp->info.color_attachments[i].resolve_target);
                TAV_ASSERT(dst);

                blit_data.push_back({GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), dst});
            }
        }

        // Blit mask for depth/stencil attachment
        GLbitfield depth_stencil_blit_mask = 0;
        if (store_op::resolve == rp->info.depth_stencil_attachment.depth_store) {
            depth_stencil_blit_mask |= GL_DEPTH_BUFFER_BIT;
        }
        if (store_op::resolve == rp->info.depth_stencil_attachment.stencil_store) {
            depth_stencil_blit_mask |= GL_STENCIL_BUFFER_BIT;
        }

        // Get resolve texture
        gl_texture* depth_stencil_resolve_dst = nullptr;
        if (depth_stencil_blit_mask != 0) {
            auto* dst = m_device->get_resources()->try_get(rp->info.depth_stencil_attachment.resolve_target);
            TAV_ASSERT(dst);
            depth_stencil_resolve_dst = dst;
        }

        auto need_resolve = depth_stencil_blit_mask != 0 || blit_data.size() > 0;
        if (need_resolve) {
            // Bind for resolve and attach textures
            GL_CALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolve_fbo));
            GL_CALL(glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->framebuffer_obj));

            // Check if need to resolve for any attachment
            if (blit_data.size() > 0) {
                // Always use COLOR_ATTACHMENT0 for drawing
                GL_CALL(glDrawBuffer(GL_COLOR_ATTACHMENT0));

                for (uint32 i = 0; i < blit_data.size(); ++i) {
                    auto& blit_info = blit_data[i];
                    TAV_ASSERT(blit_info.dst->target == GL_TEXTURE_2D);

                    GL_CALL(glFramebufferTexture2D(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        blit_info.dst->target,
                        blit_info.dst->texture_obj,
                        0
                    ));

                    // Resolve color attachments
                    GL_CALL(glReadBuffer(blit_info.attachment));
                    GL_CALL(glBlitFramebuffer(
                        0, 0, fb->info.width, fb->info.height,
                        0, 0, fb->info.width, fb->info.height,
                        GL_COLOR_BUFFER_BIT,
                        GL_NEAREST
                    ));
                }
            }

            // Check if need to resolve depth/stencil attachment
            if (depth_stencil_blit_mask) {
                TAV_ASSERT(depth_stencil_resolve_dst->target == GL_TEXTURE_2D);

                auto ds_info = to_depth_stencil_fromat(depth_stencil_resolve_dst->info.format);
                TAV_ASSERT(ds_info.is_depth_stencil_format);

                GL_CALL(glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    ds_info.depth_stencil_attachment_type,
                    depth_stencil_resolve_dst->target,
                    depth_stencil_resolve_dst->texture_obj,
                    0
                ));

                // Resolve depth/stencil attachment
                GL_CALL(glBlitFramebuffer(
                    0, 0, fb->info.width, fb->info.height,
                    0, 0, fb->info.width, fb->info.height,
                    depth_stencil_blit_mask,
                    GL_NEAREST // For depth/stencil, filter must be GL_NEAREST
                ));
            }

            GL_CALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
            GL_CALL(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
        }

        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        m_current_framebuffer = framebuffer_handle();
        m_current_render_pass = render_pass_handle();
        GL_CALL(glBindVertexArray(0));
    }

    void command_queue_opengl::set_viewport(const viewport_info& viewport)
    {
        TAV_ASSERT(viewport.width >= 1 && viewport.height >= 1);

        auto x = static_cast<GLint>(viewport.left);
        auto y = static_cast<GLint>(viewport.top);
        auto w = static_cast<GLsizei>(viewport.width);
        auto h = static_cast<GLsizei>(viewport.height);
        GL_CALL(glViewport(x, y, w, h));
    }

    void command_queue_opengl::set_scissor(const scissor_info& scissor)
    {
        TAV_ASSERT(scissor.width >= 0 && scissor.height >= 0);

        auto* fb = m_device->get_resources()->try_get(m_current_framebuffer);
        if (!fb) {
            ::logger.error("Failed to set scissor: no framebuffer is bound");
            return;
        }

        auto w = static_cast<GLsizei>(scissor.width);
        auto h = static_cast<GLsizei>(scissor.height);
        auto x = static_cast<GLint>(scissor.left);
        auto y = static_cast<GLint>(fb->info.height) - static_cast<GLint>(scissor.top) - static_cast<GLint>(h);
        GL_CALL(glScissor(x, y, w, h));
    }

    void command_queue_opengl::draw(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance)
    {
        auto* p = m_device->get_resources()->try_get(m_current_pipeline);
        if (!p) {
            if (!m_current_pipeline) {
                ::logger.error("Failed to draw: no pipeline is bound");
            } else {
                ::logger.error("Failed to draw: pipeline {} not found", m_current_pipeline);
            }
            return;
        }

        auto gl_topology = to_gl_topology(p->info.topology);
        auto gl_first_vertex = static_cast<GLint>(first_vertex);
        auto gl_vertex_count = static_cast<GLsizei>(vertex_count);
        auto gl_instance_count = static_cast<GLsizei>(instance_count);
        auto gl_first_instance = static_cast<GLuint>(first_instance);

        if (instance_count > 1 || first_instance > 0) {
            GL_CALL(glDrawArraysInstancedBaseInstance(
                gl_topology,
                gl_first_vertex,
                gl_vertex_count,
                gl_instance_count,
                gl_first_instance
            ));
        } else if (instance_count == 1) {
            GL_CALL(glDrawArrays(
                gl_topology,
                gl_first_vertex,
                gl_vertex_count
            ));
        } else {
            ::logger.warning("Failed to draw: instance count {} must be at least 1", fmt::styled_param((instance_count)));
        }
    }

    void command_queue_opengl::draw_indexed(uint32 index_count, uint32 first_index, uint32 vertex_offset, uint32 instance_count, uint32 first_instance)
    {
        if (index_count == 0 || instance_count == 0) {
            return;
        }

        auto* p = m_device->get_resources()->try_get(m_current_pipeline);
        if (!p) {
            if (!m_current_pipeline) {
                ::logger.error("Failed to draw indexed: no pipeline is bound");
            } else {
                ::logger.error("Failed to draw indexed: pipeline {} not found", m_current_pipeline);
            }
            return;
        }

        auto  gl_index_format = to_gl_index_format(m_current_index_buffer_format);
        auto  gl_topology = to_gl_topology(p->info.topology);
        auto* gl_index_offset = reinterpret_cast<const void*>(static_cast<size_t>(first_index * gl_index_format.size));
        auto  gl_index_count = static_cast<GLsizei>(index_count);
        auto  gl_instance_count = static_cast<GLsizei>(instance_count);
        auto  gl_first_instance = static_cast<GLuint>(first_instance);
        auto  gl_vertex_offset = static_cast<GLint>(vertex_offset);

        if (instance_count > 1 || first_instance > 0) {
            GL_CALL(glDrawElementsInstancedBaseVertexBaseInstance(
                gl_topology,
                gl_index_count,
                gl_index_format.type,
                gl_index_offset,
                gl_instance_count,
                gl_vertex_offset,
                gl_first_instance
            ));
        } else if (instance_count == 1) {
            GL_CALL(glDrawElementsBaseVertex(
                gl_topology,
                gl_index_count,
                gl_index_format.type,
                gl_index_offset,
                gl_vertex_offset
            ));
        } else {
            ::logger.warning("Failed to draw indexed: instance count {} must be at least 1", fmt::styled_param(instance_count));
        }
    }

    void command_queue_opengl::signal_fence(fence_handle fence)
    {
        auto* f = m_device->get_resources()->try_get(fence);
        if (!f) {
            ::logger.error("Failed to signal fence {}: fence not found", fence);
            return;
        }

        if (f->fence_obj) {
            GL_CALL(glDeleteSync(f->fence_obj));
        }

        GL_CALL(f->fence_obj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
    }

    void command_queue_opengl::wait_for_fence(fence_handle fence)
    {
        auto* f = m_device->get_resources()->try_get(fence);
        if (!f) {
            ::logger.error("Failed to wait for fence {}: fence not found", fence);
            return;
        }

        if (!f->fence_obj) {
            ::logger.error("Failed to wait for fence {}: fence not created", fence);
            return;
        }

        GL_CALL(glWaitSync(f->fence_obj, 0, GL_TIMEOUT_IGNORED));
    }

    void command_queue_opengl::copy_buffer(buffer_handle src_buffer, buffer_handle dst_buffer, size_t size, size_t src_offset, size_t dst_offset)
    {
        // Get dst and src buffers
        auto* src = m_device->get_resources()->try_get(src_buffer);
        if (!src) {
            ::logger.error("Failed to copy buffer {}: source buffer not found", src_buffer);
            return;
        }

        auto* dst = m_device->get_resources()->try_get(dst_buffer);
        if (!dst) {
            ::logger.error("Failed to copy buffer {}: destination buffer not found", dst_buffer);
            return;
        }

        // Check memory region
        if (dst_offset + size > dst->info.size) {
            ::logger.error(
                "Failed to copy buffer {}: destination buffer overflowed, offset {} + size {} exceeds buffer size {}",
                dst_buffer,
                fmt::styled_param(dst_offset),
                fmt::styled_param(size),
                fmt::styled_param(dst->info.size)
            );
            return;
        }

        if (src_offset + size > src->info.size) {
            ::logger.error(
                "Failed to copy buffer {}: source buffer overflowed, offset {} + size {} exceeds buffer size {}",
                src_buffer,
                fmt::styled_param(src_offset),
                fmt::styled_param(size),
                fmt::styled_param(src->info.size)
            );
            return;
        }

        // Check access
        // Allowed: cpu_to_gpu -> gpu_only
        // Allowed: gpu_only   -> gpu_only
        auto dst_access = dst->info.access;
        auto src_access = src->info.access;

        if (dst_access != buffer_access::gpu_only) {
            ::logger.error("Failed to copy buffer {}: destination buffer has invalid access {}", dst_buffer, dst_access);
            return;
        }

        if (src_access != buffer_access::cpu_to_gpu && src_access != buffer_access::gpu_only) {
            ::logger.error("Failed to copy buffer {}: source buffer has invalid access {}", src_buffer, src_access);
            return;
        }

        // Make copy data
        GL_CALL(glBindBuffer(GL_COPY_READ_BUFFER, src->buffer_obj));
        GL_CALL(glBindBuffer(GL_COPY_WRITE_BUFFER, dst->buffer_obj));

        GL_CALL(glCopyBufferSubData(
            GL_COPY_READ_BUFFER,
            GL_COPY_WRITE_BUFFER,
            static_cast<GLintptr>(src_offset),
            static_cast<GLintptr>(dst_offset),
            static_cast<GLsizeiptr>(size)
        ));

        GL_CALL(glBindBuffer(GL_COPY_READ_BUFFER, 0));
        GL_CALL(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
    }

    void command_queue_opengl::copy_buffer_to_texture(buffer_handle src_buffer, texture_handle dst_texture, const texture_copy_region& region)
    {
        auto* b = m_device->get_resources()->try_get(src_buffer);
        if (!b) {
            ::logger.error("Failed to copy buffer {} to texture {}: source buffer not found", src_buffer, dst_texture);
            return;
        }

        auto* tex = m_device->get_resources()->try_get(dst_texture);
        if (!tex) {
            ::logger.error("Failed to copy buffer {} to texture {}: destination buffer not found", src_buffer, dst_texture);
            return;
        }

        auto& tinfo = tex->info;

        if (b->info.usage != buffer_usage::stage) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: invalid source buffer usage type, expected `stage` got {}",
                src_buffer, dst_texture, b->info.usage
            );
            return;
        }

        if (b->info.access != buffer_access::cpu_to_gpu) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: invalid source buffer access type, expected `cpu_to_gpu` got {}",
                src_buffer, dst_texture, b->info.access
            );
            return;
        }

        if (tinfo.sample_count != 1) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: destination texture has invalid sample count {}, only 1 is supported",
                src_buffer, dst_texture,
                fmt::styled_param(tex->info.sample_count)
            );
            return;
        }

        if (!tinfo.usage.has_flag(texture_usage::transfer_destination)) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: destination texture does not have `transfer_destination` usage flag",
                src_buffer, dst_texture
            );
            return;
        }

        if (!is_color_format(tinfo.format)) {
            ::logger.error("Failed to copy buffer {} to texture {}: destination texture format is not a color format", src_buffer, dst_texture);
            return;
        }

        if (region.mip_level >= tex->max_mip) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: mip_level ({}) exceeds max mip level ({}).",
                src_buffer, dst_texture, region.mip_level, tex->max_mip
            );
            return;
        }

        if (region.width == 0 || region.height == 0) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: region dimensions are invalid (width={}, height={}). "
                "Width and height must be greater than zero.",
                src_buffer, dst_texture, region.width, region.height
            );
            return;
        }

        auto max_w = math::mip_side(tinfo.width, region.mip_level);
        auto max_h = math::mip_side(tinfo.height, region.mip_level);

        if (region.x_offset + region.width > max_w || region.y_offset + max_h > tinfo.height) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: region out of bounds. "
                "Region (x_offset={}, y_offset={}, width={}, height={}) exceeds mip level () size ({}x{}).",
                src_buffer, dst_texture,
                region.x_offset, region.y_offset, region.width, region.height,
                region.mip_level, max_w, max_h
            );
            return;
        }

        if (tinfo.type == texture_type::texture_2d || tinfo.type == texture_type::texture_cube) {
            if (region.depth != 1 || region.z_offset != 0) {
                ::logger.error(
                    "Failed to copy buffer {} to texture {}: invalid depth ({}) or z_offset ({}). "
                    "Depth must be 1 and z_offset must be 0 for texture_2d or texture_cube.",
                    src_buffer, dst_texture, region.depth, region.z_offset
                );
                return;
            }
        } else if (tinfo.type == texture_type::texture_3d) {
            auto max_d = math::mip_side(tinfo.depth, region.mip_level);

            if (region.depth == 0) {
                ::logger.error(
                    "Failed to copy buffer {} to texture {}: invalid depth ({}). "
                    "Depth must be greater than zero for texture_3d.",
                    src_buffer, dst_texture, region.depth
                );
                return;
            }

            if (region.z_offset + region.depth > max_d) {
                ::logger.error(
                    "Failed to copy buffer {} to texture {}: region z-range out of bounds. "
                    "Region (z_offset={}, depth={}) exceeds mip level ({}) depth ({}).",
                    src_buffer, dst_texture,
                    region.z_offset, region.depth, region.mip_level, max_d
                );
                return;
            }
        } else {
            TAV_UNREACHABLE();
        }

        auto   gl_pixel_format = to_gl_pixel_format(tinfo.format);
        uint32 real_row_bytes = tinfo.width * gl_pixel_format.bytes;
        size_t stride_bytes = static_cast<size_t>(region.buffer_row_length > 0 ? region.buffer_row_length * gl_pixel_format.bytes : real_row_bytes);
        size_t need_bytes = stride_bytes * tinfo.height * tinfo.depth - (stride_bytes - real_row_bytes);

        if (region.buffer_offset + need_bytes > b->info.size) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: buffer is too small. Required {} bytes, available {} bytes",
                src_buffer, dst_texture, fmt::styled_param(region.buffer_offset + need_bytes), fmt::styled_param(b->info.size)
            );
            return;
        }

        GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, b->buffer_obj));
        GL_CALL(glBindTexture(tex->target, tex->texture_obj));

        auto row_length_in_pixels = static_cast<GLint>(region.buffer_row_length);
        GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_in_pixels));
        GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        auto* gl_buffer_offset = reinterpret_cast<const void*>(region.buffer_offset);

        static constexpr GLenum faces[6] = {
            GL_TEXTURE_CUBE_MAP_POSITIVE_X,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
        };

        switch (tinfo.type) {
        case texture_type::texture_2d:
            GL_CALL(glTexSubImage2D(
                GL_TEXTURE_2D,
                static_cast<GLint>(region.mip_level),
                static_cast<GLint>(region.x_offset),
                static_cast<GLint>(region.y_offset),
                static_cast<GLsizei>(region.width),
                static_cast<GLsizei>(region.height),
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));
            break;

        case texture_type::texture_3d:
            GL_CALL(glTexSubImage3D(
                GL_TEXTURE_3D,
                static_cast<GLint>(region.mip_level),
                static_cast<GLint>(region.x_offset),
                static_cast<GLint>(region.y_offset),
                static_cast<GLint>(region.z_offset),
                static_cast<GLsizei>(region.width),
                static_cast<GLsizei>(region.height),
                static_cast<GLsizei>(region.depth),
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));
            break;

        case texture_type::texture_cube:
            GL_CALL(glTexSubImage2D(
                faces[region.layer_index % 6],
                static_cast<GLint>(region.mip_level),
                static_cast<GLint>(region.x_offset),
                static_cast<GLint>(region.y_offset),
                static_cast<GLsizei>(region.width),
                static_cast<GLsizei>(region.height),
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));
            break;

        default:
            TAV_UNREACHABLE();
            break;
        }

        GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
        GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));

        GL_CALL(glBindTexture(tex->target, 0));
        GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    }

    void command_queue_opengl::copy_texture_to_buffer(texture_handle src_texture, buffer_handle dst_buffer, const texture_copy_region& region)
    {
        auto* tex = m_device->get_resources()->try_get(src_texture);
        if (!tex) {
            ::logger.error("Failed to copy texture {} to buffer {}: source texture not found", src_texture, dst_buffer);
            return;
        }

        auto* b = m_device->get_resources()->try_get(dst_buffer);
        if (!b) {
            ::logger.error("Failed to copy texture {} to buffer {}: destination buffer not found", src_texture, dst_buffer);
            return;
        }

        auto& tinfo = tex->info;

        if (b->info.usage != buffer_usage::stage) {
            ::logger.error(
                "Failed to copy texture {} to buffer {}: invalid destination buffer usage type, expected `stage` got {}",
                src_texture, dst_buffer, b->info.usage
            );
            return;
        }

        if (b->info.access != buffer_access::gpu_to_cpu) {
            ::logger.error(
                "Failed to copy texture {} to buffer {}: invalid destination buffer access type, expected `gpu_to_cpu` got {}",
                src_texture, dst_buffer, b->info.access
            );
            return;
        }

        if (!tinfo.usage.has_flag(texture_usage::transfer_source)) {
            ::logger.error(
                "Failed to copy texture {} to buffer {}: source texture does not have `transfer_source` usage flag",
                src_texture, dst_buffer
            );
            return;
        }

        auto gl_pixel_format = to_gl_pixel_format(tinfo.format);

        uint32 real_row_bytes = tinfo.width * gl_pixel_format.bytes;
        size_t stride_bytes = static_cast<size_t>(region.buffer_row_length > 0 ? region.buffer_row_length * gl_pixel_format.bytes : real_row_bytes);
        size_t need_bytes = stride_bytes * tinfo.height * tinfo.depth - (stride_bytes - real_row_bytes);

        if (region.buffer_offset + need_bytes > b->info.size) {
            ::logger.error(
                "Failed to copy texture {} to buffer {}: buffer is too small. Required {} bytes, available {}",
                src_texture, dst_buffer, fmt::styled_param(region.buffer_offset + need_bytes), fmt::styled_param(b->info.size)
            );
            return;
        }

        auto max_w = math::mip_side(tinfo.width, region.mip_level);
        auto max_h = math::mip_side(tinfo.height, region.mip_level);

        if (region.x_offset + region.width > max_w || region.y_offset + max_h > tinfo.height) {
            ::logger.error(
                "Failed to copy texture {} to buffer {}: region out of bounds. "
                "Region (x_offset={}, y_offset={}, width={}, height={}) exceeds mip level () size ({}x{}).",
                src_texture, dst_buffer,
                region.x_offset, region.y_offset, region.width, region.height,
                region.mip_level, max_w, max_h
            );
            return;
        }

        GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, b->buffer_obj));
        GL_CALL(glBindTexture(tex->target, tex->texture_obj));

        GL_CALL(glPixelStorei(GL_PACK_ROW_LENGTH, static_cast<GLint>(region.buffer_row_length)));
        GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 1));

        auto* gl_buffer_offset = reinterpret_cast<void*>(region.buffer_offset);

        if (tinfo.type == texture_type::texture_2d) {
            auto  is_rb = GL_RENDERBUFFER == tex->target;
            GLint prev_read_fbo = 0;

            if (!is_rb) {
                // Is not a renderbuffer, so, attach texture to the read from framebuffer
                GL_CALL(glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo));

                TAV_ASSERT(m_resolve_fbo);

                GL_CALL(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_resolve_fbo));

                GLenum read_attachment = GL_COLOR_ATTACHMENT0;
                auto   ds_fmt = to_depth_stencil_fromat(tex->info.format);
                if (ds_fmt.is_depth_stencil_format) {
                    read_attachment = ds_fmt.depth_stencil_attachment_type;
                }

                if (tex->target == GL_TEXTURE_2D) {
                    GL_CALL(glFramebufferTexture2D(
                        GL_READ_FRAMEBUFFER,
                        read_attachment,
                        tex->target,
                        tex->texture_obj,
                        static_cast<GLint>(region.mip_level)
                    ));
                } else {
                    GL_CALL(glFramebufferTexture(
                        GL_READ_FRAMEBUFFER,
                        read_attachment,
                        tex->texture_obj,
                        static_cast<GLint>(region.mip_level)
                    ));
                }

                // Set read buffer only with color attachment
                if (!ds_fmt.is_depth_stencil_format) {
                    GL_CALL(glReadBuffer(read_attachment));
                }

                if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)) {
                    ::logger.error(
                        "Failed to copy texture {} to buffer {}: framebuffer is not complete",
                        src_texture, dst_buffer
                    );
                    return;
                }
            }

            // Read pixels to the pbo
            GL_CALL(glReadPixels(
                static_cast<GLint>(region.x_offset),
                static_cast<GLint>(region.y_offset),
                static_cast<GLsizei>(region.width),
                static_cast<GLsizei>(region.height),
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));

            if (!is_rb) {
                // Restore old read framebuffer
                GL_CALL(glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo));
            }
        } else if (tinfo.type == texture_type::texture_3d) {
            GL_CALL(glGetTexImage(
                GL_TEXTURE_3D,
                static_cast<GLint>(region.mip_level),
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));
        } else if (tinfo.type == texture_type::texture_cube) {
            static constexpr GLenum faces[6] = {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
            };

            GL_CALL(glGetTexImage(
                faces[region.layer_index % 6],
                static_cast<GLint>(region.mip_level),
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));
        } else {
            TAV_UNREACHABLE();
        }

        GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 4));
        GL_CALL(glPixelStorei(GL_PACK_ROW_LENGTH, 0));

        GL_CALL(glBindTexture(tex->target, 0));
        GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
    }


} // namespace tavros::renderer::rhi
