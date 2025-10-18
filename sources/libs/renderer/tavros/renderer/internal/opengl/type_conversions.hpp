#pragma once

#include <tavros/renderer/rhi/sampler_create_info.hpp>
#include <tavros/renderer/rhi/shader_create_info.hpp>
#include <tavros/renderer/rhi/pipeline_create_info.hpp>
#include <tavros/renderer/rhi/geometry_create_info.hpp>
#include <tavros/renderer/rhi/handle.hpp>

#include <glad/glad.h>

namespace tavros::renderer::rhi
{

    struct gl_pixel_format
    {
        GLint  internal_format;
        GLenum format; // data format
        GLenum type;   // data type
        GLint  bytes;  // number of bytes per pixel
    };

    struct gl_wnd_fb_info
    {
        bool   supported;
        uint32 color_bits;
        uint32 depth_bits;
        uint32 stencil_bits;
    };

    struct gl_filter
    {
        GLenum min_filter;
        GLenum mag_filter;
    };

    struct gl_depth_stencil_format
    {
        bool   is_depth_stencil_format;
        GLenum depth_stencil_attachment_type;
    };

    struct gl_vertex_format
    {
        GLenum type;
        uint32 size;
    };

    struct gl_index_format
    {
        GLenum type;
        uint32 size;
    };

    struct gl_attribute_info
    {
        GLenum type; // GL base type (e.g., GL_FLOAT, GL_INT)
        GLint  size; // Size of one component in bytes
        GLint  rows; // Number of scalars per column (vector size)
        GLint  cols; // Number of columns (for matrices)
    };

    struct gl_rhi_type_info
    {
        bool                      valid = false;
        tavros::core::string_view gl_typename;
        attribute_type            type;
        attribute_format          format;
    };


    gl_pixel_format to_gl_pixel_format(pixel_format format);

    gl_wnd_fb_info to_gl_wnd_fb_info(pixel_format format);

    gl_filter to_gl_filter(sampler_filter filter);

    bool is_color_format(pixel_format format);

    gl_depth_stencil_format to_depth_stencil_fromat(pixel_format format);

    GLenum to_gl_wrap_mode(wrap_mode mode);

    GLenum to_gl_compare_func(compare_op func);

    GLenum to_gl_shader_stage(shader_stage type);

    GLenum to_gl_stencil_op(stencil_op op);

    gl_vertex_format to_gl_vertex_format(attribute_format format);

    gl_index_format to_gl_index_format(index_buffer_format format);

    GLenum to_gl_topology(primitive_topology topology);

    gl_attribute_info to_gl_attribute_info(attribute_type type, attribute_format format);

    gl_rhi_type_info gl_type_to_rhi_type(GLenum type);

    GLenum to_gl_blend_factor(blend_factor factor);

    GLenum to_gl_blend_op(blend_op op);

    GLenum to_gl_cull_face(cull_face cull);

    GLenum to_gl_face(front_face face);

    GLenum to_gl_polygon_mode(polygon_mode mode);

    GLenum to_gl_polygon_offset(polygon_mode mode);

} // namespace tavros::renderer::rhi
