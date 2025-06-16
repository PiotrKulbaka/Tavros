#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

#include <tavros/renderer/internal/opengl/swapchain_opengl.hpp>
#include <tavros/core/scoped_owner.hpp>
#include <tavros/core/prelude.hpp>


using namespace tavros::renderer;

namespace
{
    tavros::core::logger logger("graphics_device_opengl");

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

    bool is_color_fromat(pixel_format format)
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

    struct gl_depth_stencil_format
    {
        bool   is_depth_stencil_format;
        GLenum depth_stencil_attachment_type;
    };

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

    format_info to_gl_attribute_format(attribute_format format)
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
        } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            l.info("%s", message);
        } else {
            l.debug("Unknown severity: %s", message);
        }
    }

    void init_gl_debug()
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    template<class Pool, class Del>
    void destroy_for(Pool& pool, Del deleter)
    {
        tavros::core::vector<uint32> handles;
        handles.reserve(pool.size());
        for (auto& it : pool) {
            handles.push_back(it.first);
        }

        for (auto handle : handles) {
            deleter({handle});
        }
    }

} // namespace

namespace tavros::renderer
{

    graphics_device_opengl::graphics_device_opengl()
    {
        ::logger.info("graphics_device_opengl created");
    }

    graphics_device_opengl::~graphics_device_opengl()
    {
        destroy();
        ::logger.info("graphics_device_opengl destroyed");
    }

    void graphics_device_opengl::destroy()
    {
        destroy_for(m_resources.samplers, [this](sampler_handle h) {
            destroy_sampler(h);
        });

        destroy_for(m_resources.textures, [this](texture_handle h) {
            destroy_texture(h);
        });

        destroy_for(m_resources.pipelines, [this](pipeline_handle h) {
            destroy_pipeline(h);
        });

        destroy_for(m_resources.framebuffers, [this](framebuffer_handle h) {
            destroy_framebuffer(h);
        });

        destroy_for(m_resources.buffers, [this](buffer_handle h) {
            destroy_buffer(h);
        });

        destroy_for(m_resources.geometry_bindings, [this](geometry_binding_handle h) {
            destroy_geometry(h);
        });

        // Should be removed in last turn because swapchain owns the OpenGL context
        destroy_for(m_resources.swapchains, [this](swapchain_handle h) {
            destroy_swapchain(h);
        });
    }

    swapchain_handle graphics_device_opengl::create_swapchain(const swapchain_desc& desc, void* native_handle)
    {
        // Check if swapchain with native handle already created
        for (auto& sc : m_resources.swapchains) {
            if (native_handle == sc.second.native_handle) {
                ::logger.error("Swapchain with native handle %p already created", native_handle);
                return {0};
            }
        }

        auto sc = create_swapchain_opengl(this, desc, native_handle);

        if (!sc) {
            ::logger.error("Swapchain creation failed");
            return {0};
        }

        // Initialize debug callback for OpenGL here, because it's not possible to do it in the constructor
        // the first call of create_swapchain_opengl() will create the context
        init_gl_debug();

        swapchain_handle handle = {m_resources.swapchains.insert({desc, sc, native_handle})};
        ::logger.debug("Swapchain with id %u created", handle.id);
        return handle;
    }

    void graphics_device_opengl::destroy_swapchain(swapchain_handle swapchain)
    {
        if (auto* desc = m_resources.swapchains.try_get(swapchain.id)) {
            m_resources.swapchains.remove(swapchain.id);
            ::logger.debug("Swapchain with id %u destroyed", swapchain.id);
        } else {
            ::logger.error("Can't destroy swapchain with id %u because it doesn't exist", swapchain.id);
        }
    }

    swapchain* graphics_device_opengl::get_swapchain_ptr_by_handle(swapchain_handle swapchain)
    {
        if (auto* desc = m_resources.swapchains.try_get(swapchain.id)) {
            return desc->swapchain_ptr.get();
        } else {
            ::logger.error("Can't find swapchain with id %u", swapchain.id);
            return nullptr;
        }
    }

