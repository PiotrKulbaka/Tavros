#include <tavros/renderer/internal/opengl/command_list_opengl.hpp>

#include <tavros/renderer/internal/opengl/type_conversions.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>
#include <tavros/renderer/internal/opengl/gl_check.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <glad/glad.h>

using namespace tavros::renderer::rhi;

namespace
{
    tavros::core::logger logger("command_list_opengl");

    GLboolean to_gl_bool(bool value) noexcept
    {
        return value ? GL_TRUE : GL_FALSE;
    }
} // namespace

namespace tavros::renderer::rhi
{

    command_list_opengl::command_list_opengl(graphics_device_opengl* device)
        : m_device(device)
    {
        ::logger.debug("command_list_opengl created");

        GL_CALL(glGenFramebuffers(1, &m_resolve_fbo));
    }

    command_list_opengl::~command_list_opengl()
    {
        GL_CALL(glDeleteFramebuffers(1, &m_resolve_fbo));

        ::logger.debug("command_list_opengl destroyed");
    }

    void command_list_opengl::bind_pipeline(pipeline_handle pipeline)
    {
        auto* p = m_device->get_resources()->try_get(pipeline);
        if (!p) {
            ::logger.error("Failed to bind pipeline {}: not found", pipeline);
            GL_CALL(glUseProgram(0));
            return;
        }

        m_current_pipeline = pipeline;
        auto& info = p->info;

        auto* pass = m_device->get_resources()->try_get(m_current_render_pass);
        if (!pass) {
            ::logger.error("Failed to bind pipeline {}: render pass {} not found", pipeline, m_current_render_pass);
            GL_CALL(glUseProgram(0));
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
            return;
        }

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

        // multisample state
        if (info.multisample.sample_shading_enabled) {
            GL_CALL(glEnable(GL_SAMPLE_SHADING));
            GL_CALL(glMinSampleShading(info.multisample.min_sample_shading));
        } else {
            GL_CALL(glDisable(GL_SAMPLE_SHADING));
        }

        GL_CALL(glUseProgram(p->program_obj));
    }

    void command_list_opengl::bind_geometry(geometry_handle geometry)
    {
        if (auto* g = m_device->get_resources()->try_get(geometry)) {
            GL_CALL(glBindVertexArray(g->vao_obj));
            m_current_geometry = geometry;
        } else {
            ::logger.error("Failed to bind geometry binding {}: not found", geometry);
            GL_CALL(glBindVertexArray(0));
            m_current_geometry = geometry_handle();
        }
    }

    void command_list_opengl::bind_shader_binding(shader_binding_handle shader_binding)
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
            auto tex_h = sb->textures[binding.texture_index];
            if (auto* t = m_device->get_resources()->try_get(tex_h)) {
                if (!t->info.usage.has_flag(texture_usage::sampled)) {
                    ::logger.error("Failed to bind shader binding {}: texture {} is not sampled", shader_binding, tex_h);
                    return;
                }
                GL_CALL(glActiveTexture(GL_TEXTURE0 + binding.binding));
                GL_CALL(glBindTexture(t->target, t->texture_obj));
            } else {
                ::logger.error("Failed to bind shader binding {}: texture {} not found", shader_binding, tex_h);
                return;
            }

