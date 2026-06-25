#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

#include <tavros/renderer/internal/opengl/frame_composer_opengl.hpp>
#include <tavros/renderer/internal/opengl/type_conversions.hpp>
#include <tavros/renderer/internal/opengl/gl_check.hpp>

#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/raii/scoped_owner.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/math/functions/basic_math.hpp>

#include <tavros/renderer/rhi/string_utils.hpp>

using namespace tavros::renderer::rhi;

namespace
{
    tavros::core::logger logger("graphics_device_opengl");

    GLuint compile_shader_module(tavros::core::string_view program, GLenum shader_type)
    {
        auto shader = glCreateShader(shader_type);
        if (shader == 0) {
            logger.error("glCreateShader() returns 0");
            return 0;
        }

        // compile shader
        const char* program_text = program.data();
        auto        text_length = static_cast<GLint>(program.size());
        GL_CALL(glShaderSource(shader, 1, &program_text, &text_length));
        GL_CALL(glCompileShader(shader));

        // check compile status
        GLint status;
        GL_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
        if (!status) {
            char buffer[4096];
            GL_CALL(glGetShaderInfoLog(shader, sizeof(buffer), nullptr, buffer));
            logger.error("glCompileShader() failed:\n{}", buffer);
            GL_CALL(glDeleteShader(shader));
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

        GL_CALL(glAttachShader(program, vert_shader));
        GL_CALL(glAttachShader(program, frag_shader));
        GL_CALL(glLinkProgram(program));

        // check link status
        GLint status;
        GL_CALL(glGetProgramiv(program, GL_LINK_STATUS, &status));
        if (!status) {
            char buffer[4096];
            GL_CALL(glGetProgramInfoLog(program, sizeof(buffer), nullptr, buffer));
            logger.error("glLinkProgram() failed:\n{}", buffer);
            failed = true;
        }

        // validate program
        GL_CALL(glValidateProgram(program));
        GL_CALL(glGetProgramiv(program, GL_VALIDATE_STATUS, &status));
        if (!status) {
            char buffer[4096];
            GL_CALL(glGetProgramInfoLog(program, sizeof(buffer), nullptr, buffer));
            logger.error("glValidateProgram() failed:\n{}", buffer);
            failed = true;
        }

        GL_CALL(glDetachShader(program, vert_shader));
        GL_CALL(glDetachShader(program, frag_shader));

        if (failed) {
            GL_CALL(glDeleteProgram(program));
            return 0;
        }

        return program;
    }

    void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
    {
        TAV_UNUSED(type);
        TAV_UNUSED(source);
        TAV_UNUSED(user_param);
        TAV_UNUSED(id);

        tavros::core::logger l("OpenGL_debug");
        if (severity == GL_DEBUG_SEVERITY_HIGH) {
            l.error("{}", tavros::core::string_view(message, length));
        } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
            l.warning("{}", tavros::core::string_view(message, length));
        } else if (severity == GL_DEBUG_SEVERITY_LOW) {
            l.info("{}", tavros::core::string_view(message, length));
        } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            // l.info("{}", tavros::core::string_view(message, length));
        } else {
            l.debug("Unknown severity: {}", tavros::core::string_view(message, length));
        }
    }

    void init_gl_debug()
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    template<class HandleT, class Del>
    void destroy_for(tavros::renderer::rhi::device_resources_opengl& res, Del deleter)
    {
        auto& pool = res.get_pool<tavros::renderer::rhi::device_resources_opengl::object_type_of<HandleT>>();
        for (auto [h, obj] : pool) {
            deleter(h);
        }
    }

} // namespace

namespace tavros::renderer::rhi
{

    graphics_device_opengl::graphics_device_opengl()
        : graphics_device()
        , m_resources()
    {
        ::logger.debug("graphics_device_opengl created");
    }

    graphics_device_opengl::~graphics_device_opengl()
    {
        destroy();
        ::logger.debug("graphics_device_opengl destroyed");
    }