    sampler_handle graphics_device_opengl::create_sampler(const sampler_desc& desc)
    {
        GLuint sampler;
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

        sampler_handle handle = {m_resources.samplers.insert({desc, sampler})};
        ::logger.debug("Sampler with id %u created", handle.id);
        return handle;
    }

    void graphics_device_opengl::destroy_sampler(sampler_handle sampler)
    {
        if (auto* desc = m_resources.samplers.try_get(sampler.id)) {
            glDeleteSamplers(1, &desc->sampler_obj);
            m_resources.samplers.remove(sampler.id);
            ::logger.debug("Sampler with id %u destroyed", sampler.id);
        } else {
            ::logger.error("Can't destroy sampler with id %u because it doesn't exist", sampler.id);
        }
    }

    texture_handle graphics_device_opengl::create_texture(
        const texture_desc& desc,
        const uint8*        pixels,
        uint32              stride
    )
    {
        auto gl_internal_format = to_gl_internal_format(desc.format);
        auto gl_depth_stencil_format = to_depth_stencil_fromat(desc.format);

        // Validate texture width/height
        if (desc.width == 0 || desc.height == 0) {
            ::logger.error("Texture width and height must be greater than zero");
            return {0};
        }

        // Validate texture depth
        if (desc.depth == 0) {
            ::logger.error("Texture depth must be at least 1");
            return {0};
        } else if (desc.depth != 1) {
            ::logger.error("Texture depth support is only 2D textures");
            return {0};
        }

        // Validate texture array layers
        if (desc.array_layers == 0) {
            ::logger.error("Texture array_layers must be at least 1");
            return {0};
        } else if (desc.array_layers != 1) {
            ::logger.error("Texture array_layers support is only for single layer textures");
            return {0};
        }

        if (desc.mip_levels == 0) {
            ::logger.error("Texture mip_levels must be at least 1");
            return {0};
        }

        // Validate sample count
        if (desc.sample_count > 1) {
            // Resolve source texture must be a render target with sample_count == 1
            if (desc.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error("Resolve destination texture must have sample_count == 1");
                return {0};
            }

            // Multisample texture cannot be used as storage
            if (desc.usage.has_flag(texture_usage::storage)) {
                ::logger.error("Multisample texture cannot be used as storage");
                return {0};
            }

            // Multisample texture cannot be used as sampled
            if (desc.usage.has_flag(texture_usage::sampled)) {
                ::logger.error("Multisample texture cannot be used as sampled");
                return {0};
            }

            // Resolve source texture must have sample_count == 1
            if (desc.usage.has_flag(texture_usage::resolve_source)) {
                ::logger.error("Resolve source texture must have sample_count == 1");
                return {0};
            }

            // Mip levels should be 1 for multisample textures
            if (desc.mip_levels > 1) {
                ::logger.error("Multisample texture cannot have mip levels");
                return {0};
            }

            // Pixels can't be provided for multisample textures
            if (pixels != nullptr) {
                ::logger.error("Texture pixels can't be provided for multisample textures");
                return {0};
            }
        }

        if (desc.usage.has_flag(texture_usage::resolve_source)) {
            // Resolve source texture must be a render target
            if (!desc.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Resolve source texture must be a render target");
                return {0};
            }
        }

        if (desc.usage.has_flag(texture_usage::depth_stencil_target)) {
            // Depth stencil target texture must be a depth stencil format
            if (!gl_depth_stencil_format.is_depth_stencil_format) {
                ::logger.error("Depth stencil target texture must be a depth stencil format");
                return {0};
            }

            // Depth stencil target texture cannot be used as sampled
            if (desc.usage.has_flag(texture_usage::storage)) {
                ::logger.error("Depth stencil target texture cannot be used as sampled");
                return {0};
            }
        }

        if (pixels != nullptr && !desc.usage.has_flag(texture_usage::transfer_destination)) {
            ::logger.error("Texture pixels can only be provided for transfer_destination textures");
            return {0};
        }

        auto   is_multisample = desc.sample_count > 1;
        GLenum target = is_multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

        GLuint tex;
        glGenTextures(1, &tex);
        ::logger.debug("OpenGL Texture object with id %u created", tex);

        auto texture_owner = core::make_scoped_owner(tex, [target](GLuint id) {
            glBindTexture(target, 0);
            glDeleteTextures(1, &id);
            ::logger.debug("OpenGL Texture object with id %u deleted", id);
        });

        // Bind texture
        glBindTexture(target, tex);

        if (is_multisample) {
            glTexImage2DMultisample(
                target,
                desc.sample_count,
                gl_internal_format.internal_format,
                desc.width,
                desc.height,
                GL_TRUE
            );
        } else {
            if (stride != 0) {
                // Set the row length to match the stride
                uint32 row_length_in_pixels = stride / gl_internal_format.bytes;
                glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_in_pixels);
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(
                target,
                0, // mip level
                gl_internal_format.internal_format,
                desc.width,
                desc.height,
                0, // border
                gl_internal_format.format,
                gl_internal_format.type,
                pixels
            );

            // Set the default alignment
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

            // Generate mipmaps
            if (desc.mip_levels > 1 && pixels != nullptr) {
                glGenerateMipmap(target);
            }
        }

        glBindTexture(target, 0);

        texture_handle handle = {m_resources.textures.insert({desc, texture_owner.release(), target})};
        ::logger.debug("Texture with id %u created", handle.id);
        return handle;
    }

