#include <tavros/renderer/internal/opengl/command_list_opengl.hpp>

#include <tavros/core/prelude.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>

#include <glad/glad.h>

using namespace tavros::renderer::rhi;

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

    struct gl_index_format
    {
        GLenum type;
        uint32 size;
    };

    gl_index_format to_gl_index_format(index_buffer_format format)
    {
        switch (format) {
        case index_buffer_format::u16:
            return {GL_UNSIGNED_SHORT, 2};
        case index_buffer_format::u32:
            return {GL_UNSIGNED_INT, 4};
        default:
            TAV_UNREACHABLE();
        }
    }

    GLenum to_gl_topology(primitive_topology topology)
    {
        switch (topology) {
        case tavros::renderer::rhi::primitive_topology::points:
            return GL_POINTS;
        case tavros::renderer::rhi::primitive_topology::lines:
            return GL_LINES;
        case tavros::renderer::rhi::primitive_topology::line_strip:
            return GL_LINE_STRIP;
        case tavros::renderer::rhi::primitive_topology::triangles:
            return GL_TRIANGLES;
        case tavros::renderer::rhi::primitive_topology::triangle_strip:
            return GL_TRIANGLE_STRIP;
        case tavros::renderer::rhi::primitive_topology::triangle_fan:
            return GL_TRIANGLE_FAN;
        default:
            TAV_UNREACHABLE();
        }
    }

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
        m_current_pipeline = pipeline;
        if (auto* p = m_device->get_resources()->pipelines.try_get(pipeline.id)) {
            auto& desc = p->info;

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
            m_current_geometry_binding = geometry_binding;
        } else {
            ::logger.error("Can't bind the geometry binding with id `%u`", geometry_binding.id);
            glBindVertexArray(0);
            m_current_geometry_binding = {0};
        }
    }

    void command_list_opengl::bind_shader_binding(shader_binding_handle shader_binding)
    {
        if (auto* sb = m_device->get_resources()->shader_bindings.try_get(shader_binding.id)) {
            auto& info = sb->info;

            // Bind textures and samplers
            for (uint32 i = 0; i < info.texture_bindings.size(); ++i) {
                auto& binding = info.texture_bindings[i];

                // Bind texture
                auto tex_id = sb->textures[binding.texture_index].id;
                if (auto* t = m_device->get_resources()->textures.try_get(tex_id)) {
                    if (!t->info.usage.has_flag(texture_usage::sampled)) {
                        ::logger.error("Can't bind not sampled texture with id `%u`", tex_id);
                        continue;
                    }
                    glActiveTexture(GL_TEXTURE0 + binding.binding);
                    glBindTexture(t->target, t->texture_obj);
                } else {
                    ::logger.error("Can't bind the texture with id `%u`", tex_id);
                }

                // Bind sampler
                auto sampler_id = sb->samplers[binding.sampler_index].id;
                if (auto* s = m_device->get_resources()->samplers.try_get(sampler_id)) {
                    glBindSampler(binding.binding, s->sampler_obj);
                } else {
                    ::logger.error("Can't bind the sampler with id `%u`", sampler_id);
                }
            }

            // Bind buffers (UBO buffers)
            for (uint32 i = 0; i < info.buffer_bindings.size(); ++i) {
                auto& binding = info.buffer_bindings[i];

                auto buf_id = sb->buffers[binding.buffer_index].id;
                if (auto* b = m_device->get_resources()->buffers.try_get(buf_id)) {
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
                    ::logger.error("Can't bind the buffer with id `%u`", buf_id);
                }
            }
        } else {
            ::logger.error("Can't bind the shader binding with id `%u` because shader binding is not found", shader_binding.id);
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
        if (rp->info.color_attachments.size() != fb->info.color_attachment_formats.size()) {
            ::logger.error("Mismatched number of color attachments for render pass with id `%u` and framebuffer with id `%u`", render_pass.id, framebuffer.id);
            return;
        }

        // Validate color attachments format
        for (uint32 i = 0; i < rp->info.color_attachments.size(); ++i) {
            auto rp_color_format = rp->info.color_attachments[i].format;
            auto fb_color_format = fb->info.color_attachment_formats[i];
            if (rp_color_format != fb_color_format) {
                ::logger.error("Mismatched color attachment format for render pass with id `%u` and framebuffer with id `%u`", render_pass.id, framebuffer.id);
                return;
            }
            if (rp->info.color_attachments[i].sample_count != fb->info.sample_count) {
                ::logger.error("Mismatched color attachment sample count for render pass with id `%u` and framebuffer with id `%u`", render_pass.id, framebuffer.id);
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
                    ::logger.error("Invalid resolve target attachment index for render pass with id `%u`", render_pass.id);
                    return;
                }
                // Validate that the resolve target attachment format matches
                auto resolve_target_format = rp->info.color_attachments[resolve_attachment_index].format;
                if (attachment.format != resolve_target_format) {
                    ::logger.error("Mismatched resolve target attachment format for render pass with id `%u`", render_pass.id);
                    return;
                }
                // Validate that the resolve target attachment texture is single-sampled
                auto resolve_texture_handle = rp->resolve_attachments[resolve_attachment_index];
                if (auto* tex = m_device->get_resources()->textures.try_get(resolve_texture_handle.id)) {
                    if (tex->info.sample_count != 1) {
                        ::logger.error("Resolve target attachment texture must be single-sampled for render pass with id `%u`", render_pass.id);
                        return;
                    }
                } else {
                    ::logger.error("Can't find the texture with id `%u`", resolve_texture_handle.id);
                    return;
                }
                // Validate that the source attachment texture is multi-sampled
                auto source_texture_handle = fb->color_attachments[i];
                if (auto* tex = m_device->get_resources()->textures.try_get(source_texture_handle.id)) {
                    if (tex->info.sample_count == 1) {
                        ::logger.error("Source attachment texture must be multi-sampled for render pass with id `%u`", render_pass.id);
                        return;
                    }
                } else {
                    ::logger.error("Can't find the texture with id `%u`", source_texture_handle.id);
                    return;
                }
            }
        }

        // Validate depth/stencil attachment format
        if (rp->info.depth_stencil_attachment.format != fb->info.depth_stencil_attachment_format) {
            ::logger.error("Mismatched depth/stencil attachment format for render pass with id `%u` and framebuffer with id `%u`", render_pass.id, framebuffer.id);
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
                ::logger.error("Framebuffer is not complete");
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
                auto attachment_handle = fb->color_attachments[i];
                if (auto* tex = m_device->get_resources()->textures.try_get(attachment_handle.id)) {
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
                    ::logger.error("Can't find the texture with id `%u`", attachment_handle.id);
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

        if (fb->is_default) {
            TAV_ASSERT(rp->info.color_attachments[0].store != store_op::resolve);
        } else {
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
                auto* source_tex = m_device->get_resources()->textures.try_get(fb->color_attachments[i].id);
                TAV_ASSERT(source_tex);

                if (rp_color_attachment.store == store_op::resolve) {
                    // Resolve attachments
                    auto resolve_texture_index = rp_color_attachment.resolve_texture_index;
                    rp->resolve_attachments.size();

                    if (rp->resolve_attachments.size() > resolve_texture_index) {
                        // Find the resolve texture and validate it
                        auto* resolve_tex = m_device->get_resources()->textures.try_get(rp->resolve_attachments[resolve_texture_index].id);
                        if (resolve_tex == nullptr) {
                            ::logger.error("Invalid resolve attachment index `%u`", resolve_texture_index);
                            return;
                        }

                        // everything is ok, add to the list
                        blit_data.push_back({GL_COLOR_ATTACHMENT0 + attachment_index, source_tex, resolve_tex});
                        attachment_index++;
                    } else {
                        ::logger.error("Invalid resolve attachment index `%u`", resolve_texture_index);
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
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_current_framebuffer = {0};
        m_current_render_pass = {0};
    }

    void command_list_opengl::draw(uint32 count, uint32 first_vertex)
    {
        auto* p = m_device->get_resources()->pipelines.try_get(m_current_pipeline.id);
        if (!p) {
            ::logger.error("Can't draw indexsed because no pipeline is bound");
            return;
        }

        auto topology = to_gl_topology(p->info.topology);
        glDrawArrays(topology, static_cast<GLint>(first_vertex), static_cast<GLsizei>(count));
    }

    void command_list_opengl::draw_indexed(uint32 index_count, uint32 first_index, uint32 vertex_offset, uint32 instance_count, uint32 first_instance)
    {
        if (auto* gb = m_device->get_resources()->geometry_bindings.try_get(m_current_geometry_binding.id)) {
            if (!gb->info.has_index_buffer) {
                ::logger.error("Can't draw indexsed because current geometry binding doesn't have index buffer");
                return;
            }

            auto* p = m_device->get_resources()->pipelines.try_get(m_current_pipeline.id);
            if (!p) {
                ::logger.error("Can't draw indexsed because no pipeline is bound");
                return;
            }


            auto        index_format = to_gl_index_format(gb->info.index_format);
            auto        topology = to_gl_topology(p->info.topology);
            const void* index_offset = reinterpret_cast<const void*>(first_index * index_format.size);

            if (instance_count > 1) {
                glDrawElementsInstanced(
                    topology,
                    index_count,
                    index_format.type,
                    index_offset,
                    instance_count
                );
            } else {
                glDrawElements(
                    topology,
                    index_count,
                    index_format.type,
                    index_offset
                );
            }
        } else {
            ::logger.error("Can't find the geometry binding with id `%u`", m_current_geometry_binding.id);
        }
    }

    void command_list_opengl::copy_buffer_data(buffer_handle buffer, const void* data, size_t size, size_t offset)
    {
        TAV_ASSERT(data != nullptr);

        if (auto* b = m_device->get_resources()->buffers.try_get(buffer.id)) {
            if (offset + size > b->info.size) {
                ::logger.error("Can't copy data to buffer with id `%u` because the size is out of range", buffer.id);
                return;
            }
            if (b->info.access != buffer_access::cpu_to_gpu) {
                ::logger.error("Can't copy data to buffer with id `%u` because the buffer is not cpu_to_gpu", buffer.id);
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
        } else {
            ::logger.error("Can't copy buffer data because buffer with id `%u` not found", buffer.id);
        }
    }

    void command_list_opengl::copy_buffer(buffer_handle dst_buffer, buffer_handle src_buffer, size_t size, size_t dst_offset, size_t src_offset)
    {
        // Get dst and src buffers
        auto* dst = m_device->get_resources()->buffers.try_get(dst_buffer.id);
        if (!dst) {
            ::logger.error("Cannot copy buffer: destination buffer with id `%u` not found", dst_buffer.id);
            return;
        }

        auto* src = m_device->get_resources()->buffers.try_get(src_buffer.id);
        if (!src) {
            ::logger.error("Cannot copy buffer: source buffer with id `%u` not found", src_buffer.id);
            return;
        }

        // Check memory region
        if (dst_offset + size > dst->info.size) {
            ::logger.error(
                "Cannot copy buffer: destination buffer with id `%u` overflow (offset %llu + size %llu > buffer size %llu)",
                dst_buffer.id, dst_offset, size, dst->info.size
            );
            return;
        }

        if (src_offset + size > src->info.size) {
            ::logger.error(
                "Cannot copy buffer: source buffer with id `%u` overflow (offset %llu + size %llu > buffer size %llu)",
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
                "Cannot copy buffer: destination buffer with id `%u` has invalid access `%s`",
                dst_buffer.id, to_string(dst_access).data()
            );
            return;
        }

        if (src_access != buffer_access::cpu_to_gpu && src_access != buffer_access::gpu_only) {
            ::logger.error(
                "Cannot copy buffer: source buffer with id `%u` has invalid access `%s`",
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

} // namespace tavros::renderer::rhi
