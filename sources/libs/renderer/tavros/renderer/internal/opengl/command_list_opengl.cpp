#include <tavros/renderer/internal/opengl/command_list_opengl.hpp>

#include <tavros/renderer/internal/opengl/type_conversions.hpp>
#include <tavros/core/prelude.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>

#include <glad/glad.h>

using namespace tavros::renderer::rhi;

namespace
{
    tavros::core::logger logger("command_list_opengl");
} // namespace

namespace tavros::renderer::rhi
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
        auto* p = m_device->get_resources()->try_get(pipeline);
        if (!p) {
            ::logger.error("Failed to bind pipeline `%s`: not found", htos(pipeline));
            glUseProgram(0);
            return;
        }

        m_current_pipeline = pipeline;
        auto& info = p->info;

        auto* pass = m_device->get_resources()->try_get(m_current_render_pass);
        if (!pass) {
            ::logger.error("Failed to bind pipeline `%s`: render pass `%s` not found", htos(pipeline), htos(m_current_render_pass));
            glUseProgram(0);
            return;
        }

        auto& pass_info = pass->info;
        if (pass_info.color_attachments.size() != info.blend_states.size()) {
            ::logger.error("Failed to bind pipeline `%s`: mismatch between color attachments in current render pass (%llu) and blend states in pipeline (%llu)", htos(pipeline), pass_info.color_attachments.size(), info.blend_states.size());
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
                    glBlendFuncSeparate(src_color_factor, dst_color_factor, src_alpha_factor, dst_alpha_factor);
                    glBlendEquationSeparate(color_blend_op, alpha_blend_op);
                }
                // Enable blending for i attachment
                glBlendFuncSeparatei(gl_attachment_index, src_color_factor, dst_color_factor, src_alpha_factor, dst_alpha_factor);
                glBlendEquationSeparatei(gl_attachment_index, color_blend_op, alpha_blend_op);

                need_enable_blending = true;
            } else {
                // Disable blending for i attachment
                glBlendFunci(gl_attachment_index, GL_ONE, GL_ZERO);
                glBlendEquationi(gl_attachment_index, GL_FUNC_ADD);
            }

            // Enable color mask
            auto r_color_enabled = blend_state.mask.has_flag(color_mask::red) ? GL_TRUE : GL_FALSE;
            auto g_color_enabled = blend_state.mask.has_flag(color_mask::green) ? GL_TRUE : GL_FALSE;
            auto b_color_enabled = blend_state.mask.has_flag(color_mask::blue) ? GL_TRUE : GL_FALSE;
            auto a_color_enabled = blend_state.mask.has_flag(color_mask::alpha) ? GL_TRUE : GL_FALSE;
            if (gl_attachment_index == 0) {
                // Also enable for default framebuffer
                glColorMask(r_color_enabled, g_color_enabled, b_color_enabled, a_color_enabled);
            }
            glColorMaski(gl_attachment_index, r_color_enabled, g_color_enabled, b_color_enabled, a_color_enabled);
        }

        if (need_enable_blending) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }

        // depth test
        if (info.depth_stencil.depth_test_enable) {
            glEnable(GL_DEPTH_TEST);

            // depth write
            glDepthMask(info.depth_stencil.depth_write_enable ? GL_TRUE : GL_FALSE);

            // depth compare func
            glDepthFunc(to_gl_compare_func(info.depth_stencil.depth_compare));
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        // stencil test
        if (info.depth_stencil.stencil_test_enable) {
            glEnable(GL_STENCIL_TEST);

            // stencil front
            glStencilFuncSeparate(
                GL_FRONT,
                to_gl_compare_func(info.depth_stencil.stencil_front.compare),
                info.depth_stencil.stencil_front.reference_value,
                info.depth_stencil.stencil_front.read_mask
            );
            glStencilOpSeparate(
                GL_FRONT,
                to_gl_stencil_op(info.depth_stencil.stencil_front.stencil_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_front.depth_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_front.pass_op)
            );
            glStencilMaskSeparate(GL_FRONT, info.depth_stencil.stencil_front.write_mask);

            // stencil back
            glStencilFuncSeparate(
                GL_BACK,
                to_gl_compare_func(info.depth_stencil.stencil_back.compare),
                info.depth_stencil.stencil_back.reference_value,
                info.depth_stencil.stencil_back.read_mask
            );
            glStencilOpSeparate(
                GL_BACK,
                to_gl_stencil_op(info.depth_stencil.stencil_back.stencil_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_back.depth_fail_op),
                to_gl_stencil_op(info.depth_stencil.stencil_back.pass_op)
            );
            glStencilMaskSeparate(GL_BACK, info.depth_stencil.stencil_back.write_mask);
        } else {
            glDisable(GL_STENCIL_TEST);
        }

        // rasterizer state
        // cull face
        if (info.rasterizer.cull == cull_face::off) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(to_gl_cull_face(info.rasterizer.cull));
        }

        // front face
        glFrontFace(to_gl_face(info.rasterizer.face));

        // polygon mode
        glPolygonMode(GL_FRONT_AND_BACK, to_gl_polygon_mode(info.rasterizer.polygon));

        // depth clamp
        if (info.rasterizer.depth_clamp_enable) {
            glEnable(GL_DEPTH_CLAMP);
            glDepthRange(info.rasterizer.depth_clamp_near, info.rasterizer.depth_clamp_far);
        } else {
            glDisable(GL_DEPTH_CLAMP);
        }

        // depth bias
        if (info.rasterizer.depth_bias_enable) {
            glEnable(to_gl_polygon_offset(info.rasterizer.polygon));
            glPolygonOffset(info.rasterizer.depth_bias_factor, info.rasterizer.depth_bias);
        } else {
            glDisable(to_gl_polygon_offset(info.rasterizer.polygon));
        }

        // multisample state
        if (info.multisample.sample_shading_enabled) {
            glEnable(GL_SAMPLE_SHADING);
            glMinSampleShading(info.multisample.min_sample_shading);
        } else {
            glDisable(GL_SAMPLE_SHADING);
        }

        glUseProgram(p->program_obj);
    }

    void command_list_opengl::bind_geometry(geometry_handle geometry)
    {
        if (auto* g = m_device->get_resources()->try_get(geometry)) {
            glBindVertexArray(g->vao_obj);
            m_current_geometry = geometry;
        } else {
            ::logger.error("Failed to bind geometry binding `%s`: not found", htos(geometry));
            glBindVertexArray(0);
            m_current_geometry = geometry_handle::invalid();
        }
    }

    void command_list_opengl::bind_shader_binding(shader_binding_handle shader_binding)
    {
        auto* sb = m_device->get_resources()->try_get(shader_binding);
        if (!sb) {
            ::logger.error("Failed to bind shader binding `%s`: shader binding not found", htos(shader_binding));
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
                    ::logger.error("Failed to bind shader binding `%s`: texture `%s` is not sampled", htos(shader_binding), htos(tex_h));
                    return;
                }
                glActiveTexture(GL_TEXTURE0 + binding.binding);
                glBindTexture(t->target, t->texture_obj);
            } else {
                ::logger.error("Failed to bind shader binding `%s`: texture `%s` not found", htos(shader_binding), htos(tex_h));
                return;
            }

            // Bind sampler
            auto sampler_h = sb->samplers[binding.sampler_index];
            if (auto* s = m_device->get_resources()->try_get(sampler_h)) {
                glBindSampler(binding.binding, s->sampler_obj);
            } else {
                ::logger.error("Failed to bind shader binding `%s`: sampler `%s` not found", htos(shader_binding), htos(sampler_h));
                return;
            }
        }

        // Bind buffers (UBO buffers)
        for (uint32 i = 0; i < info.buffer_bindings.size(); ++i) {
            auto& binding = info.buffer_bindings[i];

            auto buf_h = sb->buffers[binding.buffer_index];
            if (auto* b = m_device->get_resources()->try_get(buf_h)) {
                TAV_ASSERT(b->gl_target == GL_UNIFORM_BUFFER);

                if (binding.size == 0) {
                    // Bind the entire buffer
                    glBindBufferBase(b->gl_target, binding.binding, b->buffer_obj);
                } else {
                    // Bind a range of the buffer (subbuffer)
                    glBindBufferRange(
                        b->gl_target,
                        binding.binding,
                        b->buffer_obj,
                        static_cast<GLintptr>(binding.offset),
                        static_cast<GLsizeiptr>(binding.size)
                    );
                }
            } else {
                ::logger.error("Failed to bind shader binding `%s`: buffer `%s` not found", htos(shader_binding), htos(buf_h));
                return;
            }
        }
    }

    void command_list_opengl::begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer)
    {
        if (m_current_render_pass != render_pass_handle::invalid() || m_current_framebuffer != framebuffer_handle::invalid()) {
            ::logger.error("Failed to begin render pass `%s`: previous render pass `%s` not ended", htos(render_pass), htos(m_current_render_pass));
            return;
        }

        gl_render_pass* rp = m_device->get_resources()->try_get(render_pass);
        if (!rp) {
            ::logger.error("Failed to begin render pass `%s`: render pass not found", htos(render_pass));
            return;
        }

        gl_framebuffer* fb = m_device->get_resources()->try_get(framebuffer);
        if (fb == nullptr) {
            ::logger.error("Failed to begin render pass `%s`: framebuffer `%s` not found", htos(render_pass), htos(framebuffer));
            return;
        }

        // Validate color attachments size
        if (rp->info.color_attachments.size() != fb->info.color_attachment_formats.size()) {
            ::logger.error("Failed to begin render pass `%s`: number of color attachments does not match framebuffer `%s`", htos(render_pass), htos(framebuffer));
            return;
        }

        // Validate color attachments format
        for (uint32 i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto rp_color_format = rp->info.color_attachments[i].format;
            auto fb_color_format = fb->info.color_attachment_formats[i];
            if (rp_color_format != fb_color_format) {
                ::logger.error("Failed to begin render pass `%s`: color attachment format mismatch with framebuffer `%s`", htos(render_pass), htos(framebuffer));
                return;
            }
            if (rp->info.color_attachments[i].sample_count != fb->info.sample_count) {
                ::logger.error("Failed to begin render pass `%s`: color attachment sample count mismatch with framebuffer `%s`", htos(render_pass), htos(framebuffer));
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
                    ::logger.error("Failed to begin render pass `%s`: invalid resolve target attachment index", htos(render_pass));
                    return;
                }
                // Validate that the resolve target attachment format matches
                auto resolve_target_format = rp->info.color_attachments[resolve_attachment_index].format;
                if (attachment.format != resolve_target_format) {
                    ::logger.error("Failed to begin render pass `%s`: mismatched resolve target attachment format", htos(render_pass));
                    return;
                }
                // Validate that the resolve target attachment texture is single-sampled
                auto resolve_texture_h = rp->resolve_attachments[resolve_attachment_index];
                if (auto* tex = m_device->get_resources()->try_get(resolve_texture_h)) {
                    if (tex->info.sample_count != 1) {
                        ::logger.error("Failed to begin render pass `%s`: resolve target attachment texture '%s' must be single-sampled", htos(render_pass), htos(resolve_texture_h));
                        return;
                    }
                } else {
                    ::logger.error("Failed to begin render pass `%s`: resolve texture `%s` not found", htos(render_pass), htos(resolve_texture_h));
                    return;
                }
                // Validate that the source attachment texture is multi-sampled
                auto source_texture_h = fb->color_attachments[i];
                if (auto* tex = m_device->get_resources()->try_get(source_texture_h)) {
                    if (tex->info.sample_count == 1) {
                        ::logger.error("Failed to begin render pass `%s`: source attachment texture `%s` must be multi-sampled", htos(render_pass), htos(source_texture_h));
                        return;
                    }
                } else {
                    ::logger.error("Failed to begin render pass `%s`: source attachment texture `%s` not found", htos(render_pass), htos(source_texture_h));
                    return;
                }
            }
        }

        // Validate depth/stencil attachment format
        if (rp->info.depth_stencil_attachment.format != fb->info.depth_stencil_attachment_format) {
            ::logger.error("Failed to begin render pass `%s`: mismatched depth/stencil attachment format with framebuffer `%s`", htos(render_pass), htos(framebuffer));
            return;
        }

        // bind framebuffer
        if (fb->is_default) {
            TAV_ASSERT(fb->framebuffer_obj == 0);
            TAV_ASSERT(rp->info.color_attachments.size() == 1);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer_obj);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                ::logger.error("Failed to begin render pass `%s`: framebuffer `%s` is not complete", htos(render_pass), htos(framebuffer));
                return;
            }
        }

        // Set viewport
        glViewport(0, 0, fb->info.width, fb->info.height);

        // Allpy load operations to the color attachments and depth/stencil attachment
        // Only clear is supported, any other load operations are ignored
        if (fb->is_default) {
            GLbitfield clear_mask = 0;
            // Clear for color buffer
            if (rp->info.color_attachments[0].load == load_op::clear) {
                clear_mask |= GL_COLOR_BUFFER_BIT;
                auto color = rp->info.color_attachments[0].clear_value;
                glClearColor(color[0], color[1], color[2], color[3]);
            }

            if (rp->info.depth_stencil_attachment.format != pixel_format::none) {
                // Clear for depth buffer
                if (rp->info.depth_stencil_attachment.depth_load == load_op::clear) {
                    clear_mask |= GL_DEPTH_BUFFER_BIT;
                    glClearDepth(rp->info.depth_stencil_attachment.depth_clear_value);
                }
                // Clear for stencil buffer
                if (rp->info.depth_stencil_attachment.stencil_load == load_op::clear) {
                    clear_mask |= GL_STENCIL_BUFFER_BIT;
                    glClearStencil(rp->info.depth_stencil_attachment.stencil_clear_value);
                }
            }

            // Clear buffers if needed
            if (clear_mask) {
                glClear(clear_mask);
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
                        glClearBufferfv(GL_COLOR, attachment_index, rp_color_attachment.clear_value);
                    }
                } else {
                    ::logger.error("Failed to begin render pass `%s`: attachment texture `%s` not found", htos(render_pass), htos(attachment_h));
                    return;
                }
            }

            // Apply load operations to depth/stencil attachment
            if (rp->info.depth_stencil_attachment.format != pixel_format::none) {
                auto& rp_depth_stencil_attachment = rp->info.depth_stencil_attachment;

                // Apply load operation to depth component
                if (rp_depth_stencil_attachment.depth_load == load_op::clear) {
                    glClearBufferfv(GL_DEPTH, 0, &rp_depth_stencil_attachment.depth_clear_value);
                }

                // Apply load operation to stencil component
                if (rp_depth_stencil_attachment.stencil_load == load_op::clear) {
                    glClearBufferiv(GL_STENCIL, 0, &rp_depth_stencil_attachment.stencil_clear_value);
                }
            }
        }

        m_current_framebuffer = framebuffer;
        m_current_render_pass = render_pass;
    }

    void command_list_opengl::end_render_pass()
    {
        if (m_current_render_pass == render_pass_handle::invalid() || m_current_framebuffer == framebuffer_handle::invalid()) {
            ::logger.error("Failed to end render pass: render pass not started");
            return;
        }

        auto* rp = m_device->get_resources()->try_get(m_current_render_pass);
        if (!rp) {
            ::logger.error("Failed to end render pass `%s`: render pass not found", htos(m_current_render_pass));
            return;
        }

        auto* fb = m_device->get_resources()->try_get(m_current_framebuffer);
        if (fb == nullptr) {
            ::logger.error("Failed to end render pass `%s`: framebuffer `%s` not found", htos(m_current_render_pass), htos(m_current_framebuffer));
            return;
        }

        if (fb->is_default) {
            TAV_ASSERT(rp->info.color_attachments[0].store != store_op::resolve);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_current_framebuffer = framebuffer_handle::invalid();
            m_current_render_pass = render_pass_handle::invalid();
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
                rp->resolve_attachments.size();

                if (rp->resolve_attachments.size() > resolve_texture_index) {
                    // Find the resolve texture and validate it
                    auto  resolve_attachment_h = rp->resolve_attachments[resolve_texture_index];
                    auto* resolve_tex = m_device->get_resources()->try_get(resolve_attachment_h);
                    if (resolve_tex == nullptr) {
                        ::logger.error("Failed to end render pass `%s`: resolve attachment texture `%u` not found", htos(m_current_render_pass), resolve_texture_index);
                        return;
                    }

                    // everything is ok, add to the list
                    blit_data.push_back({GL_COLOR_ATTACHMENT0 + attachment_index, source_tex, resolve_tex});
                    attachment_index++;
                } else {
                    ::logger.error("Failed to end render pass `%s`: invalid resolve attachment index `%u`", htos(m_current_render_pass), resolve_texture_index);
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
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolve_fbo);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->framebuffer_obj);

            // Check if need to resolve for any attachment
            if (blit_data.size() > 0) {
                for (uint32 i = 0; i < blit_data.size(); ++i) {
                    TAV_ASSERT(blit_data[i].destination->target == GL_TEXTURE_2D);
                    glFramebufferTexture2D(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        blit_data[i].destination->target,
                        blit_data[i].destination->texture_obj,
                        0
                    );
                }

                // Resolve color attachments
                for (uint32 i = 0; i < blit_data.size(); ++i) {
                    glReadBuffer(blit_data[i].attachment);
                    glDrawBuffer(GL_COLOR_ATTACHMENT0);
                    glBlitFramebuffer(
                        0, 0, fb->info.width, fb->info.height,
                        0, 0, fb->info.width, fb->info.height,
                        GL_COLOR_BUFFER_BIT,
                        GL_NEAREST
                    );
                }
            }

            // Check if need to resolve depth/stencil attachment
            if (depth_stencil_blit_mask) {
                // Resolve depth/stencil attachment
                glBlitFramebuffer(
                    0, 0, fb->info.width, fb->info.height,
                    0, 0, fb->info.width, fb->info.height,
                    depth_stencil_blit_mask,
                    GL_NEAREST // For depth/stencil, filter must be GL_NEAREST
                );
            }


            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_current_framebuffer = framebuffer_handle::invalid();
        m_current_render_pass = render_pass_handle::invalid();
        glBindVertexArray(0);
        m_current_geometry = geometry_handle::invalid();
    }

    void command_list_opengl::draw(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance)
    {
        auto* p = m_device->get_resources()->try_get(m_current_pipeline);
        if (!p) {
            if (m_current_pipeline == pipeline_handle::invalid()) {
                ::logger.error("Failed to draw: no pipeline is bound");
            } else {
                ::logger.error("Failed to draw: pipeline `%s` not found", htos(m_current_pipeline));
            }
            return;
        }

        auto gl_topology = to_gl_topology(p->info.topology);
        auto gl_first_vertex = static_cast<GLint>(first_vertex);
        auto gl_vertex_count = static_cast<GLsizei>(vertex_count);
        auto gl_instance_count = static_cast<GLsizei>(instance_count);
        auto gl_first_instance = static_cast<GLuint>(first_instance);

        if (instance_count > 1 || first_instance > 0) {
            glDrawArraysInstancedBaseInstance(
                gl_topology,
                gl_first_vertex,
                gl_vertex_count,
                gl_instance_count,
                gl_first_instance
            );
        } else if (instance_count == 1) {
            glDrawArrays(
                gl_topology,
                gl_first_vertex,
                gl_vertex_count
            );
        } else {
            ::logger.warning("Failed to draw: instance count `%u` must be at least 1", instance_count);
        }
    }

    void command_list_opengl::draw_indexed(uint32 index_count, uint32 first_index, uint32 vertex_offset, uint32 instance_count, uint32 first_instance)
    {
        auto* p = m_device->get_resources()->try_get(m_current_pipeline);
        if (!p) {
            if (m_current_pipeline == pipeline_handle::invalid()) {
                ::logger.error("Failed to draw indexed: no pipeline is bound");
            } else {
                ::logger.error("Failed to draw indexed: pipeline `%s` not found", htos(m_current_pipeline));
            }
            return;
        }

        auto* g = m_device->get_resources()->try_get(m_current_geometry);
        if (!g) {
            if (m_current_geometry == geometry_handle::invalid()) {
                ::logger.error("Failed to draw indexed: no geometry binding is bound");
            } else {
                ::logger.error("Failed to draw indexed: geometry binding `%s` not found", htos(m_current_geometry));
            }
            return;
        }

        if (!g->info.has_index_buffer) {
            ::logger.error("Failed to draw indexed: current geometry binding `%s` has no index buffer", htos(m_current_geometry));
            return;
        }

        auto  gl_index_format = to_gl_index_format(g->info.index_format);
        auto  gl_topology = to_gl_topology(p->info.topology);
        auto* gl_index_offset = reinterpret_cast<const void*>(first_index * gl_index_format.size);
        auto  gl_index_count = static_cast<GLsizei>(index_count);
        auto  gl_instance_count = static_cast<GLsizei>(instance_count);
        auto  gl_first_instance = static_cast<GLuint>(first_instance);

        if (instance_count > 1 || first_instance > 0) {
            glDrawElementsInstancedBaseInstance(
                gl_topology,
                gl_index_count,
                gl_index_format.type,
                gl_index_offset,
                gl_instance_count,
                gl_first_instance
            );
        } else if (instance_count == 1) {
            glDrawElements(
                gl_topology,
                gl_index_count,
                gl_index_format.type,
                gl_index_offset
            );
        } else {
            ::logger.warning("Failed to draw indexed: instance count `%u` must be at least 1", instance_count);
        }
    }

    void command_list_opengl::copy_buffer_data(buffer_handle buffer, const void* data, size_t size, size_t offset)
    {
        TAV_ASSERT(data != nullptr);

        auto* b = m_device->get_resources()->try_get(buffer);
        if (!b) {
            ::logger.error("Failed to copy data to buffer `%s`: buffer not found", htos(buffer));
            return;
        }

        if (offset + size > b->info.size) {
            ::logger.error("Failed to copy data to buffer `%s`: offset %llu + size %llu exceeds buffer size %llu", htos(buffer), offset, size, b->info.size);
            return;
        }

        if (b->info.access != buffer_access::cpu_to_gpu) {
            ::logger.error("Failed to copy data to buffer `%s`: buffer is not CPU-to-GPU", htos(buffer));
            return;
        }

        TAV_ASSERT(b->gl_target == GL_COPY_WRITE_BUFFER);

        auto target = b->gl_target;

        // Get buffer range
        glBindBuffer(target, b->buffer_obj);

        void* ptr = glMapBufferRange(
            target,
            static_cast<GLintptr>(offset),
            static_cast<GLsizeiptr>(size),
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT
        );

        // Write data
        memcpy(ptr, data, size);

        glUnmapBuffer(target);

        glBindBuffer(target, 0);
    }

    void command_list_opengl::copy_buffer(buffer_handle src_buffer, buffer_handle dst_buffer, size_t size, size_t src_offset, size_t dst_offset)
    {
        // Get dst and src buffers
        auto* src = m_device->get_resources()->try_get(src_buffer);
        if (!src) {
            ::logger.error("Failed to copy buffer `%s`: source buffer not found", htos(src_buffer));
            return;
        }

        auto* dst = m_device->get_resources()->try_get(dst_buffer);
        if (!dst) {
            ::logger.error("Failed to copy buffer `%s`: destination buffer not found", htos(dst_buffer));
            return;
        }

        // Check memory region
        if (dst_offset + size > dst->info.size) {
            ::logger.error(
                "Failed to copy buffer `%s`: destination buffer overflowed, offset %llu + size %llu exceeds buffer size %llu",
                htos(dst_buffer), dst_offset, size, dst->info.size
            );
            return;
        }

        if (src_offset + size > src->info.size) {
            ::logger.error(
                "Failed to copy buffer `%s`: source buffer overflowed, offset %llu + size %llu exceeds buffer size %llu",
                htos(src_buffer), src_offset, size, src->info.size
            );
            return;
        }

        // Check access
        // Allowed: cpu_to_gpu -> gpu_only
        // Allowed: gpu_only   -> gpu_only
        auto dst_access = dst->info.access;
        auto src_access = src->info.access;

        if (dst_access != buffer_access::gpu_only) {
            ::logger.error(
                "Failed to copy buffer `%s`: destination buffer has invalid access `%s`",
                htos(dst_buffer), to_string(dst_access).data()
            );
            return;
        }

        if (src_access != buffer_access::cpu_to_gpu && src_access != buffer_access::gpu_only) {
            ::logger.error(
                "Failed to copy buffer `%s`: source buffer has invalid access `%s`",
                htos(src_buffer), to_string(src_access).data()
            );
            return;
        }

        // Make copy data
        glBindBuffer(GL_COPY_READ_BUFFER, src->buffer_obj);
        glBindBuffer(GL_COPY_WRITE_BUFFER, dst->buffer_obj);

        glCopyBufferSubData(
            GL_COPY_READ_BUFFER,
            GL_COPY_WRITE_BUFFER,
            static_cast<GLintptr>(src_offset),
            static_cast<GLintptr>(dst_offset),
            static_cast<GLsizeiptr>(size)
        );

        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

    void command_list_opengl::copy_buffer_to_texture(buffer_handle src_buffer, texture_handle dst_texture, uint32 layer_index, size_t size, size_t src_offset, uint32 row_stride)
    {
        auto* b = m_device->get_resources()->try_get(src_buffer);
        if (!b) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: source buffer not found", htos(src_buffer), htos(dst_texture));
            return;
        }

        auto* tex = m_device->get_resources()->try_get(dst_texture);
        if (!tex) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: destination buffer not found", htos(src_buffer), htos(dst_texture));
            return;
        }

        auto& tinfo = tex->info;

        if (b->info.usage != buffer_usage::stage) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: invalid source buffer usage type, expected `stage` got `%s`", htos(src_buffer), htos(dst_texture), to_string(b->info.usage).data());
            return;
        }

        if (b->info.access != buffer_access::cpu_to_gpu) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: invalid source buffer access type, expected `cpu_to_gpu` got `%s`", htos(src_buffer), htos(dst_texture), to_string(b->info.access).data());
            return;
        }

        if (tinfo.sample_count != 1) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: destination texture has invalid sample count `%u`, only 1 is supported", htos(src_buffer), htos(dst_texture), tex->info.sample_count);
            return;
        }

        if (!tinfo.usage.has_flag(texture_usage::transfer_destination)) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: destination texture does not have `transfer_destination` usage flag", htos(src_buffer), htos(dst_texture));
            return;
        }

        if (!is_color_format(tinfo.format)) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: destination texture format is not a color format", htos(src_buffer), htos(dst_texture));
            return;
        }

        auto gl_pixel_format = to_gl_pixel_format(tinfo.format);
        if (row_stride % gl_pixel_format.bytes != 0) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: row stride `%u` must be aligned to pixel size `%u`", htos(src_buffer), htos(dst_texture), row_stride, gl_pixel_format.bytes);
            return;
        }

        uint32 real_row_bytes = tinfo.width * gl_pixel_format.bytes;
        if (row_stride > 0 && row_stride < real_row_bytes) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: row stride `%u` less than required row size `%u`", htos(src_buffer), htos(dst_texture), row_stride, real_row_bytes);
            return;
        }

        size_t stride_bytes = static_cast<size_t>(row_stride > 0 ? row_stride : real_row_bytes);
        size_t need_bytes = stride_bytes * tinfo.height * tinfo.depth - (stride_bytes - real_row_bytes);

        if (src_offset + need_bytes > b->info.size) {
            ::logger.error("Failed to copy buffer `%s` to texture `%s`: buffer is too small. Required %llu bytes, available %llu bytes", htos(src_buffer), htos(dst_texture), src_offset + need_bytes, b->info.size);
            return;
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, b->buffer_obj);
        glBindTexture(tex->target, tex->texture_obj);

        auto row_length_in_pixels = static_cast<GLint>(row_stride / gl_pixel_format.bytes);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_in_pixels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        auto* gl_buffer_offset = reinterpret_cast<const void*>(src_offset);

        if (tinfo.type == texture_type::texture_2d) {
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,    // Mip level
                0, 0, // xoffset, yoffset
                tinfo.width, tinfo.height,
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            );
        } else if (tinfo.type == texture_type::texture_3d) {
            glTexSubImage3D(
                GL_TEXTURE_3D,
                0,       // mip level
                0, 0, 0, // xoffset, yoffset, zoffset
                tinfo.width, tinfo.height, tinfo.depth,
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            );
        } else if (tinfo.type == texture_type::texture_cube) {
            static constexpr GLenum faces[6] = {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
            };

            glTexSubImage2D(
                faces[layer_index % 6],
                0,    // mip level
                0, 0, // xoffset, yoffset
                tinfo.width, tinfo.height,
                gl_pixel_format.format,
                gl_pixel_format.type,
                gl_buffer_offset
            );
        } else {
            TAV_UNREACHABLE();
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        if (tinfo.mip_levels > 1) {
            // This call gets warning for texture_cube:
            // warning [OpenGL_debug] Pixel-path performance warning: Pixel transfer is synchronized with 3D rendering
            // TODO: fix it
            glGenerateMipmap(tex->target);
        }

        glBindTexture(tex->target, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

} // namespace tavros::renderer::rhi