    void graphics_device_opengl::destroy_texture(texture_handle texture)
    {
        if (auto* desc = m_resources.textures.try_get(texture.id)) {
            glDeleteTextures(1, &desc->texture_obj);
            ::logger.debug("OpenGL Texture object with id %u deleted", desc->texture_obj);
            m_resources.textures.remove(texture.id);
            ::logger.debug("Texture with id %u destroyed", texture.id);
        } else {
            ::logger.error("Can't destroy texture with id %u because it doesn't exist", texture.id);
        }
    }

    pipeline_handle graphics_device_opengl::create_pipeline(const pipeline_desc& desc)
    {
        // create shader program
        auto v = compile_shader(desc.shaders.vertex_source, GL_VERTEX_SHADER);
        auto f = compile_shader(desc.shaders.fragment_source, GL_FRAGMENT_SHADER);

        // Validate shaders
        if (v == 0) {
            ::logger.error("Can't compile vertex shader");
            if (f) {
                glDeleteShader(f);
            }
            return {0};
        }
        if (f == 0) {
            ::logger.error("Can't compile fragment shader");
            glDeleteShader(v);
            return {0};
        }

        auto program = link_program(v, f);
        glDeleteShader(v);
        glDeleteShader(f);

        // Validate program
        if (program == 0) {
            ::logger.error("Can't link program");
            return {0};
        }

        // create pipeline
        pipeline_handle handle = {m_resources.pipelines.insert({desc, program})};
        ::logger.debug("Pipeline with id %u created", handle.id);
        return handle;
    }

    void graphics_device_opengl::destroy_pipeline(pipeline_handle handle)
    {
        if (auto* desc = m_resources.pipelines.try_get(handle.id)) {
            glDeleteProgram(desc->program_obj);
            m_resources.pipelines.remove(handle.id);
            ::logger.debug("Pipeline with id %u destroyed", handle.id);
        } else {
            ::logger.error("Can't destroy pipeline with id %u because it doesn't exist", handle.id);
        }
    }

    framebuffer_handle graphics_device_opengl::create_framebuffer(
        const framebuffer_desc&                desc,
        const core::span<const texture_handle> color_attachments,
        core::optional<texture_handle>         depth_stencil_attachment
    )
    {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);

        ::logger.debug("OpenGL Framebuffer object with id %u created", fbo);

