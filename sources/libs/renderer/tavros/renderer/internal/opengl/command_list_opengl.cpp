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
    }

    command_list_opengl::~command_list_opengl()
    {
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

    void command_list_opengl::bind_framebuffer(framebuffer_handle pipeline)
    {
        if (auto* fb = m_device->get_resources()->framebuffers.try_get(pipeline.id)) {
            if (fb->is_default) {
                TAV_ASSERT(fb->framebuffer_obj == 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer_obj);
            }
        } else {
            ::logger.error("Can't bind the pipeline with id `%u`", pipeline.id);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

} // namespace tavros::renderer