    void graphics_device_opengl::init_limits()
    {
        GLint value = 0;

        // Texture limits
        GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value));
        m_limits.max_2d_texture_size = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &value);
        m_limits.max_3d_texture_size = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &value);
        m_limits.max_array_texture_layers = static_cast<uint32>(value);

        // Uniform / SSBO
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &value);
        m_limits.max_ubo_size = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
        m_limits.max_ssbo_size = static_cast<uint32>(value);

        // Vertex attributes
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
        m_limits.max_vertex_attributes = static_cast<uint32>(value);

        // Framebuffer / Renderbuffer
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &value);
        m_limits.max_color_attachments = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &value);
        m_limits.max_draw_buffers = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &value);
        m_limits.max_renderbuffer_size = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &value);
        m_limits.max_framebuffer_width = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &value);
        m_limits.max_framebuffer_height = static_cast<uint32>(value);
    }

    void graphics_device_opengl::release_program(gl_program_handle handle) noexcept
    {
        if (auto* p = m_resources.find(handle)) {
            if (p->rc.decrement()) {
                GL_CALL(glDeleteProgram(p->prog_obj));
                m_resources.remove(handle);
                ::logger.debug("Program {} deleted", handle);
            }
        } else {
            ::logger.error("Failed to release program {}: not found", handle);
        }
    }

    void graphics_device_opengl::destroy()
    {
        destroy_for<sampler_handle>(m_resources, [this](auto h) { destroy_sampler(h); });

        destroy_for<texture_handle>(m_resources, [this](auto h) { destroy_texture(h); });

        destroy_for<shader_handle>(m_resources, [this](auto h) { destroy_shader(h); });

        destroy_for<pipeline_handle>(m_resources, [this](auto h) { destroy_pipeline(h); });

        destroy_for<framebuffer_handle>(m_resources, [this](auto h) {
            if (auto* fb = m_resources.find(framebuffer_handle(h))) {
                if (!fb->is_default) {
                    // We only delete non default framebuffers
                    // Default ones should be deleted elsewhere, in frame_composer
                    destroy_framebuffer(h);
                }
            } else {
                // This shouldn't happen, but just in case, we call a method that will throw an error in the log
                destroy_framebuffer(h);
                TAV_UNREACHABLE();
            }
        });

        destroy_for<buffer_handle>(m_resources, [this](auto h) { destroy_buffer(h); });

        destroy_for<fence_handle>(m_resources, [this](auto h) { destroy_fence(h); });

        // Should be removed in last turn because swapchain owns the OpenGL context
        destroy_for<frame_composer_handle>(m_resources, [this](auto h) { destroy_frame_composer(h); });
    }

    frame_composer_handle graphics_device_opengl::create_frame_composer(const frame_composer_create_info& info)
    {
        // Check if frame composer with native handle already created
        bool has_native_handle = false;

        auto& pool = m_resources.get_pool<gl_composer>();
        for (auto [handle, obj] : pool) {
            if (info.native_handle == obj->info.native_handle) {
                has_native_handle = true;
                break;
            }
        }

        if (has_native_handle) {
            ::logger.error("Failed to create frame composer: native handle {} already exists", info.native_handle);
            return {};
        }

        // Create a new frame composer
        auto composer = frame_composer_opengl::create(this, info, info.native_handle);

        if (!composer) {
            // Detailed info sould be written at frame_composer_opengl
            ::logger.error("Failed to create frame composer");
            return {};
        }

        // Initialize debug callback for OpenGL here, because it's not possible to do it in the constructor
        // the first call of frame_composer_opengl::create() will create the context
#if TAV_DEBUG
        init_gl_debug();
#endif

        init_limits();

        GL_CALL(glEnable(GL_PROGRAM_POINT_SIZE));

        auto h = m_resources.create(gl_composer{info, std::move(composer)});
        ::logger.debug("Frame composer {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_frame_composer(frame_composer_handle composer)
    {
        if (auto* fc = m_resources.find(composer)) {
            m_resources.remove(composer);
            ::logger.debug("Frame composer {} destroyed", composer);
        } else {
            ::logger.error("Failed to destroy frame composer {}: not found", composer);
        }
    }

    frame_composer* graphics_device_opengl::get_frame_composer_ptr(frame_composer_handle composer)
    {
        if (auto* fc = m_resources.find(composer)) {
            return fc->composer_ptr.get();
        } else {
            ::logger.error("Failed to get frame composer {}: not found", composer);
            return nullptr;
        }
    }

    shader_handle graphics_device_opengl::create_shader(const shader_create_info& info)
    {
        auto deleter = [](GLuint o) { if (o != 0) {GL_CALL(glDeleteShader(o));} };
        auto vso_owner = core::make_scoped_owner(compile_shader_module(info.vertex_shader_source, GL_VERTEX_SHADER), deleter);
        auto fso_owner = core::make_scoped_owner(compile_shader_module(info.fragment_shader_source, GL_FRAGMENT_SHADER), deleter);

        if (vso_owner.get() == 0 || fso_owner.get() == 0) {
            ::logger.error("Failed to create shader: compilation failed");
            return {};
        }

        auto prog = link_program(vso_owner.get(), fso_owner.get());
        if (prog == 0) {
            ::logger.error("Failed to create shader: failed to link program");
            return {};
        }

        const bool is_compute = false;
        auto       reflect = core::make_unique<gl_shader_program_reflect>(prog, is_compute);
        if (!reflect->is_valid()) {
            ::logger.error("Failed to create shader: conventions are violated");
            GL_CALL(glDeleteProgram(prog));
            return {};
        }

        auto sh_h = m_resources.create(gl_program{prog, {}});
        auto h = m_resources.create(gl_shader{sh_h, std::move(reflect)});
        ::logger.debug("Shader {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_shader(shader_handle shader)
    {
        if (auto* sh = m_resources.find(shader)) {
            release_program(sh->program_h);
            m_resources.remove(shader);
            ::logger.debug("Shader {} destroyed", shader);
        } else {
            ::logger.error("Failed to destroy shader {}: not found", shader);
        }
    }

    const shader_reflect* graphics_device_opengl::get_shader_reflect_ptr(shader_handle shader) const noexcept
    {
        if (auto* p = m_resources.find(shader)) {
            return p->reflect.get();
        } else {
            ::logger.error("Failed to get shader reflection {}: not found", shader);
            return nullptr;
        }
    }

    sampler_handle graphics_device_opengl::create_sampler(const sampler_create_info& info)
    {
        GLuint sampler;
        GL_CALL(glGenSamplers(1, &sampler));

        auto filter = to_gl_filter(info.filter);

        // Filtering
        GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, filter.min_filter));
        GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, filter.mag_filter));

        // Wrapping
        GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, to_gl_wrap_mode(info.wrap_mode.wrap_s)));
        GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, to_gl_wrap_mode(info.wrap_mode.wrap_t)));
        GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, to_gl_wrap_mode(info.wrap_mode.wrap_r)));

        // LOD parameters
        GL_CALL(glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, info.mip_lod_bias));
        GL_CALL(glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, info.min_lod));
        GL_CALL(glSamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, info.max_lod));

        // Depth compare
        if (info.depth_compare != compare_op::off) {
            GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
            GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, to_gl_compare_func(info.depth_compare)));
        } else {
            GL_CALL(glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE));
        }

        auto h = m_resources.create(gl_sampler{info, sampler});
        ::logger.debug("Sampler {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_sampler(sampler_handle sampler)
    {
        if (auto* s = m_resources.find(sampler)) {
            GL_CALL(glDeleteSamplers(1, &s->sampler_obj));
            m_resources.remove(sampler);
            ::logger.debug("Sampler {} destroyed", sampler);
        } else {
            ::logger.error("Failed to destroy sampler {}: not found", sampler);
        }
    }

    texture_handle graphics_device_opengl::create_texture(const texture_create_info& info)
    {
        auto gl_pixel_format = to_gl_pixel_format(info.format);
        auto max_available_mip = math::mip_levels(info.width, info.height, info.depth);

        if (info.type == texture_type::texture_2d) {
            if (info.width == 0 || info.height == 0 || info.depth != 1) {
                ::logger.error("Failed to create `texture_2d`: width and height must be greater than 0, and depth must be 1");
                return {};
            }

            if (info.width > m_limits.max_2d_texture_size || info.height > m_limits.max_2d_texture_size) {
                ::logger.warning(
                    "Texture creation `texture_2d`: size {}x{} exceeds device limit {}",
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height),
                    fmt::styled_param(m_limits.max_2d_texture_size)
                );
            }

            if (info.sample_count == 0 || !math::is_power_of_two(info.sample_count) || info.sample_count > 32) {
                ::logger.error(
                    "Failed to create texture_2d: invalid sample count {}. Allowed values are 1, 2, 4, 8, 16, 32.",
                    fmt::styled_param(info.sample_count)
                );
                return {};
            }

            if (info.sample_count == 1 && info.usage.has_flag(texture_usage::resolve_source)) {
                GLint supported = 0;
                GL_CALL(glGetInternalformativ(GL_RENDERBUFFER, gl_pixel_format.internal_format, GL_INTERNALFORMAT_SUPPORTED, 1, &supported));
                if (supported == GL_FALSE) {
                    ::logger.error("Failed to create `texture_2d`: resolve source textures must have sample count > 1 (format is not supported by renderbuffer)");
                    return {};
                }
            }

            if (info.sample_count > 1 && info.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error("Failed to create `texture_2d`: resolve destination textures must have sample_count == 1");
                return {};
            }

            if (info.sample_count > 1 && info.usage.has_flag(texture_usage::storage)) {
                ::logger.error("Failed to create `texture_2d`: multisample textures cannot be used as storage");
                return {};
            }

            if (info.sample_count > 1 && info.usage.has_flag(texture_usage::sampled)) {
                ::logger.error("Failed to create `texture_2d`: multisample textures cannot be used as sampled");
                return {};
            }

            if (info.sample_count > 1 && info.mip_levels > 1) {
                ::logger.error("Failed to create `texture_2d`: multisample textures cannot have mip levels");
                return {};
            }

            if (info.mip_levels == 0) {
                ::logger.error("Failed to create `texture_2d`: mip levels must be at least 1");
                return {};
            }

            if (info.mip_levels > max_available_mip) {
                ::logger.error("Failed to create `texture_2d`: requested {} mip levels, but only {} are possible for size {}x{}", fmt::styled_param(info.mip_levels), fmt::styled_param(max_available_mip), fmt::styled_param(info.width), fmt::styled_param(info.height));
                return {};
            }

            // Resolve source texture must be a render target
            if (info.usage.has_flag(texture_usage::resolve_source) && !info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Failed to create `texture_2d`: resolve source textures must be render targets");
                return {};
            }

            // Multisampled texture must be a render target
            if (info.sample_count > 1 && !info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Failed to create `texture_2d`: multisampled textures must have render_target usage");
                return {};
            }

        } else if (info.type == texture_type::texture_3d) {
            if (info.width == 0 || info.height == 0 || info.depth == 0) {
                ::logger.error("Failed to create `texture_3d`: width, height and depth must be greater than 0");
                return {};
            }

            auto size_limit = m_limits.max_3d_texture_size;
            if (info.width > size_limit || info.height > size_limit || info.depth > size_limit) {
                ::logger.warning(
                    "Texture creation `texture_3d`: size {}x{}x{} exceeds device limit {}",
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height),
                    fmt::styled_param(info.depth),
                    fmt::styled_param(size_limit)
                );
            }

            if (info.array_layers != 1) {
                ::logger.error("Failed to create `texture_3d`: array layers must be 1");
                return {};
            }

            if (info.sample_count != 1) {
                ::logger.error("Failed to create `texture_3d`: sample count must be 1");
                return {};
            }

            if (info.mip_levels == 0) {
                ::logger.error("Failed to create `texture_3d`: mip levels must be at least 1");
                return {};
            }

            if (info.mip_levels > max_available_mip) {
                ::logger.error("Failed to create `texture_3d`: requested {} mip levels, but only {} are possible for size {}x{}x{}", fmt::styled_param(info.mip_levels), fmt::styled_param(max_available_mip), fmt::styled_param(info.width), fmt::styled_param(info.height), fmt::styled_param(info.depth));
                return {};
            }

            if (info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Failed to create `texture_3d`: usage flag `render_target` is not allowed");
                return {};
            }

            if (info.usage.has_flag(texture_usage::resolve_source)) {
                ::logger.error("Failed to create `texture_3d`: usage flags `resolve_source` is not allowed");
                return {};
            }

            if (info.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error("Failed to create `texture_3d`: usage flag `resolve_destination` is not allowed");
                return {};
            }

        } else if (info.type == texture_type::texture_cube) {
            if (info.width == 0 || info.height == 0 || info.width != info.height || info.depth != 1) {
                ::logger.error("Failed to create `texture_cube`: width and height must be equal and greater than 0, depth must be 1");
                return {};
            }

            auto size_limit = m_limits.max_2d_texture_size;
            if (info.width > size_limit || info.height > size_limit) {
                ::logger.warning(
                    "Texture creation `texture_cube`: size {}x{} exceeds device limit {}",
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height),
                    fmt::styled_param(size_limit)
                );
            }

            if (info.array_layers % 6 != 0 || info.array_layers != 6) {
                ::logger.error("Failed to create `texture_cube`: array layers must be a multiple of 6 and at least 6");
                return {};
            }

            if (info.sample_count != 1) {
                ::logger.error("Failed to create `texture_cube`: sample count must be 1");
                return {};
            }

            if (info.mip_levels == 0) {
                ::logger.error("Failed to create `texture_cube`: mip levels must be at least 1");
                return {};
            }

            if (info.mip_levels > max_available_mip) {
                ::logger.error("Failed to create `texture_cube`: requested {} mip levels, but only {} are possible for size {}x", fmt::styled_param(info.mip_levels), fmt::styled_param(max_available_mip), fmt::styled_param(info.width), fmt::styled_param(info.height));
                return {};
            }

            if (info.usage.has_flag(texture_usage::resolve_source)) {
                ::logger.error("Failed to create `texture_cube`: usage flags `resolve_source` is not allowed");
                return {};
            }

            if (info.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error("Failed to create `texture_cube`: usage flag `resolve_destination` is not allowed");
                return {};
            }

        } else {
            ::logger.error("Failed to create texture: unknown texture type");
            return {};
        }

        if (info.array_layers > m_limits.max_array_texture_layers) {
            ::logger.warning(
                "Texture creation `{}`: array layers {} exceeds device limit {}",
                to_string(info.type),
                fmt::styled_param(info.array_layers),
                fmt::styled_param(m_limits.max_array_texture_layers)
            );
        }

        // Checking for renderbuffer creation
        bool need_create_renderbuffer =
            info.usage.has_flag(texture_usage::render_target)
            && info.type == texture_type::texture_2d
            && info.array_layers == 1
            && !info.usage.has_flag(texture_usage::sampled);

        if (need_create_renderbuffer) {
            // Create a renderbuffer instead of a texture (OpenGL works faster with renderbuffer objects)

            TAV_ASSERT(info.type == texture_type::texture_2d);

            if (info.width > m_limits.max_renderbuffer_size || info.height > m_limits.max_renderbuffer_size) {
                ::logger.warning(
                    "Texture creation: renderbuffer size {}x{} exceeds device limit {}x{}",
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height),
                    fmt::styled_param(m_limits.max_framebuffer_width),
                    fmt::styled_param(m_limits.max_framebuffer_height)
                );
            }

            GLuint rbo;
            GL_CALL(glGenRenderbuffers(1, &rbo));
            GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, rbo));

            if (info.sample_count == 1) {
                // Regular renderbuffer
                GL_CALL(glRenderbufferStorage(
                    GL_RENDERBUFFER,
                    gl_pixel_format.internal_format,
                    info.width,
                    info.height
                ));
            } else {
                // Multisampled renderbuffer
                GL_CALL(glRenderbufferStorageMultisample(
                    GL_RENDERBUFFER,
                    info.sample_count,
                    gl_pixel_format.internal_format,
                    info.width,
                    info.height
                ));
            }

            GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            auto h = m_resources.create(gl_texture{info, 0, GL_RENDERBUFFER, rbo, 1});
            ::logger.debug("Texture ({}) {} created as {}", info.type, h, fmt::styled_param("renderbuffer"));
            return h;

        } else {
            // Otherwise, create a texture object

            auto mip_levels = static_cast<GLsizei>(info.mip_levels);

            GLenum gl_target = 0;
            switch (info.type) {
            case texture_type::texture_2d:
                if (info.array_layers > 1) {
                    gl_target = GL_TEXTURE_2D_ARRAY;
                } else if (info.sample_count > 1) {
                    gl_target = GL_TEXTURE_2D_MULTISAMPLE;
                } else {
                    gl_target = GL_TEXTURE_2D;
                }
                break;

            case texture_type::texture_3d:
                gl_target = GL_TEXTURE_3D;
                break;

            case texture_type::texture_cube:
                gl_target = GL_TEXTURE_CUBE_MAP;
                break;

            default:
                TAV_UNREACHABLE();
                break;
            }

            GLuint tex;
            GL_CALL(glGenTextures(1, &tex));

            // Bind texture
            GL_CALL(glBindTexture(gl_target, tex));

            switch (gl_target) {
            case GL_TEXTURE_2D_MULTISAMPLE:
                GL_CALL(glTexImage2DMultisample(
                    gl_target,
                    info.sample_count,
                    gl_pixel_format.internal_format,
                    info.width,
                    info.height,
                    GL_TRUE
                ));
                break;

            case GL_TEXTURE_2D:
                if (mip_levels > 1) {
                    GL_CALL(glTexStorage2D(
                        gl_target,
                        mip_levels,
                        gl_pixel_format.internal_format,
                        info.width,
                        info.height
                    ));
                } else {
                    GL_CALL(glTexImage2D(
                        gl_target,
                        0, // mip level
                        gl_pixel_format.internal_format,
                        info.width,
                        info.height,
                        0, // border, always 0
                        gl_pixel_format.format,
                        gl_pixel_format.type,
                        nullptr
                    ));
                }
                break;

            case GL_TEXTURE_2D_ARRAY:
                // glTexStorage3D / glTexImage3D - depth = array_layers
                if (mip_levels > 1) {
                    GL_CALL(glTexStorage3D(
                        gl_target,
                        mip_levels,
                        gl_pixel_format.internal_format,
                        info.width,
                        info.height,
                        static_cast<GLsizei>(info.array_layers)
                    ));
                } else {
                    GL_CALL(glTexImage3D(
                        gl_target,
                        0,
                        gl_pixel_format.internal_format,
                        info.width,
                        info.height,
                        static_cast<GLsizei>(info.array_layers),
                        0,
                        gl_pixel_format.format,
                        gl_pixel_format.type,
                        nullptr
                    ));
                }
                break;

            case GL_TEXTURE_3D:
                if (mip_levels > 1) {
                    GL_CALL(glTexStorage3D(
                        gl_target,
                        mip_levels,
                        gl_pixel_format.internal_format,
                        info.width,
                        info.height,
                        info.depth
                    ));
                } else {
                    GL_CALL(glTexImage3D(
                        gl_target,
                        0, // mip level
                        gl_pixel_format.internal_format,
                        info.width,
                        info.height,
                        info.depth,
                        0, // border, always 0
                        gl_pixel_format.format,
                        gl_pixel_format.type,
                        nullptr
                    ));
                }
                break;

            case GL_TEXTURE_CUBE_MAP:
                // Cubemap, allocate memory for all 6 faces
                if (mip_levels > 1) {
                    GL_CALL(glTexStorage2D(
                        gl_target,
                        mip_levels,
                        gl_pixel_format.internal_format,
                        info.width,
                        info.height
                    ));
                } else {
                    for (int32 i = 0; i < 6; ++i) {
                        GL_CALL(glTexImage2D(
                            static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
                            0, // mip level
                            gl_pixel_format.internal_format,
                            info.width,
                            info.height,
                            0, // border, always 0
                            gl_pixel_format.format,
                            gl_pixel_format.type,
                            nullptr
                        ));
                    }
                }

                break;

            default:
                TAV_UNREACHABLE();
                break;
            }

            GL_CALL(glTexParameteri(gl_target, GL_TEXTURE_BASE_LEVEL, 0));
            GL_CALL(glTexParameteri(gl_target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mip_levels - 1)));

            GL_CALL(glBindTexture(gl_target, 0));

            auto h = m_resources.create(gl_texture{info, tex, gl_target, 0, info.mip_levels});
            ::logger.debug("Texture ({}) {} created", info.type, h);
            return h;
        }
    }

    void graphics_device_opengl::destroy_texture(texture_handle texture)
    {
        if (auto* tex = m_resources.find(texture)) {
            auto is_texture = tex->texture_obj != 0;
            if (is_texture) {
                TAV_ASSERT(tex->renderbuffer_obj == 0);
                GL_CALL(glDeleteTextures(1, &tex->texture_obj));
            } else {
                TAV_ASSERT(tex->texture_obj == 0);
                TAV_ASSERT(tex->target == GL_RENDERBUFFER);
                GL_CALL(glDeleteRenderbuffers(1, &tex->renderbuffer_obj));
            }

            m_resources.remove(texture);
            if (is_texture) {
                ::logger.debug("Texture ({}) {} destroyed", tex->info.type, texture);
            } else {
                ::logger.debug("Texture ({}) {} destroyed as {}", tex->info.type, texture, fmt::styled_param("renderbuffer"));
            }
        } else {
            ::logger.error("Failed to destroy texture {}: not found", texture);
        }
    }

    pipeline_handle graphics_device_opengl::create_pipeline(const pipeline_create_info& info)
    {
        auto* sh = m_resources.find(info.shader_program);
        if (!sh) {
            ::logger.error("Failed to create pipeline: shader {} not found", info.shader_program);
            return {};
        }

        auto* p = m_resources.find(sh->program_h);
        if (!p) {
            ::logger.error("Failed to create pipeline: program {} not found", sh->program_h);
            return {};
        }

        // Validate attributes

        // Map attributes to location index, for fast search
        struct va_map_t
        {
            bool             has_attrib = false;
            vertex_attribute attrib;
        };
        va_map_t mapped_attributes[k_max_vertex_attributes];

        for (const auto& attr : info.bindings) {
            if (attr.location >= k_max_vertex_attributes) {
                ::logger.error(
                    "Failed to create pipeline: vertex attribute location {} exceeds maximum allowed ({})",
                    fmt::styled_param(attr.location),
                    fmt::styled_param(k_max_vertex_attributes)
                );
                return {};
            }
            auto& it = mapped_attributes[attr.location];
            if (it.has_attrib) {
                ::logger.error(
                    "Failed to create pipeline: vertex attribute location {} is used multiple times",
                    fmt::styled_param(attr.location)
                );
                return {};
            }
            it.has_attrib = true;
            it.attrib = attr;
        }

        auto prog = p->prog_obj;

        // Get number of attributes in compiled shader
        GLint gl_prog_attrib_count;
        GL_CALL(glGetProgramiv(prog, GL_ACTIVE_ATTRIBUTES, &gl_prog_attrib_count));

        // And validate each attribute
        size_t total_gl_attributes = 0;
        for (GLint i = 0; i < gl_prog_attrib_count; ++i) {
            GLchar attrib_name[256] = {0};
            GLint  size;
            GLenum type;
            GL_CALL(glGetActiveAttrib(prog, i, sizeof(attrib_name), nullptr, &size, &type, attrib_name));

            GLint gl_attrib_location = glGetAttribLocation(prog, attrib_name);
            if (gl_attrib_location < 0) {
                // builtin attribute, just ignore it
                continue;
            }

            auto shader_type = gl_type_to_rhi_type(type);
            if (!shader_type.valid) {
                ::logger.error(
                    "Failed to create pipeline: unsupported attribute type by RHI, for attribute name {}",
                    fmt::styled_text(attrib_name)
                );
                return {};
            }

            // Check attribute location
            auto location = static_cast<uint32>(gl_attrib_location);
            if (location >= k_max_vertex_attributes) {
                ::logger.error(
                    "Failed to create pipeline: vertex attribute location {} exceeds maximum allowed ({})",
                    fmt::styled_param(location),
                    fmt::styled_param(k_max_vertex_attributes)
                );
                return {};
            }

            const auto& it = mapped_attributes[location];
            if (!it.has_attrib) {
                ::logger.error(
                    "Failed to create pipeline: attribute location {} not found in provided attributes",
                    fmt::styled_param(gl_attrib_location)
                );
                return {};
            }

            // Check attribute type and format
            if (it.attrib.type != shader_type.type || it.attrib.format != shader_type.format) {
                ::logger.error(
                    "Failed to create pipeline: attribute {} at location {} has mismatched type/format. "
                    "Shader type/format = {}/{}, expected type/format = {}/{}",
                    fmt::styled_text(attrib_name),
                    fmt::styled_param(gl_attrib_location),
                    shader_type.type,
                    shader_type.format,
                    it.attrib.type,
                    it.attrib.format
                );
                return {};
            }

            total_gl_attributes++;
        }

        if (total_gl_attributes != info.bindings.size()) {
            ::logger.warning(
                "Pipeline creation: mismach attributes size. Provided {}, expected {}",
                fmt::styled_param(info.bindings.size()),
                fmt::styled_param(total_gl_attributes)
            );
        }

        // Create VAO
        GLuint vao = 0;
        if (0 != info.bindings.size()) {
            GL_CALL(glGenVertexArrays(1, &vao));

            if (vao == 0) {
                ::logger.error("Failed to create pipeline: failed to create VAO");
                return {};
            }

            GL_CALL(glBindVertexArray(vao));

            // Setup attribute bindings
            for (auto attrib_i = 0; attrib_i < info.bindings.size(); ++attrib_i) {
                auto& binding = info.bindings[attrib_i];

                bool is_flt = scalar_type::f32 == binding.type || scalar_type::f16 == binding.type || scalar_type::f64 == binding.type;
                auto gl_attrib_info = to_gl_attribute_info(binding.format, binding.type);
                for (int32 col = 0; col < gl_attrib_info.cols; ++col) {
                    GLuint location = binding.location + col;
                    GLuint offset = binding.offset + col * gl_attrib_info.rows * gl_attrib_info.size;

                    // Enable attribute and set pointer
                    GL_CALL(glEnableVertexAttribArray(location));
                    if (is_flt) {
                        GL_CALL(glVertexAttribFormat(location, gl_attrib_info.rows, gl_attrib_info.type, binding.normalize, offset));
                    } else {
                        GL_CALL(glVertexAttribIFormat(location, gl_attrib_info.rows, gl_attrib_info.type, offset));
                    }
                    GL_CALL(glVertexAttribBinding(location, attrib_i));
                }
                GL_CALL(glVertexBindingDivisor(attrib_i, binding.instance_divisor));
            }

            GL_CALL(glBindVertexArray(0));
        }

        p->rc.increment();
        // create pipeline
        auto h = m_resources.create(gl_pipeline{info, sh->program_h, p->prog_obj, vao});
        ::logger.debug("Pipeline {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_pipeline(pipeline_handle handle)
    {
        if (auto* sh = m_resources.find(handle)) {
            release_program(sh->program_h);
            if (0 != sh->vao_obj) {
                GL_CALL(glDeleteVertexArrays(1, &sh->vao_obj));
            }
            m_resources.remove(handle);
            ::logger.debug("Pipeline {} destroyed", handle);
        } else {
            ::logger.error("Failed to destroy pipeline {}: not found", handle);
        }
    }

    framebuffer_handle graphics_device_opengl::create_framebuffer(const framebuffer_create_info& info)
    {
        // ----------------------------------------------------------------
        // Framebuffer-level limits
        // ----------------------------------------------------------------
        if (info.width > m_limits.max_framebuffer_width || info.height > m_limits.max_framebuffer_height) {
            ::logger.warning(
                "Framebuffer creation: size {}x{} exceeds device limit {}x{}",
                fmt::styled_param(info.width),
                fmt::styled_param(info.height),
                fmt::styled_param(m_limits.max_framebuffer_width),
                fmt::styled_param(m_limits.max_framebuffer_height)
            );
        }

        if (static_cast<uint32>(info.color_attachments.size()) > m_limits.max_color_attachments) {
            ::logger.warning(
                "Framebuffer creation: color attachment count {} exceeds device limit {}",
                fmt::styled_param(info.color_attachments.size()),
                fmt::styled_param(m_limits.max_color_attachments)
            );
        }

        if (info.color_attachments.size() == 0 && !info.depth_stencil_attachment.target.valid()) {
            ::logger.error("Failed to create framebuffer: no color or depth/stencil attachments provided");
            return {};
        }

        // ----------------------------------------------------------------
        // Validate color attachments
        // ----------------------------------------------------------------
        core::fixed_vector<gl_texture*, k_max_color_attachments>  color_textures;
        core::fixed_vector<pixel_format, k_max_color_attachments> color_formats;

        for (size_t i = 0; i < info.color_attachments.size(); ++i) {
            const auto& ca = info.color_attachments[i];

            auto  src_tex_h = ca.target;
            auto* src_tex = m_resources.find(src_tex_h);
            if (!src_tex) {
                ::logger.error("Failed to create framebuffer: color attachment {} at index {} not found", src_tex_h, fmt::styled_param(i));
                return {};
            }

            // A resolve target is only meaningful when store op is `resolve`.
            // Cover every combination of (resolve target specified) x (store op == resolve) explicitly.
            auto  resolve_tex_h = ca.resolve_target;
            auto* resolve_tex = m_resources.find(resolve_tex_h);
            bool  check_resolve = false;
            bool  need_resolve = info.sample_count > 1 && ca.store == store_op::resolve;

            if (resolve_tex_h.valid()) {
                if (resolve_tex) {
                    if (ca.store == store_op::resolve) {
                        check_resolve = true;
                    } else {
                        ::logger.warning(
                            "Framebuffer creation: resolve attachment {} at index {} specified but store op is not resolve, ignored",
                            resolve_tex_h,
                            fmt::styled_param(i)
                        );
                    }
                } else {
                    if (ca.store == store_op::resolve) {
                        ::logger.error("Failed to create framebuffer: resolve attachment {} at index {} not found", resolve_tex_h, fmt::styled_param(i));
                        return {};
                    } else {
                        ::logger.warning(
                            "Framebuffer creation: resolve attachment {} at index {} not found, ignored (store op is not resolve)",
                            resolve_tex_h,
                            fmt::styled_param(i)
                        );
                    }
                }
            } else if (ca.store == store_op::resolve) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} store op is resolve but no resolve attachment specified",
                    src_tex_h,
                    fmt::styled_param(i)
                );
                return {};
            }

            // Validate formats
            if (!is_color_format(src_tex->info.format)) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} has unsupported format {}",
                    src_tex_h,
                    fmt::styled_param(i),
                    src_tex->info.format
                );
                return {};
            }

            if (check_resolve && !is_color_format(resolve_tex->info.format)) {
                ::logger.error(
                    "Failed to create framebuffer: resolve attachment {} at index {} has unsupported format {}",
                    resolve_tex_h,
                    fmt::styled_param(i),
                    resolve_tex->info.format
                );
                return {};
            }

            if (check_resolve && src_tex->info.format != resolve_tex->info.format) {
                ::logger.error(
                    "Failed to create framebuffer: resolve attachment {} at index {} format {} does not match color attachment format {}",
                    resolve_tex_h,
                    fmt::styled_param(i),
                    resolve_tex->info.format,
                    src_tex->info.format
                );
                return {};
            }

            color_formats.push_back(src_tex->info.format);

            // Validate size
            if (src_tex->info.width != info.width || src_tex->info.height != info.height) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} size {}x{} does not match framebuffer size {}x{}",
                    src_tex_h,
                    fmt::styled_param(i),
                    fmt::styled_param(src_tex->info.width),
                    fmt::styled_param(src_tex->info.height),
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height)
                );
                return {};
            }

            if (check_resolve && (resolve_tex->info.width != info.width || resolve_tex->info.height != info.height)) {
                ::logger.error(
                    "Failed to create framebuffer: resolve attachment {} at index {} size {}x{} does not match framebuffer size {}x{}",
                    resolve_tex_h,
                    fmt::styled_param(i),
                    fmt::styled_param(resolve_tex->info.width),
                    fmt::styled_param(resolve_tex->info.height),
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height)
                );
                return {};
            }

            // Validate MSAA
            if (info.sample_count != src_tex->info.sample_count) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} sample count {} mismatch with framebuffer sample count {}",
                    src_tex_h,
                    fmt::styled_param(i),
                    fmt::styled_param(src_tex->info.sample_count),
                    fmt::styled_param(info.sample_count)
                );
                return {};
            }

            if (info.sample_count == 1 && ca.store == store_op::resolve) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment at index {} store op is resolve but msaa is single-sampled",
                    fmt::styled_param(i)
                );
                return {};
            }

            if (check_resolve && resolve_tex->info.sample_count != 1) {
                ::logger.error(
                    "Failed to create framebuffer: resolve attachment {} at index {} must be single-sampled",
                    resolve_tex_h,
                    fmt::styled_param(i)
                );
                return {};
            }

            // Validate usage
            if (!src_tex->info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} is not marked as a render target",
                    src_tex_h,
                    fmt::styled_param(i)
                );
                return {};
            }

            if (need_resolve && check_resolve) {
                if (!src_tex->info.usage.has_flag(texture_usage::resolve_source)) {
                    ::logger.error(
                        "Failed to create framebuffer: color attachment {} at index {} is not marked as a resolve source",
                        src_tex_h,
                        fmt::styled_param(i)
                    );
                    return {};
                }

                if (!resolve_tex->info.usage.has_flag(texture_usage::resolve_destination)) {
                    ::logger.error(
                        "Failed to create framebuffer: resolve attachment {} at index {} is not marked as a resolve destination",
                        resolve_tex_h,
                        fmt::styled_param(i)
                    );
                    return {};
                }
            }

            color_textures.push_back(src_tex);
        }


        // ----------------------------------------------------------------
        // Validate depth/stencil attachment
        // ----------------------------------------------------------------
        gl_texture*             ds_tex = nullptr;
        gl_depth_stencil_format ds_format_info{};
        pixel_format            ds_format = pixel_format::none;

        const auto& dsa = info.depth_stencil_attachment;
        auto        ds_tex_h = dsa.target;
        bool        resolve_requested = dsa.depth_store == store_op::resolve || dsa.stencil_store == store_op::resolve;

        if (ds_tex_h.valid()) {
            ds_tex = m_resources.find(ds_tex_h);
            if (!ds_tex) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment {} not found", ds_tex_h);
                return {};
            }

            auto  ds_resolve_tex_h = dsa.resolve_target;
            auto* ds_resolve_tex = m_resources.find(ds_resolve_tex_h);
            bool  check_resolve = false;
            bool  need_resolve = info.sample_count > 1 && resolve_requested;

            if (ds_resolve_tex_h.valid()) {
                if (ds_resolve_tex) {
                    if (resolve_requested) {
                        check_resolve = true;
                    } else {
                        ::logger.warning(
                            "Framebuffer creation: depth/stencil resolve attachment {} specified but no store op is resolve, ignored",
                            ds_resolve_tex_h
                        );
                    }
                } else {
                    if (resolve_requested) {
                        ::logger.error(
                            "Failed to create framebuffer: depth/stencil resolve attachment {} not found",
                            ds_resolve_tex_h
                        );
                        return {};
                    } else {
                        ::logger.warning(
                            "Framebuffer creation: depth/stencil resolve attachment {} not found, ignored (no store op is resolve)",
                            ds_resolve_tex_h
                        );
                    }
                }
            } else if (resolve_requested) {
                ::logger.error("Failed to create framebuffer: depth/stencil store op is resolve but no resolve attachment specified");
                return {};
            }

            // Validate formats
            ds_format_info = to_depth_stencil_format(ds_tex->info.format);
            if (!ds_format_info.is_depth_stencil_format) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil attachment {} has unsupported format {}",
                    ds_tex_h,
                    ds_tex->info.format
                );
                return {};
            }

            if (check_resolve && !to_depth_stencil_format(ds_resolve_tex->info.format).is_depth_stencil_format) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil resolve attachment {} has unsupported format {}",
                    ds_resolve_tex_h,
                    ds_resolve_tex->info.format
                );
                return {};
            }

            if (check_resolve && ds_tex->info.format != ds_resolve_tex->info.format) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil resolve attachment {} format {} does not match attachment format {}",
                    ds_resolve_tex_h,
                    ds_resolve_tex->info.format,
                    ds_tex->info.format
                );
                return {};
            }

            ds_format = ds_tex->info.format;

            // Validate size
            if (ds_tex->info.width != info.width || ds_tex->info.height != info.height) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil attachment {} size {}x{} does not match framebuffer size {}x{}",
                    ds_tex_h,
                    fmt::styled_param(ds_tex->info.width),
                    fmt::styled_param(ds_tex->info.height),
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height)
                );
                return {};
            }

            if (check_resolve && (ds_resolve_tex->info.width != info.width || ds_resolve_tex->info.height != info.height)) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil resolve attachment {} size {}x{} does not match framebuffer size {}x{}",
                    ds_resolve_tex_h,
                    fmt::styled_param(ds_resolve_tex->info.width),
                    fmt::styled_param(ds_resolve_tex->info.height),
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height)
                );
                return {};
            }

            // Validate MSAA
            if (info.sample_count != ds_tex->info.sample_count) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil attachment {} sample count {} mismatch with framebuffer sample count {}",
                    ds_tex_h,
                    fmt::styled_param(ds_tex->info.sample_count),
                    fmt::styled_param(info.sample_count)
                );
                return {};
            }

            if (info.sample_count == 1 && (dsa.depth_store == store_op::resolve || dsa.stencil_store == store_op::resolve)) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil attachment store op is resolve but msaa is single-sampled"
                );
                return {};
            }

            if (check_resolve && ds_resolve_tex->info.sample_count != 1) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil resolve attachment {} must be single-sampled",
                    ds_resolve_tex_h
                );
                return {};
            }

            // Validate usage
            if (!ds_tex->info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil attachment {} is not marked as a render target",
                    ds_tex_h
                );
                return {};
            }

            if (need_resolve && check_resolve) {
                if (!ds_tex->info.usage.has_flag(texture_usage::resolve_source)) {
                    ::logger.error(
                        "Failed to create framebuffer: depth/stencil attachment {} is not marked as a resolve source",
                        ds_tex_h
                    );
                    return {};
                }

                if (!ds_resolve_tex->info.usage.has_flag(texture_usage::resolve_destination)) {
                    ::logger.error(
                        "Failed to create framebuffer: depth/stencil resolve attachment {} is not marked as a resolve destination",
                        ds_resolve_tex_h
                    );
                    return {};
                }
            }
        } else if (dsa.resolve_target.valid() || resolve_requested) {
            ::logger.warning("Framebuffer creation: depth/stencil resolve settings specified but attachment is disabled, ignored");
        }


        // ----------------------------------------------------------------
        // Create the GL framebuffer object and attach textures
        // ----------------------------------------------------------------
        GLuint fbo;
        GL_CALL(glGenFramebuffers(1, &fbo));

        // Scope for framebuffer deletion if something goes wrong
        auto fbo_owner = core::make_scoped_owner(fbo, [](GLuint id) {
            GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            GL_CALL(glDeleteFramebuffers(1, &id));
        });

        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

        // Attach color textures
        core::fixed_vector<GLenum, k_max_color_attachments> draw_buffers;
        for (uint32 i = 0; i < color_textures.size(); ++i) {
            auto*  tex = color_textures[i];
            GLenum attachment_type = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
            draw_buffers.push_back(attachment_type);

            if (tex->target == GL_RENDERBUFFER) {
                TAV_ASSERT(tex->renderbuffer_obj != 0);
                GL_CALL(glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    attachment_type,
                    GL_RENDERBUFFER,
                    tex->renderbuffer_obj
                ));
            } else {
                TAV_ASSERT(tex->texture_obj != 0);
                GL_CALL(glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    attachment_type,
                    tex->target,
                    tex->texture_obj,
                    0
                ));
            }
        }

        if (draw_buffers.empty()) {
            // Drawing only onto depth/stencil buffer.
            // Framebuffer will be incomplete if the depth/stencil texture is not attached.
            GL_CALL(glDrawBuffer(GL_NONE));
            GL_CALL(glReadBuffer(GL_NONE));
        } else {
            if (static_cast<uint32>(draw_buffers.size()) > m_limits.max_draw_buffers) {
                ::logger.warning(
                    "Framebuffer creation: draw buffer count {} exceeds device limit {}",
                    fmt::styled_param(draw_buffers.size()),
                    fmt::styled_param(m_limits.max_draw_buffers)
                );
            }
            GL_CALL(glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data()));
        }

        // Attach depth/stencil texture
        if (ds_tex) {
            if (ds_tex->target == GL_RENDERBUFFER) {
                TAV_ASSERT(ds_tex->renderbuffer_obj != 0);
                GL_CALL(glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    ds_format_info.depth_stencil_attachment_type,
                    GL_RENDERBUFFER,
                    ds_tex->renderbuffer_obj
                ));
            } else {
                TAV_ASSERT(ds_tex->texture_obj != 0);
                GL_CALL(glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    ds_format_info.depth_stencil_attachment_type,
                    ds_tex->target,
                    ds_tex->texture_obj,
                    0
                ));
            }
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            ::logger.error("Failed to create framebuffer: incomplete (glCheckFramebufferStatus = {})", fmt::styled_param(static_cast<uint32>(status)));
            return {};
        }

        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));


        // ----------------------------------------------------------------
        // Register the framebuffer resource
        // ----------------------------------------------------------------
        auto gl_fb = gl_framebuffer{info, fbo_owner.release(), false, color_formats, ds_format};
        auto h = m_resources.create(gl_framebuffer(gl_fb));
        ::logger.debug("Framebuffer {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_framebuffer(framebuffer_handle framebuffer)
    {
        if (auto* fb = m_resources.find(framebuffer)) {
            GL_CALL(glDeleteFramebuffers(1, &fb->framebuffer_obj));
            m_resources.remove(framebuffer);
            ::logger.debug("Framebuffer {} destroyed", framebuffer);
        } else {
            ::logger.error("Failed to destroy framebuffer {}: not found", framebuffer);
        }
    }

    buffer_handle graphics_device_opengl::create_buffer(const buffer_create_info& info)
    {
        if (info.size == 0) {
            ::logger.error("Buffer creation: ({}) of size 0 is not allowed", info.usage);
            return {};
        }

        GLenum     gl_target = 0;
        GLenum     gl_usage = 0;
        GLbitfield gl_flags = 0;

        switch (info.usage) {
        case buffer_usage::stage:

            switch (info.access) {
            case buffer_access::cpu_to_gpu:
                gl_target = GL_COPY_WRITE_BUFFER;
                gl_usage = GL_STREAM_DRAW;
                gl_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
                break;

            case buffer_access::gpu_only:
                gl_target = GL_COPY_WRITE_BUFFER;
                gl_usage = GL_STREAM_COPY;
                gl_flags = GL_CLIENT_STORAGE_BIT;
                break;

            case buffer_access::gpu_to_cpu:
                gl_target = GL_COPY_READ_BUFFER;
                gl_usage = GL_STREAM_READ;
                gl_flags = GL_MAP_READ_BIT;
                break;

            default:
                TAV_UNREACHABLE();
                break;
            }
            break;

        case buffer_usage::index:

            switch (info.access) {
            case buffer_access::cpu_to_gpu:
                gl_target = GL_ELEMENT_ARRAY_BUFFER;
                gl_usage = GL_DYNAMIC_DRAW;
                gl_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
                break;

            case buffer_access::gpu_only:
                gl_target = GL_ELEMENT_ARRAY_BUFFER;
                gl_usage = GL_STREAM_COPY; // GL_STREAM_COPY, GL_STATIC_COPY, GL_STATIC_DRAW - is also possible
                gl_flags = GL_CLIENT_STORAGE_BIT;
                break;

            case buffer_access::gpu_to_cpu:
                ::logger.error("Failed to create buffer: {} can't be {}", info.usage, info.access);
                return {};
            default:
                TAV_UNREACHABLE();
                break;
            }
            break;

        case buffer_usage::vertex:

            switch (info.access) {
            case buffer_access::cpu_to_gpu:
                gl_target = GL_ARRAY_BUFFER;
                gl_usage = GL_DYNAMIC_DRAW;
                gl_flags |= GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
                break;
            case buffer_access::gpu_only:
                gl_target = GL_ARRAY_BUFFER;
                gl_usage = GL_STREAM_COPY;
                gl_flags = GL_CLIENT_STORAGE_BIT;
                break;
            case buffer_access::gpu_to_cpu:
                ::logger.error("Failed to create buffer: {} can't be {}", info.usage, info.access);
                return {};
            default:
                TAV_UNREACHABLE();
                break;
            }
            break;

        case buffer_usage::constant:

            switch (info.access) {
            case buffer_access::cpu_to_gpu:
                gl_target = GL_UNIFORM_BUFFER;
                gl_usage = GL_DYNAMIC_DRAW;
                gl_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
                break;
            case buffer_access::gpu_only:
                gl_target = GL_UNIFORM_BUFFER;
                gl_usage = GL_STATIC_DRAW;
                gl_flags = GL_CLIENT_STORAGE_BIT;
                break;
            case buffer_access::gpu_to_cpu:
                ::logger.error("Failed to create buffer: {} can't be {}", info.usage, info.access);
                return {};
            default:
                TAV_UNREACHABLE();
                break;
            }
            break;

        case buffer_usage::storage:

            switch (info.access) {
            case buffer_access::cpu_to_gpu:
                gl_target = GL_SHADER_STORAGE_BUFFER;
                gl_usage = GL_DYNAMIC_DRAW;
                gl_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
                break;
            case buffer_access::gpu_only:
                gl_target = GL_SHADER_STORAGE_BUFFER;
                gl_usage = GL_STATIC_DRAW;
                gl_flags = GL_CLIENT_STORAGE_BIT;
                break;
            case buffer_access::gpu_to_cpu:
                gl_target = GL_SHADER_STORAGE_BUFFER;
                gl_usage = GL_DYNAMIC_READ;
                gl_flags = GL_MAP_READ_BIT;
                break;
            default:
                TAV_UNREACHABLE();
                break;
            }
            break;

        default:
            TAV_UNREACHABLE();
            break;
        }

        if (info.usage == buffer_usage::storage) {
            if (info.size > m_limits.max_ssbo_size) {
                ::logger.warning(
                    "Buffer creation: storage (SSBO) buffer size {}bytes exceeds device limit {}bytes",
                    fmt::styled_param(info.size),
                    fmt::styled_param(m_limits.max_ubo_size)
                );
            }
        }

        auto gl_size = static_cast<GLsizeiptr>(info.size);

        GLuint bo;
        GL_CALL(glCreateBuffers(1, &bo));
        GL_CALL(glNamedBufferStorage(bo, gl_size, nullptr, gl_flags));

        auto h = m_resources.create(gl_buffer{info, bo, gl_target, gl_usage, gl_flags});
        ::logger.debug("Buffer ({}) {} created", info.usage, h);
        return h;
    }

    void graphics_device_opengl::destroy_buffer(buffer_handle buffer)
    {
        if (auto* b = m_resources.find(buffer)) {
            GL_CALL(glDeleteBuffers(1, &b->buffer_obj));
            m_resources.remove(buffer);
            ::logger.debug("Buffer ({}) {} destroyed", b->info.usage, buffer);
        } else {
            ::logger.error("Failed to destroy buffer {}: not found", buffer);
        }
    }

    fence_handle graphics_device_opengl::create_fence()
    {
        auto h = m_resources.create(gl_fence{});
        ::logger.debug("Fence {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_fence(fence_handle fence)
    {
        if (auto* f = m_resources.find(fence)) {
            if (f->fence_obj) {
                GL_CALL(glDeleteSync(f->fence_obj));
            }
            m_resources.remove(fence);
            ::logger.debug("Fence {} destroyed", fence);
        } else {
            ::logger.error("Failed to destroy fence {}: not found", fence);
        }
    }

    bool graphics_device_opengl::is_fence_signaled(fence_handle fence)
    {
        auto* f = m_resources.find(fence);
        if (!f) {
            ::logger.error("Failed to get fence status {}: fence not found", fence);
            return false;
        }

        if (!f->fence_obj) {
            return false;
        }

        GLint status = 0;
        GL_CALL(glGetSynciv(f->fence_obj, GL_SYNC_STATUS, sizeof(status), nullptr, &status));
        return status == GL_SIGNALED;
    }

    bool graphics_device_opengl::client_wait_for_fence(fence_handle fence, uint64 timeout_ns)
    {
        auto* f = m_resources.find(fence);
        if (!f) {
            ::logger.error("Failed to wait for fence {}: fence not found", fence);
            return false;
        }

        if (!f->fence_obj) {
            return false;
        }

        GLenum result = 0;
        GL_CALL(result = glClientWaitSync(f->fence_obj, GL_SYNC_FLUSH_COMMANDS_BIT, timeout_ns));
        return result == GL_CONDITION_SATISFIED || result == GL_ALREADY_SIGNALED;
    }

    core::buffer_span<uint8> graphics_device_opengl::map_buffer(buffer_handle buffer, size_t offset, size_t size)
    {
        auto* b = m_resources.find(buffer);
        if (!b) {
            ::logger.error("Failed to map buffer {}: buffer not found", buffer);
            return nullptr;
        }

        if (offset + size > b->info.size) {
            ::logger.error(
                "Failed to map buffer {}: offset {} + size {} exceeds buffer size {}",
                buffer,
                fmt::styled_param(offset),
                fmt::styled_param(size),
                fmt::styled_param(b->info.size)
            );
            return nullptr;
        }

        if (offset > 0 && size == 0) {
            ::logger.error(
                "Failed to map buffer {}: offset {} is set but size {} is not set",
                buffer,
                fmt::styled_param(offset),
                fmt::styled_param(size)
            );
            return nullptr;
        }

        if (!(b->info.access == buffer_access::cpu_to_gpu || b->info.access == buffer_access::gpu_to_cpu)) {
            ::logger.error(
                "Failed to map buffer {}: buffer has invalid access {}",
                buffer,
                b->info.access
            );
            return nullptr;
        }

        if (size == 0) {
            TAV_ASSERT(offset == 0);
            size = b->info.size;
        }

        GLbitfield gl_additional_access = b->info.access == buffer_access::cpu_to_gpu
                                            ? GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT
                                            : 0;
        GLintptr   gl_offset = static_cast<GLintptr>(offset);
        GLsizeiptr gl_size = static_cast<GLsizeiptr>(size);

        void* ptr = nullptr;
        GL_CALL(ptr = glMapNamedBufferRange(b->buffer_obj, gl_offset, gl_size, b->gl_flags | gl_additional_access));

        if (!ptr) {
            ::logger.error("Failed to map buffer {}: glMapBufferRange returned nullptr", buffer);
            return nullptr;
        }

        return core::buffer_span<uint8>(reinterpret_cast<uint8*>(ptr), size);
    }

    void graphics_device_opengl::unmap_buffer(buffer_handle buffer)
    {
        auto* b = m_resources.find(buffer);
        if (!b) {
            ::logger.error("Failed to unmap buffer {}: buffer not found", buffer);
            return;
        }

        GL_CALL(glUnmapNamedBuffer(b->buffer_obj));
    }

    device_resources_opengl* graphics_device_opengl::get_resources()
    {
        return &m_resources;
    }

} // namespace tavros::renderer::rhi
