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
        static_assert(sizeof(uint64) == sizeof(HandleT));

        auto& pool = res.get_pool<tavros::renderer::rhi::device_resources_opengl::object_type_of<HandleT>>();

        tavros::core::vector<HandleT> handles;
        handles.reserve(pool.size());
        pool.for_each([&](auto h, auto&) {
            handles.push_back(HandleT(h.id));
        });

        for (auto h : handles) {
            deleter(h);
        }
    }

} // namespace

namespace tavros::renderer::rhi
{

    graphics_device_opengl::graphics_device_opengl()
        : graphics_device()
        , m_resources(&m_internal_allocator)
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
        m_limits.max_color_attachmants = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &value);
        m_limits.max_draw_buffers = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &value);
        m_limits.max_renderbuffer_size = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &value);
        m_limits.max_framebuffer_width = static_cast<uint32>(value);

        glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &value);
        m_limits.max_framebuffer_height = static_cast<uint32>(value);
    }

    void graphics_device_opengl::destroy()
    {
        destroy_for<sampler_handle>(m_resources, [this](auto h) { destroy_sampler(h); });

        destroy_for<texture_handle>(m_resources, [this](auto h) { destroy_texture(h); });

        destroy_for<pipeline_handle>(m_resources, [this](auto h) { destroy_pipeline(h); });

        destroy_for<framebuffer_handle>(m_resources, [this](auto h) {
            if (auto* fb = m_resources.try_get(framebuffer_handle(h))) {
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

        destroy_for<geometry_handle>(m_resources, [this](auto h) { destroy_geometry(h); });

        destroy_for<shader_binding_handle>(m_resources, [this](auto h) { destroy_shader_binding(h); });

        destroy_for<shader_handle>(m_resources, [this](auto h) { destroy_shader(h); });

        destroy_for<render_pass_handle>(m_resources, [this](auto h) { destroy_render_pass(h); });

        // Should be removed in last turn because swapchain owns the OpenGL context
        destroy_for<frame_composer_handle>(m_resources, [this](auto h) { destroy_frame_composer(h); });
    }

    frame_composer_handle graphics_device_opengl::create_frame_composer(const frame_composer_create_info& info)
    {
        // Check if frame composer with native handle already created
        bool has_native_handle = false;

        auto& pool = m_resources.get_pool<gl_composer>();
        pool.for_each([&](auto h, auto& v) { if (info.native_handle == v.info.native_handle) { has_native_handle = true; } });
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
        init_gl_debug();

        init_limits();

        GL_CALL(glEnable(GL_PROGRAM_POINT_SIZE));

        frame_composer_handle handle = m_resources.create(gl_composer{info, std::move(composer)});
        ::logger.debug("Frame composer {} created", handle);
        return handle;
    }

    void graphics_device_opengl::destroy_frame_composer(frame_composer_handle composer)
    {
        if (auto* fc = m_resources.try_get(composer)) {
            m_resources.remove(composer);
            ::logger.debug("Frame composer {} destroyed", composer);
        } else {
            ::logger.error("Failed to destroy frame composer {}: not found", composer);
        }
    }

    frame_composer* graphics_device_opengl::get_frame_composer_ptr(frame_composer_handle composer)
    {
        if (auto* fc = m_resources.try_get(composer)) {
            return fc->composer_ptr.get();
        } else {
            ::logger.error("Failed to get frame composer {}: not found", composer);
            return nullptr;
        }
    }

    shader_handle graphics_device_opengl::create_shader(const shader_create_info& info)
    {
        if (info.entry_point != "main") {
            ::logger.error(
                "Failed to create shader: not supported entry point name {} only 'main' entry point is supported",
                fmt::styled_name(info.entry_point)
            );
            return shader_handle();
        }

        auto shader_obj = compile_shader(info.source_code, to_gl_shader_stage(info.stage));
        if (shader_obj == 0) {
            ::logger.error("Failed to create shader: compilation failed");
            return shader_handle();
        }

        auto h = m_resources.create(gl_shader{info, shader_obj});
        ::logger.debug("Shader ({}) {} created", info.stage, h);
        return h;
    }

    void graphics_device_opengl::destroy_shader(shader_handle shader)
    {
        if (auto* s = m_resources.try_get(shader)) {
            GL_CALL(glDeleteShader(s->shader_obj));
            s->shader_obj = 0;
            m_resources.remove(shader);
            ::logger.debug("Shader ({}) {} destroyed", s->info.stage, shader);
        } else {
            ::logger.error("Failed to destroy shader {}: not found", shader);
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
        if (auto* s = m_resources.try_get(sampler)) {
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

            if (info.array_layers != 1) {
                ::logger.error("Failed to create `texture_2d`: array layers must be 1");
                return {};
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

            uint32 max_mip = info.mip_levels > 1 ? math::mip_levels(info.width, info.height, info.depth) : 1;

            GLenum gl_target = 0;
            switch (info.type) {
            case texture_type::texture_2d:
                if (info.sample_count > 1) {
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
                break;

            case GL_TEXTURE_3D:
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
                break;

            case GL_TEXTURE_CUBE_MAP:
                // Cubemap, allocate memory for all 6 faces
                for (int32 i = 0; i < 6; ++i) {
                    GL_CALL(glTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
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

            default:
                TAV_UNREACHABLE();
                break;
            }

            GL_CALL(glBindTexture(gl_target, 0));

            auto h = m_resources.create(gl_texture{info, tex, gl_target, 0, max_mip});
            ::logger.debug("Texture ({}) {} created", info.type, h);
            return h;
        }
    }

    void graphics_device_opengl::destroy_texture(texture_handle texture)
    {
        if (auto* tex = m_resources.try_get(texture)) {
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
        if (info.shaders.size() != 2) {
            ::logger.error("Failed to create pipeline: exactly 2 shaders required (vertex and fragment)");
            return {};
        }

        gl_shader* vs = nullptr;
        gl_shader* fs = nullptr;
        for (size_t i = 0; i < info.shaders.size(); ++i) {
            auto* s = m_resources.try_get(info.shaders[i]);
            if (!s) {
                ::logger.error("Failed to create pipeline: shader {} not found", info.shaders[i]);
                return {};
            }

            if (s->info.stage == shader_stage::vertex) {
                if (vs) {
                    ::logger.error("Failed to create pipeline: multiple vertex shaders provided");
                    return {};
                }
                vs = s;
            } else if (s->info.stage == shader_stage::fragment) {
                if (fs) {
                    ::logger.error("Failed to create pipeline: multiple fragment shaders provided");
                    return {};
                }
                fs = s;
            } else {
                ::logger.error("Failed to create pipeline: unsupported shader stage");
                return {};
            }
        }

        if (!vs) {
            ::logger.error("Failed to create pipeline: missing vertex shader");
            return {};
        }
        if (!fs) {
            ::logger.error("Failed to create pipeline: missing fragment shader");
            return {};
        }

        GLuint gl_program = link_program(vs->shader_obj, fs->shader_obj);

        // Validate program
        if (gl_program == 0) {
            ::logger.error("Failed to create pipeline: failed to link program");
            return {};
        }

        auto program_owner = core::make_scoped_owner(gl_program, [](GLuint id) {
            GL_CALL(glDeleteProgram(id));
        });


        // Validate attributes

        // Map attributes to location index, for fast search
        tavros::core::unordered_map<uint32, vertex_attribute> mapped_attributes;
        for (const auto& attr : info.attributes) {
            auto it = mapped_attributes.find(attr.location);
            if (it != mapped_attributes.end()) {
                ::logger.error("Failed to create pipeline: attribute location {} is used multiple times", fmt::styled_param(attr.location));
                return {};
            }
            mapped_attributes[attr.location] = attr;
        }

        // Get number of attributes in compiled shader
        GLint gl_prog_attrib_count;
        GL_CALL(glGetProgramiv(gl_program, GL_ACTIVE_ATTRIBUTES, &gl_prog_attrib_count));

        // And validate each attribute
        size_t total_gl_attributes = 0;
        for (GLint i = 0; i < gl_prog_attrib_count; ++i) {
            GLchar attrib_name[256] = {0};
            GLint  size;
            GLenum type;
            GL_CALL(glGetActiveAttrib(gl_program, i, sizeof(attrib_name), nullptr, &size, &type, attrib_name));

            GLint gl_attrib_location = glGetAttribLocation(gl_program, attrib_name);
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
            auto it = mapped_attributes.find(static_cast<uint32>(gl_attrib_location));
            if (it == mapped_attributes.end()) {
                ::logger.error(
                    "Failed to create pipeline: attribute location {} not found in provided attributes",
                    fmt::styled_param(gl_attrib_location)
                );
                return {};
            }

            // Check attribute type and format
            if (it->second.type != shader_type.type || it->second.format != shader_type.format) {
                ::logger.error(
                    "Failed to create pipeline: attribute {} at location {} has mismatched type/format. "
                    "Shader type/format = {}/{}, expected type/format = {}/{}",
                    fmt::styled_text(attrib_name),
                    fmt::styled_param(gl_attrib_location),
                    shader_type.type,
                    shader_type.format,
                    it->second.type,
                    it->second.format
                );
                return {};
            }

            total_gl_attributes++;
        }

        if (total_gl_attributes != info.attributes.size()) {
            ::logger.error(
                "Failed to create pipeline: mismach attributes size. Provided {}, expected {}",
                fmt::styled_param(info.attributes.size()),
                fmt::styled_param(total_gl_attributes)
            );
            return {};
        }


        // create pipeline
        auto h = m_resources.create(gl_pipeline{info, program_owner.release()});
        ::logger.debug("Pipeline {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_pipeline(pipeline_handle handle)
    {
        if (auto* p = m_resources.try_get(handle)) {
            GL_CALL(glDeleteProgram(p->program_obj));
            m_resources.remove(handle);
            ::logger.debug("Pipeline {} destroyed", handle);
        } else {
            ::logger.error("Failed to destroy pipeline {}: not found", handle);
        }
    }

    framebuffer_handle graphics_device_opengl::create_framebuffer(const framebuffer_create_info& info)
    {
        if (info.width > m_limits.max_framebuffer_width || info.height > m_limits.max_framebuffer_height) {
            ::logger.warning(
                "Framebuffer creation: framebuffer size {}x{} exceeds device limit {}x{}",
                fmt::styled_param(info.width),
                fmt::styled_param(info.height),
                fmt::styled_param(m_limits.max_framebuffer_width),
                fmt::styled_param(m_limits.max_framebuffer_height)
            );
        }

        if (static_cast<uint32>(info.color_attachments.size()) > m_limits.max_color_attachmants) {
            ::logger.warning(
                "Framebuffer creation: color attachment size {} exceeds device limit {}",
                fmt::styled_param(info.color_attachments.size()),
                fmt::styled_param(m_limits.max_color_attachmants)
            );
        }

        if (!info.has_depth_stencil_attachment && info.depth_stencil_attachment.valid()) {
            ::logger.warning("Framebuffer creation: Depth/stencil attachment {} is provided but not enabled", info.depth_stencil_attachment);
        }

        // Framebuffer will be incomplete if the depth/stencil texture is not attached and
        // there are no color attachments
        if (info.color_attachments.size() == 0 && !info.has_depth_stencil_attachment) {
            ::logger.error("Failed to create framebuffer: no color or depth/stencil attachments provided");
            return {};
        }

        // Get and validate color attachments
        core::static_vector<gl_texture*, k_max_color_attachments> color_attachments;
        for (size_t i = 0; i < info.color_attachments.size(); ++i) {
            auto tex_h = info.color_attachments[i];

            auto* tex = m_resources.try_get(tex_h);
            if (!tex) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} not found",
                    tex_h,
                    fmt::styled_param(i)
                );
                return {};
            }

            // Validate formats
            if (!is_color_format(tex->info.format)) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} has unsupported format {}",
                    tex_h,
                    fmt::styled_param(i),
                    tex->info.format
                );
                return {};
            }

            // Validate size
            if (tex->info.width != info.width || tex->info.height != info.height) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} size {}x{} does not match framebuffer size {}x{}",
                    tex_h,
                    fmt::styled_param(i),
                    fmt::styled_param(tex->info.width),
                    fmt::styled_param(tex->info.height),
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height)
                );
                return {};
            }

            // Validate MSAA
            if (info.sample_count != tex->info.sample_count) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} sample count {} mismatch with framebuffer sample count {}",
                    tex_h,
                    fmt::styled_param(i),
                    fmt::styled_param(tex->info.sample_count),
                    fmt::styled_param(info.sample_count)
                );
                return {};
            }

            // All the attachments must be used as color attachments
            if (!tex->info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error(
                    "Failed to create framebuffer: color attachment {} at index {} is not marked as a render target",
                    tex_h,
                    fmt::styled_param(i)
                );
                return {};
            }

            color_attachments.push_back(tex);
        }


        // Get depth/stencil attachment texture if provided
        gl_texture*             ds_attachment = nullptr;
        gl_depth_stencil_format ds_attachment_format_info;
        if (info.has_depth_stencil_attachment) {
            auto ds_h = info.depth_stencil_attachment;

            auto* tex = m_resources.try_get(ds_h);
            if (!tex) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment {} not found", ds_h);
                return {};
            }

            // Validate formats
            ds_attachment_format_info = to_depth_stencil_fromat(tex->info.format);
            if (!ds_attachment_format_info.is_depth_stencil_format) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment {} has unsupported format {}", ds_h, tex->info.format);
                return {};
            }

            // Validate size
            if (tex->info.width != info.width || tex->info.height != info.height) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil attachment {} size {}x{} does not match framebuffer size {}x{}",
                    ds_h,
                    fmt::styled_param(tex->info.width),
                    fmt::styled_param(tex->info.height),
                    fmt::styled_param(info.width),
                    fmt::styled_param(info.height)
                );
                return {};
            }

            // Validate MSAA
            if (info.sample_count != tex->info.sample_count) {
                ::logger.error(
                    "Failed to create framebuffer: depth/stencil attachment {} sample count {} mismatch with framebuffer sample count {}",
                    ds_h,
                    fmt::styled_param(tex->info.sample_count),
                    fmt::styled_param(info.sample_count)
                );
                return {};
            }

            // All the attachments must be used as color attachments
            if (!tex->info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment {} is not marked as a render target", ds_h);
                return {};
            }

            ds_attachment = tex;
        }


        GLuint fbo;
        GL_CALL(glGenFramebuffers(1, &fbo));

        // Scope for framebuffer deletion if something goes wrong
        auto fbo_owner = core::make_scoped_owner(fbo, [](GLuint id) {
            GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            GL_CALL(glDeleteFramebuffers(1, &id));
        });

        // Bind framebuffer and attach textures
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

        // Attach color textures to the framebuffer
        core::static_vector<GLenum, k_max_color_attachments> draw_buffers;
        for (uint32 i = 0; i < color_attachments.size(); ++i) {
            auto*  tex = color_attachments[i];
            GLenum attachment_type = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
            draw_buffers.push_back(attachment_type);

            if (tex->target == GL_RENDERBUFFER) {
                // Attach renderbuffer
                TAV_ASSERT(tex->renderbuffer_obj != 0);

                GL_CALL(glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    attachment_type,
                    GL_RENDERBUFFER,
                    tex->renderbuffer_obj
                ));
            } else {
                // Attach regular texture
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

        // Enable color attachments
        if (draw_buffers.empty()) {
            // Drawing only onto depth/stencil buffer
            // Framebuffer will be incomplete if the depth/stencil texture is not attached
            GL_CALL(glDrawBuffer(GL_NONE));
            GL_CALL(glReadBuffer(GL_NONE));
        } else {
            if (static_cast<uint32>(draw_buffers.size()) > m_limits.max_draw_buffers) {
                ::logger.warning(
                    "Framebuffer creation: number of draw buffers {} exceeds device limit {}",
                    fmt::styled_param(draw_buffers.size()),
                    fmt::styled_param(m_limits.max_draw_buffers)
                );
            }
            // Drawing onto color attachments
            GL_CALL(glDrawBuffers(draw_buffers.size(), draw_buffers.data()));
        }

        // Enable depth/stencil attachment
        if (ds_attachment) {
            if (ds_attachment->target == GL_RENDERBUFFER) {
                // Attach renderbuffer
                TAV_ASSERT(ds_attachment->renderbuffer_obj != 0);

                GL_CALL(glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    ds_attachment_format_info.depth_stencil_attachment_type,
                    GL_RENDERBUFFER,
                    ds_attachment->renderbuffer_obj
                ));
            } else {
                // Attach regular texture
                TAV_ASSERT(ds_attachment->texture_obj != 0);

                GL_CALL(glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    ds_attachment_format_info.depth_stencil_attachment_type,
                    ds_attachment->target,
                    ds_attachment->texture_obj,
                    0
                ));
            }
        }

        // Validate framebuffer
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            ::logger.error("Failed to create framebuffer: framebuffer is not complete");
            return {};
        }

        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        auto h = m_resources.create(gl_framebuffer{info, fbo_owner.release(), false, pixel_format::none, pixel_format::none});
        ::logger.debug("Framebuffer {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_framebuffer(framebuffer_handle framebuffer)
    {
        if (auto* fb = m_resources.try_get(framebuffer)) {
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

        GLenum gl_target = 0;
        GLenum gl_usage = 0;

        switch (info.usage) {
        case buffer_usage::stage:

            switch (info.access) {
            case buffer_access::cpu_to_gpu:
                gl_target = GL_COPY_WRITE_BUFFER;
                gl_usage = GL_STREAM_DRAW;
                break;

            case buffer_access::gpu_only:
                gl_target = GL_COPY_WRITE_BUFFER;
                gl_usage = GL_STREAM_COPY;
                break;

            case buffer_access::gpu_to_cpu:
                gl_target = GL_COPY_READ_BUFFER;
                gl_usage = GL_STREAM_READ;
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
                break;

            case buffer_access::gpu_only:
                gl_target = GL_ELEMENT_ARRAY_BUFFER;
                gl_usage = GL_STREAM_COPY; // GL_STREAM_COPY, GL_STATIC_COPY, GL_STATIC_DRAW - is also possible
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
                break;
            case buffer_access::gpu_only:
                gl_target = GL_ARRAY_BUFFER;
                gl_usage = GL_STREAM_COPY;
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
                break;
            case buffer_access::gpu_only:
                gl_target = GL_UNIFORM_BUFFER;
                gl_usage = GL_STATIC_DRAW;
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
                break;
            case buffer_access::gpu_only:
                gl_target = GL_SHADER_STORAGE_BUFFER;
                gl_usage = GL_STATIC_DRAW;
                break;
            case buffer_access::gpu_to_cpu:
                gl_target = GL_SHADER_STORAGE_BUFFER;
                gl_usage = GL_DYNAMIC_READ;
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

        if (info.usage == buffer_usage::constant) {
            if (info.size > m_limits.max_ubo_size) {
                ::logger.warning(
                    "Buffer creation: constant (UBO) buffer size {}bytes exceeds device limit {}bytes",
                    fmt::styled_param(info.size),
                    fmt::styled_param(m_limits.max_ubo_size)
                );
            }
        } else if (info.usage == buffer_usage::storage) {
            if (info.size > m_limits.max_ssbo_size) {
                ::logger.warning(
                    "Buffer creation: storage (SSBO) buffer size {}bytes exceeds device limit {}bytes",
                    fmt::styled_param(info.size),
                    fmt::styled_param(m_limits.max_ubo_size)
                );
            }
        }

        GLuint bo;
        GL_CALL(glGenBuffers(1, &bo));

        auto gl_size = static_cast<GLsizeiptr>(info.size);

        GL_CALL(glBindBuffer(gl_target, bo));
        GL_CALL(glBufferData(gl_target, gl_size, nullptr, gl_usage));
        GL_CALL(glBindBuffer(gl_target, 0));

        auto h = m_resources.create(gl_buffer{info, bo, gl_target, gl_usage});
        ::logger.debug("Buffer ({}) {} created", info.usage, h);
        return h;
    }

    void graphics_device_opengl::destroy_buffer(buffer_handle buffer)
    {
        if (auto* b = m_resources.try_get(buffer)) {
            GL_CALL(glDeleteBuffers(1, &b->buffer_obj));
            m_resources.remove(buffer);
            ::logger.debug("Buffer ({}) {} destroyed", b->info.usage, buffer);
        } else {
            ::logger.error("Failed to destroy buffer {}: not found", buffer);
        }
    }

    geometry_handle graphics_device_opengl::create_geometry(const geometry_create_info& info)
    {
        // Check vertex buffers
        if (info.vertex_buffer_layouts.size() == 0) {
            ::logger.error("Failed to create geometry: vertex buffer layout list is empty");
            return {};
        }

        // Gather vertex buffers
        core::static_vector<gl_buffer*, k_max_vertex_buffers> vert_bufs;
        for (size_t i = 0; i < info.vertex_buffer_layouts.size(); ++i) {
            auto& layout = info.vertex_buffer_layouts[i];

            auto* vb = m_resources.try_get(layout.buffer);
            if (!vb) {
                ::logger.error(
                    "Failed to create geometry: vertex buffer {} at layout index {} not found",
                    layout.buffer,
                    fmt::styled_param(i)
                );
                return {};
            }

            if (vb->info.usage != buffer_usage::vertex) {
                ::logger.error(
                    "Failed to create geometry: buffer {} at layout index {} has invalid usage (expected 'vertex', got '{}')",
                    layout.buffer,
                    fmt::styled_param(i),
                    vb->info.usage
                );
                return {};
            }

            vert_bufs.push_back(vb);
        }

        // Gather index buffer
        gl_buffer* ind_buf = nullptr;
        if (info.has_index_buffer) {
            auto* ib = m_resources.try_get(info.index_buffer);
            if (!ib) {
                ::logger.error("Failed to create geometry: index buffer {} not found", info.index_buffer);
                return {};
            }

            if (ib->info.usage != buffer_usage::index) {
                ::logger.error(
                    "Failed to create geometry: buffer {} has invalid usage (expected 'index', got '{}')",
                    info.index_buffer,
                    ib->info.usage
                );
                return {};
            }

            ind_buf = ib;
        } else if (info.index_buffer.valid()) {
            ::logger.warning("Geometry creation: index buffer {} is provided but not enabled", info.index_buffer);
        }

        // Check attribute bindings
        auto buffer_layouts_size = static_cast<uint32>(info.vertex_buffer_layouts.size());
        for (auto i = 0; i < info.attribute_bindings.size(); ++i) {
            auto buffer_layout_index = info.attribute_bindings[i].vertex_buffer_layout_index;
            if (buffer_layout_index >= buffer_layouts_size) {
                ::logger.error(
                    "Geometry creation failed: attribute binding {} references invalid buffer layout index {} (valid range is 0–{})",
                    fmt::styled_param(i),
                    fmt::styled_param(buffer_layout_index),
                    fmt::styled_param(buffer_layouts_size - 1)
                );
                return {};
            }
        }

        // Create VAO
        GLuint vao;
        GL_CALL(glGenVertexArrays(1, &vao));

        GL_CALL(glBindVertexArray(vao));

        // Setup attribute bindings
        for (auto attrib_i = 0; attrib_i < info.attribute_bindings.size(); ++attrib_i) {
            auto& attrib_bind = info.attribute_bindings[attrib_i];
            auto& attrib = attrib_bind.attribute;
            auto  buf_layout = info.vertex_buffer_layouts[attrib_bind.vertex_buffer_layout_index];
            auto* b = vert_bufs[attrib_bind.vertex_buffer_layout_index];

            // Enable the vertex buffer
            GL_CALL(glBindVertexBuffer(attrib_i, b->buffer_obj, buf_layout.base_offset, buf_layout.stride));

            bool is_flt = attribute_format::f32 == attrib.format || attribute_format::f16 == attrib.format || attribute_format::f64 == attrib.format;
            auto gl_attrib_info = to_gl_attribute_info(attrib.type, attrib.format);
            for (uint32 col = 0; col < gl_attrib_info.cols; ++col) {
                GLuint location = attrib.location + col;
                GLuint offset = attrib_bind.offset + col * gl_attrib_info.rows * gl_attrib_info.size;

                // Enable attribute and set pointer
                GL_CALL(glEnableVertexAttribArray(location));
                if (is_flt) {
                    GL_CALL(glVertexAttribFormat(location, gl_attrib_info.rows, gl_attrib_info.type, attrib.normalize, offset));
                } else {
                    GL_CALL(glVertexAttribIFormat(location, gl_attrib_info.rows, gl_attrib_info.type, offset));
                }
                GL_CALL(glVertexAttribBinding(location, attrib_i));
            }
            GL_CALL(glVertexBindingDivisor(attrib_i, attrib_bind.instance_divisor));
        }

        // Bind index buffer if present
        if (ind_buf) {
            GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ind_buf->buffer_obj));
        }

        GL_CALL(glBindVertexArray(0));

        auto h = m_resources.create(gl_geometry{info, vao});
        ::logger.debug("Geometry {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_geometry(geometry_handle geometry)
    {
        if (auto* g = m_resources.try_get(geometry)) {
            GL_CALL(glDeleteVertexArrays(1, &g->vao_obj));
            m_resources.remove(geometry);
            ::logger.debug("Geometry {} destroyed", geometry);
        } else {
            ::logger.error("Failed to destroy geometry {}: not found", geometry);
        }
    }

    render_pass_handle graphics_device_opengl::create_render_pass(const render_pass_create_info& info)
    {
        // Validate attachments
        for (const auto& attachment : info.color_attachments) {
            if (!is_color_format(attachment.format)) {
                ::logger.error("Failed to create render pass: invalid color attachment format");
                return {};
            }
        }

        // Validate depth/stencil attachment
        auto depth_stencil_is_none = info.depth_stencil_attachment.format == pixel_format::none;
        if (!depth_stencil_is_none) {
            auto f = to_depth_stencil_fromat(info.depth_stencil_attachment.format);
            if (!f.is_depth_stencil_format) {
                ::logger.error("Failed to create render pass: invalid depth/stencil attachment format");
                return {};
            }
        }

        if (info.color_attachments.size() == 0 && depth_stencil_is_none) {
            ::logger.error("Failed to create render pass: no attachments specified");
            return {};
        }

        // Validate resolve attachments
        uint32 need_resolve_textures_number = 0;
        for (auto i = 0; i < info.color_attachments.size(); ++i) {
            auto& attachment = info.color_attachments[i];
            if (store_op::resolve == attachment.store) {
                auto* resolve_tex = m_resources.try_get(attachment.resolve_target);
                if (!resolve_tex) {
                    ::logger.error(
                        "Failed to create render pass: resolve attachment {} texture {} not found",
                        fmt::styled_param(i),
                        attachment.resolve_target
                    );
                    return {};
                }

                if (resolve_tex->info.format != attachment.format) {
                    ::logger.error(
                        "Failed to create render pass: mismatched resolve attachment {} texture format {} with color attachment format {}",
                        fmt::styled_param(i),
                        fmt::styled_param(resolve_tex->info.format),
                        fmt::styled_param(attachment.format)
                    );
                    return {};
                }
                if (!resolve_tex->info.usage.has_flag(texture_usage::resolve_destination)) {
                    ::logger.error(
                        "Failed to create render pass: resolve attachment {} texture {} must have resolve_destination usage flag",
                        fmt::styled_param(i),
                        attachment.resolve_target
                    );
                    return {};
                }
                if (resolve_tex->info.sample_count != 1) {
                    ::logger.error(
                        "Failed to create render pass: resolve attachment {} texture {} must be single-sampled",
                        fmt::styled_param(i),
                        attachment.resolve_target
                    );
                    return {};
                }
            } else if (attachment.resolve_target.valid()) {
                ::logger.warning(
                    "Render pass creation: resolve attachment {} is provided but its store operation is not set to `resolve`",
                    fmt::styled_param(i)
                );
            }
        }

        auto& ds_attachment = info.depth_stencil_attachment;
        bool  need_depth_resolve = store_op::resolve == ds_attachment.depth_store;
        bool  need_stencil_resolve = store_op::resolve == ds_attachment.stencil_store;
        if (need_depth_resolve || need_stencil_resolve) {
            auto* resolve_tex = m_resources.try_get(ds_attachment.resolve_target);
            if (!resolve_tex) {
                ::logger.error(
                    "Failed to create render pass: resolve depth/stencil attachment texture {} not found",
                    ds_attachment.resolve_target
                );
                return {};
            }

            if (resolve_tex->info.format != ds_attachment.format) {
                ::logger.error(
                    "Failed to create render pass: mismatched resolve depth/stencil attachment texture format {} with color attachment format {}",
                    fmt::styled_param(resolve_tex->info.format),
                    fmt::styled_param(ds_attachment.format)
                );
                return {};
            }

            if (!resolve_tex->info.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error(
                    "Failed to create render pass: resolve depth/stencil attachment texture {} must have resolve_destination usage flag",
                    ds_attachment.resolve_target
                );
                return {};
            }

            if (resolve_tex->info.sample_count != 1) {
                ::logger.error(
                    "Failed to create render pass: resolve depth/stencil attachment texture {} must be single-sampled",
                    ds_attachment.resolve_target
                );
                return {};
            }

            auto ds_info = to_gl_wnd_fb_info(resolve_tex->info.format);
            if (ds_info.depth_bits == 0 && need_depth_resolve) {
                // No depth component, but required depth resolve
                ::logger.error(
                    "Failed to create render pass: no depth component {} but `resolve` required",
                    ds_attachment.format
                );
                return {};
            }

            if (ds_info.stencil_bits == 0 && need_stencil_resolve) {
                // No dtencil component, but required stencil resolve
                ::logger.error(
                    "Failed to create render pass: no stencil component {} but `resolve` required",
                    ds_attachment.format
                );
                return {};
            }
        } else if (ds_attachment.resolve_target.valid()) {
            ::logger.warning("Render pass creation: resolve depth/stencil attachment is provided but its store operation is not set to `resolve`");
        }

        auto h = m_resources.create(gl_render_pass{info});
        ::logger.debug("Render pass {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_render_pass(render_pass_handle render_pass)
    {
        if (auto* rp = m_resources.try_get(render_pass)) {
            m_resources.remove(render_pass);
            ::logger.debug("Render pass {} destroyed", render_pass);
        } else {
            ::logger.error("Failed to destroy render pass {}: not found", render_pass);
        }
    }

    shader_binding_handle graphics_device_opengl::create_shader_binding(const shader_binding_create_info& info)
    {
        // Validate texture and sampler bindings
        for (size_t i = 0; i < info.texture_bindings.size(); ++i) {
            auto& binding = info.texture_bindings[i];

            auto* tex = m_resources.try_get(binding.texture);
            if (!tex) {
                ::logger.error(
                    "Failed to create shader binding: texture {} binding {} not found",
                    binding.texture,
                    fmt::styled_param(i)
                );
                return {};
            }

            auto* smp = m_resources.try_get(binding.sampler);
            if (!smp) {
                ::logger.error(
                    "Failed to create shader binding: sampler {} binding {} not found",
                    binding.sampler,
                    fmt::styled_param(i)
                );
                return {};
            }
        }

        // Validate buffers
        for (size_t i = 0; i < info.buffer_bindings.size(); ++i) {
            auto& binding = info.buffer_bindings[i];

            auto* b = m_resources.try_get(binding.buffer);
            if (!b) {
                ::logger.error(
                    "Failed to create shader binding: buffer {} binding {} not found",
                    binding.buffer,
                    fmt::styled_param(i)
                );
                return {};
            }

            if (b->info.usage != buffer_usage::constant && b->info.usage != buffer_usage::storage) {
                ::logger.error(
                    "Failed to create shader binding: buffer {} binding {} has invalid usage (expected `constant` or `storage`, got {})",
                    binding.buffer,
                    fmt::styled_param(i),
                    b->info.usage
                );
                return {};
            }
        }

        auto h = m_resources.create(gl_shader_binding{info});
        ::logger.debug("Shader binding {} created", h);
        return h;
    }

    void graphics_device_opengl::destroy_shader_binding(shader_binding_handle shader_binding)
    {
        if (auto* sb = m_resources.try_get(shader_binding)) {
            m_resources.remove(shader_binding);
            ::logger.debug("Shader binding {} destroyed", shader_binding);
        } else {
            ::logger.error("Failed to destroy shader binding {}: not found", shader_binding);
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
        if (auto* f = m_resources.try_get(fence)) {
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
        auto* f = m_resources.try_get(fence);
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

    bool graphics_device_opengl::wait_for_fence(fence_handle fence, uint64 timeout_ns)
    {
        auto* f = m_resources.try_get(fence);
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
        auto* b = m_resources.try_get(buffer);
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
                "Failed to map buffer {}: buffer has invalid access (expected cpu_to_gpu, got {})",
                buffer,
                b->info.access
            );
            return nullptr;
        }

        if (size == 0) {
            TAV_ASSERT(offset == 0);
            size = b->info.size;
        }

        GLenum access = b->info.access == buffer_access::cpu_to_gpu
                          ? GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT
                          : GL_MAP_READ_BIT;
        GLenum target = b->gl_target;

        GL_CALL(glBindBuffer(target, b->buffer_obj));
        void* ptr = glMapBufferRange(target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), access);
        GL_CALL(glBindBuffer(target, 0));

        if (!ptr) {
            ::logger.error("Failed to map buffer {}: glMapBufferRange returned nullptr", buffer);
            return nullptr;
        }

        return core::buffer_span<uint8>(reinterpret_cast<uint8*>(ptr), size);
    }

    void graphics_device_opengl::unmap_buffer(buffer_handle buffer)
    {
        auto* b = m_resources.try_get(buffer);
        if (!b) {
            ::logger.error("Failed to unmap buffer {}: buffer not found", buffer);
            return;
        }

        auto target = b->gl_target;
        GL_CALL(glBindBuffer(target, b->buffer_obj));
        GL_CALL(glUnmapBuffer(target));
        GL_CALL(glBindBuffer(target, 0));
    }

    device_resources_opengl* graphics_device_opengl::get_resources()
    {
        return &m_resources;
    }

} // namespace tavros::renderer::rhi
