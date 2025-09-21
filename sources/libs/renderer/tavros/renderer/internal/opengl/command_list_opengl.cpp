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
            ::logger.error("Failed to bind pipeline `%u`: not found", pipeline.id);
            glUseProgram(0);
            return;
        }

        m_current_pipeline = pipeline;
        auto& info = p->info;

        glUseProgram(p->program_obj);

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
            glCullFace(info.rasterizer.cull == cull_face::front ? GL_FRONT : GL_BACK);
        }

        // front face
        glFrontFace(info.rasterizer.face == front_face::clockwise ? GL_CW : GL_CCW);

        // polygon mode
        if (info.rasterizer.polygon == polygon_mode::lines) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else if (info.rasterizer.polygon == polygon_mode::points) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        } else if (info.rasterizer.polygon == polygon_mode::fill) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            TAV_UNREACHABLE();
        }

        // depth clamp
        if (info.rasterizer.depth_clamp_enable) {
            glEnable(GL_DEPTH_CLAMP);
        } else {
            glDisable(GL_DEPTH_CLAMP);
        }

        // depth bias
        if (info.rasterizer.depth_bias_enable) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(info.rasterizer.depth_bias_slope, info.rasterizer.depth_bias);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        // multisample state
        // uint8 sample_count = 1; cant be initialized here
        if (info.multisample.sample_shading_enabled) {
            glEnable(GL_SAMPLE_SHADING);
            glMinSampleShading(info.multisample.min_sample_shading);
        } else {
            glDisable(GL_SAMPLE_SHADING);
        }
    }

    void command_list_opengl::bind_geometry(geometry_handle geometry)
    {
        if (auto* g = m_device->get_resources()->try_get(geometry)) {
            glBindVertexArray(g->vao_obj);
            m_current_geometry = geometry;
        } else {
            ::logger.error("Failed to bind geometry binding `%u`: not found", geometry.id);
            glBindVertexArray(0);
            m_current_geometry = {0};
        }
    }

    void command_list_opengl::bind_shader_binding(shader_binding_handle shader_binding)
    {
        auto* sb = m_device->get_resources()->try_get(shader_binding);
        if (!sb) {
            ::logger.error("Failed to bind shader binding `%u`: shader binding not found", shader_binding.id);
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
                    ::logger.error("Failed to bind shader binding `%u`: texture `%u` is not sampled", shader_binding.id, tex_h.id);
                    return;
                }
                glActiveTexture(GL_TEXTURE0 + binding.binding);
                glBindTexture(t->target, t->texture_obj);
            } else {
                ::logger.error("Failed to bind shader binding `%u`: texture `%u` not found", shader_binding.id, tex_h.id);
                return;
            }

            // Bind sampler
            auto sampler_h = sb->samplers[binding.sampler_index];
            if (auto* s = m_device->get_resources()->try_get(sampler_h)) {
                glBindSampler(binding.binding, s->sampler_obj);
            } else {
                ::logger.error("Failed to bind shader binding `%u`: sampler `%u` not found", shader_binding.id, sampler_h.id);
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
                ::logger.error("Failed to bind shader binding `%u`: buffer `%u` not found", shader_binding.id, buf_h.id);
                return;
            }
        }
    }

    void command_list_opengl::begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer)
    {
        if (m_current_render_pass.id != 0 || m_current_framebuffer.id != 0) {
            ::logger.error("Failed to begin render pass `%u`: previous render pass `%u` not ended", render_pass.id, m_current_render_pass.id);
            return;
        }

        gl_render_pass* rp = m_device->get_resources()->try_get(render_pass);
        if (!rp) {
            ::logger.error("Failed to begin render pass `%u`: render pass not found", render_pass.id);
            return;
        }

        gl_framebuffer* fb = m_device->get_resources()->try_get(framebuffer);
        if (fb == nullptr) {
            ::logger.error("Failed to begin render pass `%u`: framebuffer `%u` not found", render_pass.id, framebuffer.id);
            return;
        }

        // Validate color attachments size
        if (rp->info.color_attachments.size() != fb->info.color_attachment_formats.size()) {
            ::logger.error("Failed to begin render pass `%u`: number of color attachments does not match framebuffer `%u`", render_pass.id, framebuffer.id);
            return;
        }

        // Validate color attachments format
        for (uint32 i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto rp_color_format = rp->info.color_attachments[i].format;
            auto fb_color_format = fb->info.color_attachment_formats[i];
            if (rp_color_format != fb_color_format) {
                ::logger.error("Failed to begin render pass `%u`: color attachment format mismatch with framebuffer `%u`", render_pass.id, framebuffer.id);
                return;
            }
            if (rp->info.color_attachments[i].sample_count != fb->info.sample_count) {
                ::logger.error("Failed to begin render pass `%u`: color attachment sample count mismatch with framebuffer `%u`", render_pass.id, framebuffer.id);
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
                    ::logger.error("Failed to begin render pass `%u`: invalid resolve target attachment index", render_pass.id);
                    return;
                }
                // Validate that the resolve target attachment format matches
                auto resolve_target_format = rp->info.color_attachments[resolve_attachment_index].format;
                if (attachment.format != resolve_target_format) {
                    ::logger.error("Failed to begin render pass `%u`: mismatched resolve target attachment format", render_pass.id);
                    return;
                }
                // Validate that the resolve target attachment texture is single-sampled
                auto resolve_texture_h = rp->resolve_attachments[resolve_attachment_index];
                if (auto* tex = m_device->get_resources()->try_get(resolve_texture_h)) {
                    if (tex->info.sample_count != 1) {
                        ::logger.error("Failed to begin render pass `%u`: resolve target attachment texture '%u' must be single-sampled", render_pass.id, resolve_texture_h.id);
                        return;
                    }
                } else {
                    ::logger.error("Failed to begin render pass `%u`: resolve texture `%u` not found", render_pass.id, resolve_texture_h.id);
                    return;
                }
                // Validate that the source attachment texture is multi-sampled
                auto source_texture_h = fb->color_attachments[i];
                if (auto* tex = m_device->get_resources()->try_get(source_texture_h)) {
                    if (tex->info.sample_count == 1) {
                        ::logger.error("Failed to begin render pass `%u`: source attachment texture `%u` must be multi-sampled", render_pass.id, source_texture_h.id);
                        return;
                    }
                } else {
                    ::logger.error("Failed to begin render pass `%u`: source attachment texture `%u` not found", render_pass.id, source_texture_h.id);
                    return;
                }
            }
        }

        // Validate depth/stencil attachment format
        if (rp->info.depth_stencil_attachment.format != fb->info.depth_stencil_attachment_format) {
            ::logger.error("Failed to begin render pass `%u`: mismatched depth/stencil attachment format with framebuffer `%u`", render_pass.id, framebuffer.id);
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
                ::logger.error("Failed to begin render pass `%u`: framebuffer `%u` is not complete", render_pass.id, framebuffer.id);
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
            uint32 attachment_index = 0;
            for (uint32 i = 0; i < fb->color_attachments.size(); ++i) {
                auto attachment_h = fb->color_attachments[i];
                if (auto* tex = m_device->get_resources()->try_get(attachment_h)) {
                    // If texture has the same sample count with framebuffer, then this texture is attachment texture
                    if (tex->info.sample_count == fb->info.sample_count) {
                        auto& rp_color_attachment = rp->info.color_attachments[i];

                        // Apply load operation (only clear)
                        // Any other load operation doesn't need to be applied
                        if (load_op::clear == rp_color_attachment.load) {
                            glClearBufferfv(GL_COLOR, attachment_index, rp_color_attachment.clear_value);
                        }

                        attachment_index++;
                    }
                } else {
                    ::logger.error("Failed to begin render pass `%u`: attachment texture `%u` not found", render_pass.id, attachment_h.id);
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
        if (m_current_render_pass.id == 0 || m_current_framebuffer.id == 0) {
            ::logger.error("Failed to end render pass: render pass not started");
            return;
        }

        auto* rp = m_device->get_resources()->try_get(m_current_render_pass);
        if (!rp) {
            ::logger.error("Failed to end render pass `%u`: render pass not found", m_current_render_pass.id);
            return;
        }

        auto* fb = m_device->get_resources()->try_get(m_current_framebuffer);
        if (fb == nullptr) {
            ::logger.error("Failed to end render pass `%u`: framebuffer `%u` not found", m_current_render_pass.id, m_current_framebuffer.id);
            return;
        }

        if (fb->is_default) {
            TAV_ASSERT(rp->info.color_attachments[0].store != store_op::resolve);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_current_framebuffer = {0};
            m_current_render_pass = {0};
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
                        ::logger.error("Failed to end render pass `%u`: resolve attachment texture `%u` not found", m_current_render_pass.id, resolve_texture_index);
                        return;
                    }

                    // everything is ok, add to the list
                    blit_data.push_back({GL_COLOR_ATTACHMENT0 + attachment_index, source_tex, resolve_tex});
                    attachment_index++;
                } else {
                    ::logger.error("Failed to end render pass `%u`: invalid resolve attachment index `%u`", m_current_render_pass.id, resolve_texture_index);
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
        m_current_framebuffer = {0};
        m_current_render_pass = {0};
    }

    void command_list_opengl::draw(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance)
    {
        auto* p = m_device->get_resources()->try_get(m_current_pipeline);
        if (!p) {
            if (m_current_pipeline.id == 0) {
                ::logger.error("Failed to draw: no pipeline is bound");
            } else {
                ::logger.error("Failed to draw: pipeline `%u` not found", m_current_pipeline.id);
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
            if (m_current_pipeline.id == 0) {
                ::logger.error("Failed to draw indexed: no pipeline is bound");
            } else {
                ::logger.error("Failed to draw indexed: pipeline `%u` not found", m_current_pipeline.id);
            }
            return;
        }

        auto* g = m_device->get_resources()->try_get(m_current_geometry);
        if (!g) {
            if (m_current_geometry.id == 0) {
                ::logger.error("Failed to draw indexed: no geometry binding is bound");
            } else {
                ::logger.error("Failed to draw indexed: geometry binding `%u` not found", m_current_geometry.id);
            }
            return;
        }

        if (!g->info.has_index_buffer) {
            ::logger.error("Failed to draw indexed: current geometry binding `%u` has no index buffer", m_current_geometry.id);
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
            ::logger.error("Failed to copy data to buffer `%u`: buffer not found", buffer.id);
            return;
        }

        if (offset + size > b->info.size) {
            ::logger.error("Failed to copy data to buffer `%u`: offset %llu + size %llu exceeds buffer size %llu", buffer.id, offset, size, b->info.size);
            return;
        }

        if (b->info.access != buffer_access::cpu_to_gpu) {
            ::logger.error("Failed to copy data to buffer `%u`: buffer is not CPU-to-GPU", buffer.id);
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
            ::logger.error("Failed to copy buffer `%u`: source buffer not found", src_buffer.id);
            return;
        }

        auto* dst = m_device->get_resources()->try_get(dst_buffer);
        if (!dst) {
            ::logger.error("Failed to copy buffer `%u`: destination buffer not found", dst_buffer.id);
            return;
        }

        // Check memory region
        if (dst_offset + size > dst->info.size) {
            ::logger.error(
                "Failed to copy buffer `%u`: destination buffer overflowed, offset %llu + size %llu exceeds buffer size %llu",
                dst_buffer.id, dst_offset, size, dst->info.size
            );
            return;
        }

        if (src_offset + size > src->info.size) {
            ::logger.error(
                "Failed to copy buffer `%u`: source buffer overflowed, offset %llu + size %llu exceeds buffer size %llu",
                src_buffer.id, src_offset, size, src->info.size
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
                "Failed to copy buffer `%u`: destination buffer has invalid access `%s`",
                dst_buffer.id, to_string(dst_access).data()
            );
            return;
        }

        if (src_access != buffer_access::cpu_to_gpu && src_access != buffer_access::gpu_only) {
            ::logger.error(
                "Failed to copy buffer `%u`: source buffer has invalid access `%s`",
                src_buffer.id, to_string(src_access).data()
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

    void command_list_opengl::copy_buffer_to_texture(buffer_handle src_buffer, texture_handle dst_texture, size_t size, size_t src_offset, uint32 stride)
    {
        auto* b = m_device->get_resources()->try_get(src_buffer);
        if (!b) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: source buffer not found", src_buffer.id, dst_texture.id);
            return;
        }

        auto* tex = m_device->get_resources()->try_get(dst_texture);
        if (!tex) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: destination buffer not found", src_buffer.id, dst_texture.id);
            return;
        }

        if (b->info.usage != buffer_usage::stage) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: invalid source buffer usage type, expected `stage` got `%s`", src_buffer.id, dst_texture.id, to_string(b->info.usage).data());
            return;
        }

        if (b->info.access != buffer_access::cpu_to_gpu) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: invalid source buffer access type, expected `cpu_to_gpu` got `%s`", src_buffer.id, dst_texture.id, to_string(b->info.access).data());
            return;
        }

        if (tex->info.sample_count != 1) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: destination texture has invalid sample count `%u`, only 1 is supported", src_buffer.id, dst_texture.id, tex->info.sample_count);
            return;
        }

        if (tex->info.usage != 1) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: destination texture has invalid sample count `%u`, only 1 is supported", src_buffer.id, dst_texture.id, tex->info.sample_count);
            return;
        }

        if (!tex->info.usage.has_flag(texture_usage::transfer_destination)) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: destination texture does not have `transfer_destination` usage flag", src_buffer.id, dst_texture.id);
            return;
        }

        if (!is_color_format(tex->info.format)) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: destination texture format is not a color format", src_buffer.id, dst_texture.id);
            return;
        }

        auto gl_pixel_format = to_gl_pixel_format(tex->info.format);
        auto need_copy_size = static_cast<size_t>(stride > 0 ? stride * tex->info.height : tex->info.width * tex->info.height * gl_pixel_format.bytes);

        if (src_offset + need_copy_size > b->info.size) {
            ::logger.error("Failed to copy buffer `%u` to texture `%u`: buffer is too small. Required %llu bytes, available %llu bytes", src_buffer.id, dst_texture.id, src_offset + need_copy_size, b->info.size);
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, b->buffer_obj);
        glBindTexture(tex->target, tex->texture_obj);

        auto row_length_in_pixels = static_cast<GLint>(stride / gl_pixel_format.bytes);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_in_pixels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        auto* gl_offset = reinterpret_cast<const void*>(src_offset);

        glTexSubImage2D(
            tex->target,
            0,    // Mip level
            0, 0, // xoffset, yoffset
            tex->info.width, tex->info.height,
            gl_pixel_format.format,
            gl_pixel_format.type,
            gl_offset
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        if (tex->info.mip_levels > 1) {
            glGenerateMipmap(tex->target);
        }

        glBindTexture(tex->target, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

} // namespace tavros::renderer::rhi
