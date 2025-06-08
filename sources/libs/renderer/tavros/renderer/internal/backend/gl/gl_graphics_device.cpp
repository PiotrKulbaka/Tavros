#include <tavros/renderer/internal/backend/gl/gl_graphics_device.hpp>

#include <tavros/core/prelude.hpp>
#include <atomic>

#include <glad/glad.h>

using namespace tavros::renderer;

namespace
{
    tavros::core::logger logger("gl_graphics_device");

    struct gl_format
    {
        GLint  internal_format;
        GLenum format; // data format
        GLenum type;   // data type
        GLint  bytes;  // number of bytes per pixel
    };

    struct gl_filter
    {
        GLenum min_filter;
        GLenum mag_filter;
    };

    gl_format to_gl_internal_format(pixel_format format)
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

    GLuint compile_shader(tavros::core::string_view program, GLenum shader_type)
    {
        auto shader = glCreateShader(shader_type);
        if (shader == 0) {
            logger.error("glCreateShader() returns 0");
            return 0;
        }

        // compile shader
        const char* program_text = program.data();
        auto        text_length = static_cast<GLint>(program.size());
        glShaderSource(shader, 1, &program_text, &text_length);
        glCompileShader(shader);

        // check compile status
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char buffer[4096];
            glGetShaderInfoLog(shader, sizeof(buffer), nullptr, buffer);
            logger.error("glCompileShader() failure:\n%s", buffer);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint link_program(GLuint vert_shader, GLuint frag_shader)
    {
        auto failed = false;

        auto program = glCreateProgram();
        if (!program) {
            logger.error("glCreateProgram() failed");
            return 0;
        }

        glAttachShader(program, vert_shader);
        glAttachShader(program, frag_shader);
        glLinkProgram(program);

        // check link status
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            char buffer[4096];
            glGetProgramInfoLog(program, sizeof(buffer), nullptr, buffer);
            logger.error("glLinkProgram() failed:\n%s", buffer);
            failed = true;
        }

        // validate program
        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
        if (!status) {
            char buffer[4096];
            glGetProgramInfoLog(program, sizeof(buffer), nullptr, buffer);
            logger.error("glValidateProgram() failed:\n%s", buffer);
            failed = true;
        }

        glDetachShader(program, vert_shader);
        glDetachShader(program, frag_shader);

        if (failed) {
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }

    struct format_info
    {
        GLenum type;
        uint32 size;
    };

    format_info to_gl_type(attribute_format format)
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

    void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
    {
        tavros::core::logger l("OpenGL_debug");
        if (severity == GL_DEBUG_SEVERITY_HIGH) {
            l.error("%s", message);
        } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
            l.warning("%s", message);
        } else if (severity == GL_DEBUG_SEVERITY_LOW) {
            l.warning("%s", message);
        } else {
            l.debug("%s", message);
        }
    }

} // namespace

namespace tavros::renderer
{

    gl_graphics_device::gl_graphics_device()
    {
        ::logger.info("gl_graphics_device created");

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    gl_graphics_device::~gl_graphics_device()
    {
        ::logger.info("gl_graphics_device destroyed");
    }

    sampler_handle gl_graphics_device::create_sampler(const sampler_desc& desc)
    {
        GLuint sampler = 0;
        glGenSamplers(1, &sampler);

        auto filter = to_gl_filter(desc.filter);

        // Filtering
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, filter.min_filter);
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, filter.mag_filter);

        // Wrapping
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, to_gl_wrap_mode(desc.wrap_mode.wrap_s));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, to_gl_wrap_mode(desc.wrap_mode.wrap_t));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, to_gl_wrap_mode(desc.wrap_mode.wrap_r));

        // LOD parameters
        glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, desc.mip_lod_bias);
        glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, desc.min_lod);
        glSamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, desc.max_lod);

        // Depth compare
        if (desc.depth_compare != compare_op::off) {
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, to_gl_compare_func(desc.depth_compare));
        } else {
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }

        sampler_handle handle;
        handle.id = sampler;
        return handle;
    }

    void gl_graphics_device::destroy_sampler(sampler_handle handle)
    {
        glDeleteSamplers(1, &handle.id);
    }

    texture2d_handle gl_graphics_device::create_texture(const texture2d_desc& desc)
    {
        auto   is_multisample = desc.sample_count > 1;
        GLenum target = is_multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        GLuint texture;

        glGenTextures(1, &texture);
        glBindTexture(target, texture);

        auto f = to_gl_internal_format(desc.format);

        if (is_multisample) {
            glTexImage2DMultisample(
                target,
                desc.sample_count,
                f.internal_format,
                desc.width,
                desc.height,
                GL_TRUE
            );

            if (desc.mip_levels != 1) {
                ::logger.error("multisample textures can't have mipmaps");
                TAV_ASSERT(false);
            }

            if (desc.data != nullptr) {
                ::logger.error("multisample textures can't have initial data");
                TAV_ASSERT(false);
            }
        } else {
            if (desc.stride != 0) {
                // Set the row length to match the stride
                uint32 row_length_in_pixels = desc.stride / f.bytes;
                glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_in_pixels);
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(
                target,
                0, // mip level
                f.internal_format,
                desc.width,
                desc.height,
                0, // border
                f.format,
                f.type,
                desc.data
            );

            // Set the default alignment
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

            // Generate mipmaps
            if (desc.mip_levels > 1 && desc.data != nullptr) {
                glGenerateMipmap(target);
            }
        }

        glBindTexture(target, 0);

        texture2d_handle handle;
        handle.id = texture;
        return handle;
    }

    void gl_graphics_device::destroy_texture(texture2d_handle texture)
    {
        glDeleteTextures(1, &texture.id);
    }

    pipeline_handle gl_graphics_device::create_pipeline(const pipeline_desc& desc)
    {
        gl_pipeline pipeline;
        pipeline.desc = desc;

        static std::atomic<uint32> id = 1;

        // create shader program
        auto v = compile_shader(desc.shaders.vertex_source, GL_VERTEX_SHADER);
        auto f = compile_shader(desc.shaders.fragment_source, GL_FRAGMENT_SHADER);
        auto p = link_program(v, f);
        glDeleteShader(v);
        glDeleteShader(f);
        pipeline.program = p;
        ::logger.info("Shader successfully created");

        m_pipelines[id] = pipeline;

        pipeline_handle handle;
        handle.id = id;
        id++;
        return handle;
    }

    void gl_graphics_device::destroy_pipeline(pipeline_handle pipeline)
    {
        auto id = pipeline.id;
        if (auto it = m_pipelines.find(id); it != m_pipelines.end()) {
            if (it->second.program != 0) {
                glDeleteProgram(it->second.program);
            }
            m_pipelines.erase(it);
        } else {
            ::logger.error("Can't find pipeline with id %d", id);
        }
    }

} // namespace tavros::renderer

