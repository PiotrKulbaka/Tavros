#include <tavros/renderer/internal/opengl/type_conversions.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace tavros::renderer::rhi
{

    gl_pixel_format to_gl_pixel_format(pixel_format format)
    {
        switch (format) {
        case pixel_format::r8un:
            return {GL_R8, GL_RED, GL_UNSIGNED_BYTE, 1};
        case pixel_format::r16un:
            return {GL_R16, GL_RED, GL_UNSIGNED_SHORT, 2};
        case pixel_format::rg8un:
            return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE, 2};
        case pixel_format::rg16un:
            return {GL_RG16, GL_RG, GL_UNSIGNED_SHORT, 4};
        case pixel_format::rgb8un:
            return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, 3};
        case pixel_format::rgb16un:
            return {GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT, 6};
        case pixel_format::rgba8un:
            return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4};
        case pixel_format::rgba16un:
            return {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT, 8};

        case pixel_format::r8in:
            return {GL_R8_SNORM, GL_RED, GL_BYTE, 1};
        case pixel_format::r16in:
            return {GL_R16_SNORM, GL_RED, GL_SHORT, 2};
        case pixel_format::rg8in:
            return {GL_RG8_SNORM, GL_RG, GL_BYTE, 2};
        case pixel_format::rg16in:
            return {GL_RG16_SNORM, GL_RG, GL_SHORT, 4};
        case pixel_format::rgb8in:
            return {GL_RGB8_SNORM, GL_RGB, GL_BYTE, 3};
        case pixel_format::rgb16in:
            return {GL_RGB16_SNORM, GL_RGB, GL_SHORT, 6};
        case pixel_format::rgba8in:
            return {GL_RGBA8_SNORM, GL_RGBA, GL_BYTE, 4};
        case pixel_format::rgba16in:
            return {GL_RGBA16_SNORM, GL_RGBA, GL_SHORT, 8};

        case pixel_format::r8u:
            return {GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 1};
        case pixel_format::r16u:
            return {GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 2};
        case pixel_format::r32u:
            return {GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, 4};
        case pixel_format::rg8u:
            return {GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE, 2};
        case pixel_format::rg16u:
            return {GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT, 4};
        case pixel_format::rg32u:
            return {GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, 8};
        case pixel_format::rgb8u:
            return {GL_RGB8UI, GL_RGB_INTEGER, GL_UNSIGNED_BYTE, 3};
        case pixel_format::rgb16u:
            return {GL_RGB16UI, GL_RGB_INTEGER, GL_UNSIGNED_SHORT, 6};
        case pixel_format::rgb32u:
            return {GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT, 12};
        case pixel_format::rgba8u:
            return {GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, 4};
        case pixel_format::rgba16u:
            return {GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, 8};
        case pixel_format::rgba32u:
            return {GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 16};

        case pixel_format::r8i:
            return {GL_R8I, GL_RED_INTEGER, GL_BYTE, 1};
        case pixel_format::r16i:
            return {GL_R16I, GL_RED_INTEGER, GL_SHORT, 2};
        case pixel_format::r32i:
            return {GL_R32I, GL_RED_INTEGER, GL_INT, 4};
        case pixel_format::rg8i:
            return {GL_RG8I, GL_RG_INTEGER, GL_BYTE, 2};
        case pixel_format::rg16i:
            return {GL_RG16I, GL_RG_INTEGER, GL_SHORT, 4};
        case pixel_format::rg32i:
            return {GL_RG32I, GL_RG_INTEGER, GL_INT, 8};
        case pixel_format::rgb8i:
            return {GL_RGB8I, GL_RGB_INTEGER, GL_BYTE, 3};
        case pixel_format::rgb16i:
            return {GL_RGB16I, GL_RGB_INTEGER, GL_SHORT, 6};
        case pixel_format::rgb32i:
            return {GL_RGB32I, GL_RGB_INTEGER, GL_INT, 12};
        case pixel_format::rgba8i:
            return {GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE, 4};
        case pixel_format::rgba16i:
            return {GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT, 8};
        case pixel_format::rgba32i:
            return {GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, 16};

        case pixel_format::r16f:
            return {GL_R16F, GL_RED, GL_HALF_FLOAT, 2};
        case pixel_format::r32f:
            return {GL_R32F, GL_RED, GL_FLOAT, 4};
        case pixel_format::rg16f:
            return {GL_RG16F, GL_RG, GL_HALF_FLOAT, 4};
        case pixel_format::rg32f:
            return {GL_RG32F, GL_RG, GL_FLOAT, 8};
        case pixel_format::rgb16f:
            return {GL_RGB16F, GL_RGB, GL_HALF_FLOAT, 6};
        case pixel_format::rgb32f:
            return {GL_RGB32F, GL_RGB, GL_FLOAT, 12};
        case pixel_format::rgba16f:
            return {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, 8};
        case pixel_format::rgba32f:
            return {GL_RGBA32F, GL_RGBA, GL_FLOAT, 16};

        case pixel_format::depth16:
            return {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 2};
        case pixel_format::depth24:
            return {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 4};
        case pixel_format::depth32f:
            return {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 4};
        case pixel_format::stencil8:
            return {GL_STENCIL_INDEX8, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, 1};
        case pixel_format::depth24_stencil8:
            return {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 4};               // packed
        case pixel_format::depth32f_stencil8:
            return {GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 5}; // 4+1 bytes

        default:
            TAV_UNREACHABLE();
        }
    }

    gl_filter to_gl_filter(sampler_filter filter)
    {
        gl_filter result = {};

        switch (filter.mag_filter) {
        case filter_mode::nearest:
            result.mag_filter = GL_NEAREST;
            break;
        case filter_mode::linear:
            result.mag_filter = GL_LINEAR;
            break;
        default:
            TAV_UNREACHABLE();
        }

        switch (filter.min_filter) {
        case filter_mode::linear:
            switch (filter.mipmap_filter) {
            case mipmap_filter_mode::off:
                result.min_filter = GL_LINEAR;
                break;
            case mipmap_filter_mode::nearest:
                result.min_filter = GL_LINEAR_MIPMAP_NEAREST;
                break;
            case mipmap_filter_mode::linear:
                result.min_filter = GL_LINEAR_MIPMAP_LINEAR;
                break;
            default:
                TAV_UNREACHABLE();
            }
            break;

        case filter_mode::nearest:
            switch (filter.mipmap_filter) {
            case mipmap_filter_mode::off:
                result.min_filter = GL_NEAREST;
                break;
            case mipmap_filter_mode::nearest:
                result.min_filter = GL_NEAREST_MIPMAP_NEAREST;
                break;
            case mipmap_filter_mode::linear:
                result.min_filter = GL_NEAREST_MIPMAP_LINEAR;
                break;
            default:
                TAV_UNREACHABLE();
            }
            break;
        }

        return result;
    }

    bool is_color_format(pixel_format format)
    {
        switch (format) {
        case pixel_format::r8un:
            return true;
        case pixel_format::r16un:
            return true;
        case pixel_format::rg8un:
            return true;
        case pixel_format::rg16un:
            return true;
        case pixel_format::rgb8un:
            return true;
        case pixel_format::rgb16un:
            return true;
        case pixel_format::rgba8un:
            return true;
        case pixel_format::rgba16un:
            return true;

        case pixel_format::r8in:
            return true;
        case pixel_format::r16in:
            return true;
        case pixel_format::rg8in:
            return true;
        case pixel_format::rg16in:
            return true;
        case pixel_format::rgb8in:
            return true;
        case pixel_format::rgb16in:
            return true;
        case pixel_format::rgba8in:
            return true;
        case pixel_format::rgba16in:
            return true;

        case pixel_format::r8u:
            return true;
        case pixel_format::r16u:
            return true;
        case pixel_format::r32u:
            return true;
        case pixel_format::rg8u:
            return true;
        case pixel_format::rg16u:
            return true;
        case pixel_format::rg32u:
            return true;
        case pixel_format::rgb8u:
            return true;
        case pixel_format::rgb16u:
            return true;
        case pixel_format::rgb32u:
            return true;
        case pixel_format::rgba8u:
            return true;
        case pixel_format::rgba16u:
            return true;
        case pixel_format::rgba32u:
            return true;

        case pixel_format::r8i:
            return true;
        case pixel_format::r16i:
            return true;
        case pixel_format::r32i:
            return true;
        case pixel_format::rg8i:
            return true;
        case pixel_format::rg16i:
            return true;
        case pixel_format::rg32i:
            return true;
        case pixel_format::rgb8i:
            return true;
        case pixel_format::rgb16i:
            return true;
        case pixel_format::rgb32i:
            return true;
        case pixel_format::rgba8i:
            return true;
        case pixel_format::rgba16i:
            return true;
        case pixel_format::rgba32i:
            return true;

        case pixel_format::r16f:
            return true;
        case pixel_format::r32f:
            return true;
        case pixel_format::rg16f:
            return true;
        case pixel_format::rg32f:
            return true;
        case pixel_format::rgb16f:
            return true;
        case pixel_format::rgb32f:
            return true;
        case pixel_format::rgba16f:
            return true;
        case pixel_format::rgba32f:
            return true;
        default:
            return false;
        }
    }

    gl_depth_stencil_format to_depth_stencil_fromat(pixel_format format)
    {
        switch (format) {
        case pixel_format::depth16:
            return {true, GL_DEPTH_ATTACHMENT};
        case pixel_format::depth24:
            return {true, GL_DEPTH_ATTACHMENT};
        case pixel_format::depth32f:
            return {true, GL_DEPTH_ATTACHMENT};
        case pixel_format::stencil8:
            return {true, GL_STENCIL_ATTACHMENT};
        case pixel_format::depth24_stencil8:
            return {true, GL_DEPTH_STENCIL_ATTACHMENT};
        case pixel_format::depth32f_stencil8:
            return {true, GL_DEPTH_STENCIL_ATTACHMENT};

        default:
            return {false, 0};
        }
    }

    GLenum to_gl_wrap_mode(wrap_mode mode)
    {
        switch (mode) {
        case wrap_mode::repeat:
            return GL_REPEAT;
        case wrap_mode::mirrored_repeat:
            return GL_MIRRORED_REPEAT;
        case wrap_mode::clamp_to_edge:
            return GL_CLAMP_TO_EDGE;
        case wrap_mode::clamp_to_border:
            return GL_CLAMP_TO_BORDER;
        default:
            TAV_UNREACHABLE();
        }
    }

    GLenum to_gl_compare_func(compare_op func)
    {
        switch (func) {
        case compare_op::off:
            return GL_NEVER;
        case compare_op::less:
            return GL_LESS;
        case compare_op::equal:
            return GL_EQUAL;
        case compare_op::less_equal:
            return GL_LEQUAL;
        case compare_op::greater:
            return GL_GREATER;
        case compare_op::not_equal:
            return GL_NOTEQUAL;
        case compare_op::greater_equal:
            return GL_GEQUAL;
        case compare_op::always:
            return GL_ALWAYS;
        default:
            TAV_UNREACHABLE();
        }
    }

    GLenum to_gl_shader_stage(shader_stage type)
    {
        switch (type) {
        case shader_stage::vertex:
            return GL_VERTEX_SHADER;
        case shader_stage::fragment:
            return GL_FRAGMENT_SHADER;
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
        case attribute_format::f64:
            return {GL_DOUBLE, 8};
        default:
            TAV_UNREACHABLE();
        }
    }

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

    gl_attribute_info to_gl_attribute_info(attribute_type type, attribute_format format)
    {
        gl_attribute_info info;

        // Map format -> GL type + component size
        switch (format) {
        case attribute_format::u8:
            info.type = GL_UNSIGNED_BYTE;
            info.size = 1;
            break;
        case attribute_format::i8:
            info.type = GL_BYTE;
            info.size = 1;
            break;
        case attribute_format::u16:
            info.type = GL_UNSIGNED_SHORT;
            info.size = 2;
            break;
        case attribute_format::i16:
            info.type = GL_SHORT;
            info.size = 2;
            break;
        case attribute_format::u32:
            info.type = GL_UNSIGNED_INT;
            info.size = 4;
            break;
        case attribute_format::i32:
            info.type = GL_INT;
            info.size = 4;
            break;
        case attribute_format::f16:
            info.type = GL_HALF_FLOAT;
            info.size = 2;
            break;
        case attribute_format::f32:
            info.type = GL_FLOAT;
            info.size = 4;
            break;
        default:
            TAV_UNREACHABLE();
        }

        // Map type -> rows/cols
        switch (type) {
        case attribute_type::scalar:
            info.rows = 1;
            info.cols = 1;
            break;
        case attribute_type::vec2:
            info.rows = 2;
            info.cols = 1;
            break;
        case attribute_type::vec3:
            info.rows = 3;
            info.cols = 1;
            break;
        case attribute_type::vec4:
            info.rows = 4;
            info.cols = 1;
            break;

        case attribute_type::mat2:
            info.rows = 2;
            info.cols = 2;
            break; // 2x2
        case attribute_type::mat2x3:
            info.rows = 3;
            info.cols = 2;
            break; // 3 rows, 2 cols
        case attribute_type::mat2x4:
            info.rows = 4;
            info.cols = 2;
            break; // 4 rows, 2 cols
        case attribute_type::mat3x2:
            info.rows = 2;
            info.cols = 3;
            break; // 2 rows, 3 cols
        case attribute_type::mat3:
            info.rows = 3;
            info.cols = 3;
            break; // 3x3
        case attribute_type::mat3x4:
            info.rows = 4;
            info.cols = 3;
            break; // 4 rows, 3 cols
        case attribute_type::mat4x2:
            info.rows = 2;
            info.cols = 4;
            break; // 2 rows, 4 cols
        case attribute_type::mat4x3:
            info.rows = 3;
            info.cols = 4;
            break; // 3 rows, 4 cols
        case attribute_type::mat4:
            info.rows = 4;
            info.cols = 4;
            break; // 4x4

        default:
            TAV_UNREACHABLE();
        }

        return info;
    }

    gl_rhi_type_info gl_type_to_rhi_type(GLenum type)
    {
        switch (type) {
            // float
        case GL_FLOAT:
            return {true, "float", attribute_type::scalar, attribute_format::f32};
        case GL_FLOAT_VEC2:
            return {true, "vec2", attribute_type::vec2, attribute_format::f32};
        case GL_FLOAT_VEC3:
            return {true, "vec3", attribute_type::vec3, attribute_format::f32};
        case GL_FLOAT_VEC4:
            return {true, "vec4", attribute_type::vec4, attribute_format::f32};
        case GL_FLOAT_MAT2:
            return {true, "mat2", attribute_type::mat2, attribute_format::f32};
        case GL_FLOAT_MAT3:
            return {true, "mat3", attribute_type::mat3, attribute_format::f32};
        case GL_FLOAT_MAT4:
            return {true, "mat4", attribute_type::mat4, attribute_format::f32};
        case GL_FLOAT_MAT2x3:
            return {true, "mat2x3", attribute_type::mat2x3, attribute_format::f32};
        case GL_FLOAT_MAT2x4:
            return {true, "mat2x4", attribute_type::mat2x4, attribute_format::f32};
        case GL_FLOAT_MAT3x2:
            return {true, "mat3x2", attribute_type::mat3x2, attribute_format::f32};
        case GL_FLOAT_MAT3x4:
            return {true, "mat3x4", attribute_type::mat3x4, attribute_format::f32};
        case GL_FLOAT_MAT4x2:
            return {true, "mat4x2", attribute_type::mat4x2, attribute_format::f32};
        case GL_FLOAT_MAT4x3:
            return {true, "mat4x3", attribute_type::mat4x3, attribute_format::f32};

            // double
        case GL_DOUBLE:
            return {true, "double", attribute_type::scalar, attribute_format::f64};
        case GL_DOUBLE_VEC2:
            return {true, "dvec2", attribute_type::vec2, attribute_format::f64};
        case GL_DOUBLE_VEC3:
            return {true, "dvec3", attribute_type::vec3, attribute_format::f64};
        case GL_DOUBLE_VEC4:
            return {true, "dvec4", attribute_type::vec4, attribute_format::f64};
        case GL_DOUBLE_MAT2:
            return {true, "dmat2", attribute_type::mat2, attribute_format::f64};
        case GL_DOUBLE_MAT3:
            return {true, "dmat3", attribute_type::mat3, attribute_format::f64};
        case GL_DOUBLE_MAT4:
            return {true, "dmat4", attribute_type::mat4, attribute_format::f64};
        case GL_DOUBLE_MAT2x3:
            return {true, "dmat2x3", attribute_type::mat2x3, attribute_format::f64};
        case GL_DOUBLE_MAT2x4:
            return {true, "dmat2x4", attribute_type::mat2x4, attribute_format::f64};
        case GL_DOUBLE_MAT3x2:
            return {true, "dmat3x2", attribute_type::mat3x2, attribute_format::f64};
        case GL_DOUBLE_MAT3x4:
            return {true, "dmat3x4", attribute_type::mat3x4, attribute_format::f64};
        case GL_DOUBLE_MAT4x2:
            return {true, "dmat4x2", attribute_type::mat4x2, attribute_format::f64};
        case GL_DOUBLE_MAT4x3:
            return {true, "dmat4x3", attribute_type::mat4x3, attribute_format::f64};

            // int
        case GL_INT:
            return {true, "int", attribute_type::scalar, attribute_format::i32};
        case GL_INT_VEC2:
            return {true, "ivec2", attribute_type::vec2, attribute_format::i32};
        case GL_INT_VEC3:
            return {true, "ivec3", attribute_type::vec3, attribute_format::i32};
        case GL_INT_VEC4:
            return {true, "ivec4", attribute_type::vec4, attribute_format::i32};

            // unsigned int
        case GL_UNSIGNED_INT:
            return {true, "uint", attribute_type::scalar, attribute_format::u32};
        case GL_UNSIGNED_INT_VEC2:
            return {true, "uvec2", attribute_type::vec2, attribute_format::u32};
        case GL_UNSIGNED_INT_VEC3:
            return {true, "uvec3", attribute_type::vec3, attribute_format::u32};
        case GL_UNSIGNED_INT_VEC4:
            return {true, "uvec4", attribute_type::vec4, attribute_format::u32};

        default:
            return {false, "unknown", attribute_type::scalar, attribute_format::f32};
        }
    }

} // namespace tavros::renderer::rhi