        // Scope for framebuffer deletion if something goes wrong
        auto fbo_owner = core::make_scoped_owner(fbo, [](GLuint id) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &id);
            ::logger.debug("OpenGL Framebuffer object with id %u deleted", id);
        });

        // Bind framebuffer and attach textures
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Validate attachments size
        if (color_attachments.size() != desc.color_attachment_formats.size()) {
            ::logger.error("Incorrect number of attachments");
            return {0};
        }

        // Validate attachments
        core::static_vector<gl_texture*, k_max_color_attachments> color_attachment_textures;
        core::static_vector<gl_texture*, k_max_color_attachments> resolve_textures;
        for (uint32 i = 0; i < color_attachments.size(); ++i) {
            const texture_handle& handle = color_attachments[i];
            if (auto* tex = m_resources.textures.try_get(handle.id)) {
                if (tex->desc.width != desc.width || tex->desc.height != desc.height) {
                    ::logger.error("Invalid attachment size (framebuffer: %ux%u) != (attachment: %ux%u)", desc.width, desc.height, tex->desc.width, tex->desc.height);
                    return {0};
                }

                // Validate formats
                if (!is_color_fromat(desc.color_attachment_formats[i])) {
                    ::logger.error("Unsupported color attachment format");
                    return {0};
                }

                if (!is_color_fromat(tex->desc.format)) {
                    ::logger.error("Unsupported texture format for color attachment");
                    return {0};
                }

                if (desc.color_attachment_formats[i] != tex->desc.format) {
                    ::logger.error("Color attachment format mismatch with texture format");
                    return {0};
                }

                // If multisampling is not eanbled for framebuffer, it must be disabled for attachments
                if (desc.sample_count == 1) {
                    // All attachments must use the same sample count
                    if (tex->desc.sample_count != 1) {
                        ::logger.error("Multisampling is not enabled for framebuffer, but enabled for attachment");
                        return {0};
                    }

                    // All the attachments must be used as color attachments
                    if (!tex->desc.usage.has_flag(texture_usage::render_target)) {
                        ::logger.error("Texture attachment is not a render target");
                        return {0};
                    }

                    // All the attachments must be used as color attachments
                    color_attachment_textures.push_back(tex);
                } else {
                    // Multisampling is enabled for framebuffer
                    if (tex->desc.sample_count == 1) {
                        // The attachment will be used as a resolve target because tex->desc.sample_count == 1
                        // Also the attachmend with sample count 1 should be with usage flag texture_usage::resolve_destination
                        if (!tex->desc.usage.has_flag(texture_usage::resolve_destination)) {
                            ::logger.error("Texture attachment is not a resolve destination target");
                            return {0};
                        }

                        // The attachment will be used as a resolve target
                        resolve_textures.push_back(tex);
                    } else if (tex->desc.sample_count == desc.sample_count) {
                        // The attachment will be used as a color attachment because tex->desc.sample_count == desc.sample_count
                        // This texture sould be with usage flag texture_usage::render_target
                        if (!tex->desc.usage.has_flag(texture_usage::render_target)) {
                            ::logger.error("Texture attachment is not a render target");
                            return {0};
                        }

                        // Texture with sample count more than 1 should be with usage flag texture_usage::resolve_source
                        if (!tex->desc.usage.has_flag(texture_usage::resolve_source)) {
                            ::logger.error("Texture attachment is not a resolve source target");
                            return {0};
                        }

                        // The attachment will be used as a color attachment
                        color_attachment_textures.push_back(tex);
                    } else {
                        // Not a resolve target and not a color attachment
                        ::logger.error("Multisampling is enabled for framebuffer, but attachment has invalid sample count");
                        return {0};
                    }
                }
            } else {
                // Texture not found, so the framebuffer can't be created
                ::logger.error("Can't find texture with id %u", handle.id);
                return {0};
            }
        }

        // Verify only resolve attachments
        if (desc.sample_count != 1) {
            // If multisampling is enabled, then for each color attachment there may be only one resolve attachment
            if (resolve_textures.size() > color_attachment_textures.size()) {
                ::logger.error("Multisampling is enabled for framebuffer, but the number of resolve attachments is greater than the number of color attachments");
                return {0};
            }
        }

        // Attach color textures to the framebuffer
        core::static_vector<GLenum, k_max_color_attachments> draw_buffers;
        for (uint32 i = 0; i < color_attachment_textures.size(); ++i) {
            auto* tex = color_attachment_textures[i];
            glBindTexture(tex->target, tex->texture_obj);
            GLenum attachment_type = GL_COLOR_ATTACHMENT0 + i;
            draw_buffers.push_back(attachment_type);
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_type, tex->target, tex->texture_obj, 0);
        }

        // Attach depth/stencil texture to the framebuffer if provided
        if (desc.depth_stencil_attachment_format != pixel_format::none) {
            auto gl_format = to_depth_stencil_fromat(desc.depth_stencil_attachment_format);
            if (gl_format.is_depth_stencil_format) {
                // Should be with depth/stencil attachment
                if (!depth_stencil_attachment.has_value()) {
                    ::logger.error("Depth/stencil attachment is not provided");
                    return {0};
                }

                auto depth_stencil_attachment_value = depth_stencil_attachment.value();

                // Get depth/stencil texture and attach it
                if (auto* tex = m_resources.textures.try_get(depth_stencil_attachment_value.id)) {
                    // Validate depth/stencil size
                    if (tex->desc.width != desc.width || tex->desc.height != desc.height) {
                        ::logger.error("Invalid depth/stencil attachment size (framebuffer: %ux%u) != (attachment: %ux%u)", desc.width, desc.height, tex->desc.width, tex->desc.height);
                        return {0};
                    }

                    // Validate depth/stencil format
                    if (tex->desc.format != desc.depth_stencil_attachment_format) {
                        ::logger.error("Invalid depth/stencil attachment format");
                        return {0};
                    }

                    // Validate depth/stencil usage
                    if (!tex->desc.usage.has_flag(texture_usage::depth_stencil_target)) {
                        ::logger.error("Depth/stencil attachment is not a depth/stencil target");
                        return {0};
                    }

                    // Everything is ok, attach depth/stencil texture
                    glFramebufferTexture2D(GL_FRAMEBUFFER, gl_format.depth_stencil_attachment_type, GL_TEXTURE_2D, tex->texture_obj, 0);
                } else {
                    ::logger.error("Can't find texture with id %u");
                    return {0};
                }
            } else {
                ::logger.error("Unsupported depth/stencil format");
                return {0};
            }
        } else {
            // Check if depth/stencil attachment is provided
            if (depth_stencil_attachment.has_value()) {
                ::logger.error("Depth/stencil attachment is provided, but depth/stencil is not enabled");
                return {0};
            }
        }

        // Framebuffer will be incomplete if the depth/stencil texture is not attached and
        // there are no color attachments
        if (draw_buffers.size() == 0 && desc.depth_stencil_attachment_format == pixel_format::none) {
            ::logger.error("There are no attachments for framebuffer");
            return {0};
        }

        // Enable color attachments
        if (draw_buffers.size() == 0) {
            // Drawing only onto depth/stencil buffer
            // Framebuffer will be incomplete if the depth/stencil texture is not attached
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        } else {
            // Drawing onto color attachments
            glDrawBuffers(draw_buffers.size(), draw_buffers.data());
        }

        // Validate framebuffer
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            ::logger.error("Framebuffer is not complete");
            return {0};
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        framebuffer_handle handle = {m_resources.framebuffers.insert({desc, fbo_owner.release(), false})};
        ::logger.debug("Framebuffer with id %u created", handle.id);
        return handle;
    }

    void graphics_device_opengl::destroy_framebuffer(framebuffer_handle framebuffer)
    {
        if (auto* desc = m_resources.framebuffers.try_get(framebuffer.id)) {
            glDeleteFramebuffers(1, &desc->framebuffer_obj);
            ::logger.debug("OpenGL Framebuffer object with id %u deleted", desc->framebuffer_obj);
            m_resources.framebuffers.remove(framebuffer.id);
            ::logger.debug("Framebuffer with id %u destroyed", framebuffer.id);
        } else {
            ::logger.error("Can't destroy framebuffer with id %u because it doesn't exist", framebuffer.id);
        }
    }

    buffer_handle graphics_device_opengl::create_buffer(
        const buffer_desc& desc,
        const uint8*       data,
        uint64             size
    )
    {
        GLenum gl_target = 0;
        switch (desc.usage) {
        case buffer_usage::vertex:
            gl_target = GL_ARRAY_BUFFER;
            break;
        case buffer_usage::index:
            gl_target = GL_ELEMENT_ARRAY_BUFFER;
            break;
        case buffer_usage::uniform:
            gl_target = GL_UNIFORM_BUFFER;
            break;
        default:
            ::logger.error("Unknown buffer usage");
            return {0};
        }

        GLenum gl_usage = 0;
        switch (desc.access) {
        case buffer_access::gpu_only:
            gl_usage = GL_STATIC_DRAW;
            break;
        case buffer_access::cpu_to_gpu:
            gl_usage = GL_DYNAMIC_DRAW;
            break;
        case buffer_access::gpu_to_cpu:
            gl_usage = GL_DYNAMIC_READ;
            break;
        default:
            ::logger.error("Unknown buffer access");
            return {0};
        }

        GLuint bo;
        glGenBuffers(1, &bo);
        ::logger.debug("OpenGL Buffer object with id %u created", bo);

        auto bo_owner = core::make_scoped_owner(bo, [gl_target](GLuint id) {
            glBindBuffer(gl_target, 0);
            glDeleteBuffers(1, &id);
            ::logger.debug("OpenGL Buffer object with id %u deleted", id);
        });

        glBindBuffer(gl_target, bo);

        // Allocate buffer
        glBufferData(gl_target, static_cast<GLsizeiptr>(desc.size), nullptr, gl_usage);

        // Set buffer data
        if (data != nullptr) {
            if (size > desc.size) {
                ::logger.debug("Buffer size is greater than buffer description size");
                return {0};
            } else if (size == 0) {
                ::logger.debug("Buffer size is zero");
                return {0};
            }
            glBufferData(gl_target, static_cast<GLsizeiptr>(size), data, gl_usage);
        }

        glBindBuffer(gl_target, 0); // Unbind for safety

        buffer_handle handle = {m_resources.buffers.insert({desc, bo_owner.release()})};
        ::logger.debug("Buffer with id %u created", handle.id);
        return handle;
    }

    void graphics_device_opengl::destroy_buffer(buffer_handle buffer)
    {
        if (auto* desc = m_resources.buffers.try_get(buffer.id)) {
            glDeleteBuffers(1, &desc->buffer_obj);
            ::logger.debug("OpenGL Buffer object with id %u deleted", desc->buffer_obj);
            m_resources.buffers.remove(buffer.id);
        } else {
            ::logger.error("Can't destroy buffer with id %u because it doesn't exist", buffer.id);
        }
    }

    geometry_binding_handle graphics_device_opengl::create_geometry(
        const geometry_binding_desc&          desc,
        const core::span<const buffer_handle> vertex_buffers,
        core::optional<buffer_handle>         index_buffer
    )
    {
        // Validate desc
        if (desc.layout.attributes.size() != desc.attribute_mapping.size()) {
            ::logger.error("Invalid vertex attribute mapping size");
            return {0};
        }

        // Check vertex buffers
        if (vertex_buffers.size() == 0) {
            ::logger.error("No vertex buffers");
            return {0};
        }

        if (vertex_buffers.size() != desc.buffer_mapping.size()) {
            ::logger.error("Invalid vertex mapping");
            return {0};
        }

        // Check index buffer
        if (desc.has_index_buffer && index_buffer == core::nullopt) {
            ::logger.error("Index buffer is missing");
            return {0};
        }

        if (!desc.has_index_buffer && index_buffer != core::nullopt) {
            ::logger.error("Index buffer is not enabled");
            return {0};
        }

        // Create VAO
        GLuint vao;
        glGenVertexArrays(1, &vao);

        ::logger.debug("OpenGL Vertex Array object with id %u created", vao);

        // Scope for vertex array deletion if something goes wrong
        auto vao_owner = core::make_scoped_owner(vao, [](GLuint id) {
            glBindVertexArray(0);
            glDeleteVertexArrays(1, &id);
            ::logger.debug("OpenGL Vertex Array object with id %u deleted", id);
        });

        glBindVertexArray(vao);

        // Bind vertex buffers
        for (uint32 i = 0; i < vertex_buffers.size(); ++i) {
            const auto  buffer = vertex_buffers[i];
            const auto& mapping = desc.buffer_mapping[i];
            if (auto* desc = m_resources.buffers.try_get(buffer.id)) {
                glBindVertexBuffer(mapping.binding, desc->buffer_obj, mapping.offset, mapping.stride);

                // Validate usage
                if (desc->desc.usage != buffer_usage::vertex) {
                    ::logger.error("Invalid vertex buffer usage");
                    return {0};
                }
            } else {
                ::logger.error("Can't find vertex buffer with id %u", buffer.id);
                return {0};
            }
        }

        // Bind vertex attributes
        for (uint32 i = 0; i < desc.layout.attributes.size(); ++i) {
            const auto& attribute = desc.layout.attributes[i];
            const auto& attribute_mapping = desc.attribute_mapping[i];

            auto buffer_index = static_cast<GLuint>(attribute_mapping.buffer_index);

            // Validate buffer index
            if (buffer_index >= vertex_buffers.size()) {
                ::logger.error("Invalid buffer index in vertex attribute mapping");
                return {0};
            }

            // OpenGL format info
            auto gl_format = to_gl_attribute_format(attribute.format);
            auto attrib_index = static_cast<GLuint>(i);
            auto normalie = attribute.normalize ? GL_TRUE : GL_FALSE;
            auto binding = static_cast<GLuint>(desc.buffer_mapping[buffer_index].binding);

            // Enable attribute and set pointer
            glEnableVertexAttribArray(attrib_index);
            glVertexAttribFormat(attrib_index, gl_format.size, gl_format.type, normalie, attribute_mapping.offset);
            glVertexAttribBinding(attrib_index, binding);
        }

        if (desc.has_index_buffer) {
            if (auto* desc = m_resources.buffers.try_get(index_buffer->id)) {
                if (desc->desc.usage != buffer_usage::index) {
                    ::logger.error("Invalid index buffer usage");
                    return {0};
                }

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, desc->buffer_obj);
            } else {
                ::logger.error("Can't find index buffer with id %u", index_buffer->id);
                return {0};
            }
        }

        glBindVertexArray(0);

        geometry_binding_handle handle = {m_resources.geometry_bindings.insert({desc, vao_owner.release()})};
        ::logger.debug("Geometry binding with id %u created", handle.id);
        return handle;
    }

    void graphics_device_opengl::destroy_geometry(geometry_binding_handle geometry_binding)
    {
        if (auto* desc = m_resources.geometry_bindings.try_get(geometry_binding.id)) {
            glDeleteVertexArrays(1, &desc->vao_obj);
            ::logger.debug("OpenGL Vertex Array object with id %u deleted", desc->vao_obj);
            m_resources.geometry_bindings.remove(geometry_binding.id);
        } else {
            ::logger.error("Can't destroy geometry binding with id %u because it doesn't exist", geometry_binding.id);
        }
    }

    device_resources_opengl* graphics_device_opengl::get_resources()
    {
        return &m_resources;
    }

} // namespace tavros::renderer