            // Bind sampler
            auto sampler_h = sb->samplers[binding.sampler_index];
            if (auto* s = m_device->get_resources()->try_get(sampler_h)) {
                GL_CALL(glBindSampler(binding.binding, s->sampler_obj));
            } else {
                ::logger.error("Failed to bind shader binding {}: sampler {} not found", shader_binding, sampler_h);
                return;
            }
        }

        // Bind buffers (UBO or SSBO buffers)
        for (uint32 i = 0; i < info.buffer_bindings.size(); ++i) {
            auto& binding = info.buffer_bindings[i];

            auto buf_h = sb->buffers[binding.buffer_index];
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

    void command_list_opengl::begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer)
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
        if (fb == nullptr) {
            ::logger.error("Failed to begin render pass {}: framebuffer {} not found", render_pass, framebuffer);
            return;
        }

        // Validate color attachments size
        if (rp->info.color_attachments.size() != fb->info.color_attachment_formats.size()) {
            ::logger.error("Failed to begin render pass {}: number of color attachments does not match framebuffer {}", render_pass, framebuffer);
            return;
        }

        // Validate color attachments format
        for (uint32 i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto rp_color_format = rp->info.color_attachments[i].format;
            auto fb_color_format = fb->info.color_attachment_formats[i];
            if (rp_color_format != fb_color_format) {
                ::logger.error("Failed to begin render pass {}: color attachment format mismatch with framebuffer {}", render_pass, framebuffer);
                return;
            }
        }

        // Validate resolve attachments
        for (uint32 i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto& attachment = rp->info.color_attachments[i];
            if (attachment.store == store_op::resolve) {
                auto resolve_attachment_index = attachment.resolve_texture_index;
                // Validate resolve target index
                if (resolve_attachment_index >= rp->resolve_attachments.size()) {
                    ::logger.error("Failed to begin render pass {}: invalid resolve target attachment index", render_pass);
                    return;
                }
                // Validate that the resolve target attachment format matches
                auto resolve_target_format = rp->info.color_attachments[resolve_attachment_index].format;
                if (attachment.format != resolve_target_format) {
                    ::logger.error("Failed to begin render pass {}: mismatched resolve target attachment format", render_pass);
                    return;
                }
                // Validate that the resolve target attachment texture is single-sampled
                auto resolve_texture_h = rp->resolve_attachments[resolve_attachment_index];
                if (auto* tex = m_device->get_resources()->try_get(resolve_texture_h)) {
                    if (tex->info.sample_count != 1) {
                        ::logger.error("Failed to begin render pass {}: resolve target attachment texture {} must be single-sampled", render_pass, resolve_texture_h);
                        return;
                    }
                } else {
                    ::logger.error("Failed to begin render pass {}: resolve texture {} not found", render_pass, resolve_texture_h);
                    return;
                }

                auto source_texture_h = fb->color_attachments[i];
                if (auto* tex = m_device->get_resources()->try_get(source_texture_h); !tex) {
                    ::logger.error("Failed to begin render pass {}: source attachment texture {} not found", render_pass, source_texture_h);
                    return;
                }
            }
        }

        // Validate depth/stencil attachment format
        if (rp->info.depth_stencil_attachment.format != fb->info.depth_stencil_attachment_format) {
            ::logger.error("Failed to begin render pass {}: mismatched depth/stencil attachment format with framebuffer {}", render_pass, framebuffer);
            return;
        }

        // bind framebuffer
        if (fb->is_default) {
            TAV_ASSERT(fb->framebuffer_obj == 0);
            TAV_ASSERT(rp->info.color_attachments.size() == 1);

            GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        } else {
            GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer_obj));

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                ::logger.error("Failed to begin render pass {}: framebuffer {} is not complete", render_pass, framebuffer);
                return;
            }
        }

        // Set viewport
        GL_CALL(glViewport(0, 0, fb->info.width, fb->info.height));

        // Allpy load operations to the color attachments and depth/stencil attachment
        // Only clear is supported, any other load operations are ignored
        if (fb->is_default) {
            GLbitfield clear_mask = 0;
            // Clear for color buffer
            if (rp->info.color_attachments[0].load == load_op::clear) {
                clear_mask |= GL_COLOR_BUFFER_BIT;
                auto color = rp->info.color_attachments[0].clear_value;
                GL_CALL(glClearColor(color[0], color[1], color[2], color[3]));
            }

            if (rp->info.depth_stencil_attachment.format != pixel_format::none) {
                // Clear for depth buffer
                if (rp->info.depth_stencil_attachment.depth_load == load_op::clear) {
                    clear_mask |= GL_DEPTH_BUFFER_BIT;
                    GL_CALL(glClearDepth(rp->info.depth_stencil_attachment.depth_clear_value));
                }
                // Clear for stencil buffer
                if (rp->info.depth_stencil_attachment.stencil_load == load_op::clear) {
                    clear_mask |= GL_STENCIL_BUFFER_BIT;
                    GL_CALL(glClearStencil(rp->info.depth_stencil_attachment.stencil_clear_value));
                }
            }

            // Clear buffers if needed
            if (clear_mask) {
                GL_CALL(glClear(clear_mask));
            }
        } else {
            // Apply load operations (only clear)
            for (uint32 attachment_index = 0; attachment_index < fb->color_attachments.size(); ++attachment_index) {
                auto attachment_h = fb->color_attachments[attachment_index];
                if (auto* tex = m_device->get_resources()->try_get(attachment_h)) {
                    auto& rp_color_attachment = rp->info.color_attachments[attachment_index];

                    // Apply load operation (only clear)
                    // Any other load operation doesn't need to be applied
                    if (load_op::clear == rp_color_attachment.load) {
                        GL_CALL(glClearBufferfv(GL_COLOR, attachment_index, rp_color_attachment.clear_value));
                    }
                } else {
                    ::logger.error("Failed to begin render pass {}: attachment texture {} not found", render_pass, attachment_h);
                    return;
                }
            }

            // Apply load operations to depth/stencil attachment
            if (rp->info.depth_stencil_attachment.format != pixel_format::none) {
                auto& rp_depth_stencil_attachment = rp->info.depth_stencil_attachment;

                // Apply load operation to depth component
                if (rp_depth_stencil_attachment.depth_load == load_op::clear) {
                    GL_CALL(glClearBufferfv(GL_DEPTH, 0, &rp_depth_stencil_attachment.depth_clear_value));
                }

                // Apply load operation to stencil component
                if (rp_depth_stencil_attachment.stencil_load == load_op::clear) {
                    GL_CALL(glClearBufferiv(GL_STENCIL, 0, &rp_depth_stencil_attachment.stencil_clear_value));
                }
            }
        }

        m_current_framebuffer = framebuffer;
        m_current_render_pass = render_pass;
    }

    void command_list_opengl::end_render_pass()
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
            return;
        }

        // Collect resolve attachments
        struct blit_info
        {
            GLuint      attachment = 0;
            gl_texture* source = nullptr;
            gl_texture* destination = nullptr;
        };
        core::static_vector<blit_info, k_max_color_attachments> blit_data;
        GLuint                                                  attachment_index = 0;
        for (uint32 i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto& rp_color_attachment = rp->info.color_attachments[i];
            auto* source_tex = m_device->get_resources()->try_get(fb->color_attachments[i]);
            TAV_ASSERT(source_tex);

            if (rp_color_attachment.store == store_op::resolve) {
                // Resolve attachments
                auto resolve_texture_index = rp_color_attachment.resolve_texture_index;

                if (rp->resolve_attachments.size() > resolve_texture_index) {
                    // Find the resolve texture and validate it
                    auto  resolve_attachment_h = rp->resolve_attachments[resolve_texture_index];
                    auto* resolve_tex = m_device->get_resources()->try_get(resolve_attachment_h);
                    if (resolve_tex == nullptr) {
                        ::logger.error("Failed to end render pass {}: resolve attachment texture {} not found", m_current_render_pass, fmt::styled_param(resolve_texture_index));
                        return;
                    }

                    // everything is ok, add to the list
                    blit_data.push_back({GL_COLOR_ATTACHMENT0 + attachment_index, source_tex, resolve_tex});
                    attachment_index++;
                } else {
                    ::logger.error("Failed to end render pass {}: invalid resolve attachment index {}", m_current_render_pass, fmt::styled_param(resolve_texture_index));
                    return;
                }
            }
        }

        // Blit mask for depth/stencil attachment
        GLbitfield depth_stencil_blit_mask = 0;
        if (rp->info.depth_stencil_attachment.depth_store == store_op::resolve) {
            depth_stencil_blit_mask |= GL_DEPTH_BUFFER_BIT;
        }
        if (rp->info.depth_stencil_attachment.stencil_store == store_op::resolve) {
            depth_stencil_blit_mask |= GL_STENCIL_BUFFER_BIT;
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
                    TAV_ASSERT(blit_info.destination->target == GL_TEXTURE_2D);

                    GL_CALL(glFramebufferTexture2D(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        blit_info.destination->target,
                        blit_info.destination->texture_obj,
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
        m_current_geometry = geometry_handle();
    }

    void command_list_opengl::draw(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance)
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

    void command_list_opengl::draw_indexed(uint32 index_count, uint32 first_index, uint32 vertex_offset, uint32 instance_count, uint32 first_instance)
    {
        // TODO: unused vertex_offset
        TAV_UNUSED(vertex_offset);

        auto* p = m_device->get_resources()->try_get(m_current_pipeline);
        if (!p) {
            if (!m_current_pipeline) {
                ::logger.error("Failed to draw indexed: no pipeline is bound");
            } else {
                ::logger.error("Failed to draw indexed: pipeline {} not found", m_current_pipeline);
            }
            return;
        }

        auto* g = m_device->get_resources()->try_get(m_current_geometry);
        if (!g) {
            if (!m_current_geometry) {
                ::logger.error("Failed to draw indexed: no geometry binding is bound");
            } else {
                ::logger.error("Failed to draw indexed: geometry binding {} not found", m_current_geometry);
            }
            return;
        }

        if (!g->info.has_index_buffer) {
            ::logger.error("Failed to draw indexed: current geometry binding {} has no index buffer", m_current_geometry);
            return;
        }

        auto  gl_index_format = to_gl_index_format(g->info.index_format);
        auto  gl_topology = to_gl_topology(p->info.topology);
        auto* gl_index_offset = reinterpret_cast<const void*>(static_cast<size_t>(first_index * gl_index_format.size));
        auto  gl_index_count = static_cast<GLsizei>(index_count);
        auto  gl_instance_count = static_cast<GLsizei>(instance_count);
        auto  gl_first_instance = static_cast<GLuint>(first_instance);

        if (instance_count > 1 || first_instance > 0) {
            GL_CALL(glDrawElementsInstancedBaseInstance(
                gl_topology,
                gl_index_count,
                gl_index_format.type,
                gl_index_offset,
                gl_instance_count,
                gl_first_instance
            ));
        } else if (instance_count == 1) {
            GL_CALL(glDrawElements(
                gl_topology,
                gl_index_count,
                gl_index_format.type,
                gl_index_offset
            ));
        } else {
            ::logger.warning("Failed to draw indexed: instance count {} must be at least 1", fmt::styled_param(instance_count));
        }
    }

    void command_list_opengl::copy_buffer(buffer_handle src_buffer, buffer_handle dst_buffer, size_t size, size_t src_offset, size_t dst_offset)
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

    void command_list_opengl::copy_buffer_to_texture(buffer_handle src_buffer, texture_handle dst_texture, uint32 layer_index, size_t size, size_t src_offset, uint32 row_stride)
    {
        // TODO: unused size
        TAV_UNUSED(size);

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
            ::logger.error("Failed to copy buffer {} to texture {}: invalid source buffer usage type, expected `stage` got {}", src_buffer, dst_texture, b->info.usage);
            return;
        }

        if (b->info.access != buffer_access::cpu_to_gpu) {
            ::logger.error("Failed to copy buffer {} to texture {}: invalid source buffer access type, expected `cpu_to_gpu` got {}", src_buffer, dst_texture, b->info.access);
            return;
        }

        if (tinfo.sample_count != 1) {
            ::logger.error("Failed to copy buffer {} to texture {}: destination texture has invalid sample count {}, only 1 is supported", src_buffer, dst_texture, fmt::styled_param(tex->info.sample_count));
            return;
        }

        if (!tinfo.usage.has_flag(texture_usage::transfer_destination)) {
            ::logger.error("Failed to copy buffer {} to texture {}: destination texture does not have `transfer_destination` usage flag", src_buffer, dst_texture);
            return;
        }

        if (!is_color_format(tinfo.format)) {
            ::logger.error("Failed to copy buffer {} to texture {}: destination texture format is not a color format", src_buffer, dst_texture);
            return;
        }

        auto gl_pixel_format = to_gl_pixel_format(tinfo.format);
        if (row_stride % gl_pixel_format.bytes != 0) {
            ::logger.error("Failed to copy buffer {} to texture {}: row stride {} must be aligned to pixel size {}", src_buffer, dst_texture, fmt::styled_param(row_stride), fmt::styled_param(gl_pixel_format.bytes));
            return;
        }

        uint32 real_row_bytes = tinfo.width * gl_pixel_format.bytes;
        if (row_stride > 0 && row_stride < real_row_bytes) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: row stride {} less than required row size {}",
                src_buffer,
                dst_texture,
                fmt::styled_param(row_stride),
                fmt::styled_param(real_row_bytes)
            );
            return;
        }

        size_t stride_bytes = static_cast<size_t>(row_stride > 0 ? row_stride : real_row_bytes);
        size_t need_bytes = stride_bytes * tinfo.height * tinfo.depth - (stride_bytes - real_row_bytes);

        if (src_offset + need_bytes > b->info.size) {
            ::logger.error(
                "Failed to copy buffer {} to texture {}: buffer is too small. Required {} bytes, available {} bytes",
                src_buffer,
                dst_texture,
                fmt::styled_param(src_offset + need_bytes),
                fmt::styled_param(b->info.size)
            );
            return;
        }

        GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, b->buffer_obj));
        GL_CALL(glBindTexture(tex->target, tex->texture_obj));

        auto row_length_in_pixels = static_cast<GLint>(row_stride / gl_pixel_format.bytes);
        GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_in_pixels));
        GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        auto* gl_buffer_offset = reinterpret_cast<const void*>(src_offset);

        if (tinfo.type == texture_type::texture_2d) {
            GL_CALL(glTexSubImage2D(
                GL_TEXTURE_2D,
                0,    // Mip level
                0, 0, // xoffset, yoffset
                tinfo.width, tinfo.height,
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));
        } else if (tinfo.type == texture_type::texture_3d) {
            GL_CALL(glTexSubImage3D(
                GL_TEXTURE_3D,
                0,       // mip level
                0, 0, 0, // xoffset, yoffset, zoffset
                tinfo.width, tinfo.height, tinfo.depth,
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

            GL_CALL(glTexSubImage2D(
                faces[layer_index % 6],
                0,    // mip level
                0, 0, // xoffset, yoffset
                tinfo.width, tinfo.height,
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            ));
        } else {
            TAV_UNREACHABLE();
        }

        GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
        GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));

        if (tinfo.mip_levels > 1) {
            // This call gets warning for texture_cube:
            // warning [OpenGL_debug] Pixel-path performance warning: Pixel transfer is synchronized with 3D rendering
            // TODO: fix it
            GL_CALL(glGenerateMipmap(tex->target));
        }

        GL_CALL(glBindTexture(tex->target, 0));
        GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    }

    void command_list_opengl::copy_texture_to_buffer(texture_handle src_texture, buffer_handle dst_buffer, uint32 layer_index, size_t size, size_t dst_offset, uint32 row_stride)
    {
        TAV_UNUSED(size);

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
            ::logger.error("Failed to copy texture {} to buffer {}: invalid destination buffer usage type, expected `stage` got {}", src_texture, dst_buffer, b->info.usage);
            return;
        }

        if (b->info.access != buffer_access::gpu_to_cpu) {
            ::logger.error("Failed to copy texture {} to buffer {}: invalid destination buffer access type, expected `gpu_to_cpu` got {}", src_texture, dst_buffer, b->info.access);
            return;
        }

        if (!tinfo.usage.has_flag(texture_usage::transfer_source)) {
            ::logger.error("Failed to copy texture {} to buffer {}: source texture does not have `transfer_source` usage flag", src_texture, dst_buffer);
            return;
        }

        /*if (!is_color_format(tinfo.format)) {
            ::logger.error("Failed to copy texture {} to buffer {}: texture format is not a color format", src_texture, dst_buffer);
            return;
        }*/

        auto gl_pixel_format = to_gl_pixel_format(tinfo.format);
        if (row_stride % gl_pixel_format.bytes != 0) {
            ::logger.error("Failed to copy texture {} to buffer {}: row stride {} must be aligned to pixel size {}", src_texture, dst_buffer, fmt::styled_param(row_stride), fmt::styled_param(gl_pixel_format.bytes));
            return;
        }

        uint32 real_row_bytes = tinfo.width * gl_pixel_format.bytes;
        if (row_stride > 0 && row_stride < real_row_bytes) {
            ::logger.error("Failed to copy texture {} to buffer {}: row stride {} less than required row size {}", src_texture, dst_buffer, fmt::styled_param(row_stride), fmt::styled_param(real_row_bytes));
            return;
        }

        size_t stride_bytes = static_cast<size_t>(row_stride > 0 ? row_stride : real_row_bytes);
        size_t need_bytes = stride_bytes * tinfo.height * tinfo.depth - (stride_bytes - real_row_bytes);

        if (dst_offset + need_bytes > b->info.size) {
            ::logger.error("Failed to copy texture {} to buffer {}: buffer is too small. Required {} bytes, available {}", src_texture, dst_buffer, fmt::styled_param(dst_offset + need_bytes), fmt::styled_param(b->info.size));
            return;
        }

        GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, b->buffer_obj));
        GL_CALL(glBindTexture(tex->target, tex->texture_obj));

        auto row_length_in_pixels = static_cast<GLint>(row_stride / gl_pixel_format.bytes);
        GL_CALL(glPixelStorei(GL_PACK_ROW_LENGTH, row_length_in_pixels));
        GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 1));

        auto* gl_buffer_offset = reinterpret_cast<void*>(dst_offset);

        if (tinfo.type == texture_type::texture_2d) {
            GL_CALL(glGetTexImage(GL_TEXTURE_2D, 0, gl_pixel_format.format, gl_pixel_format.type, gl_buffer_offset));
        } else if (tinfo.type == texture_type::texture_3d) {
            GL_CALL(glGetTexImage(GL_TEXTURE_3D, 0, gl_pixel_format.format, gl_pixel_format.type, gl_buffer_offset));
        } else if (tinfo.type == texture_type::texture_cube) {
            static constexpr GLenum faces[6] = {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
            };

            GL_CALL(glGetTexImage(faces[layer_index % 6], 0, gl_pixel_format.format, gl_pixel_format.type, gl_buffer_offset));
        } else {
            TAV_UNREACHABLE();
        }

        GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 4));
        GL_CALL(glPixelStorei(GL_PACK_ROW_LENGTH, 0));

        GL_CALL(glBindTexture(tex->target, 0));
        GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
    }


} // namespace tavros::renderer::rhi
