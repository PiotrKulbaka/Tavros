#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

#include <tavros/renderer/internal/opengl/frame_composer_opengl.hpp>
#include <tavros/renderer/internal/opengl/type_conversions.hpp>
#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/scoped_owner.hpp>
#include <tavros/core/prelude.hpp>

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
        glShaderSource(shader, 1, &program_text, &text_length);
        glCompileShader(shader);

        // check compile status
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char buffer[4096];
            glGetShaderInfoLog(shader, sizeof(buffer), nullptr, buffer);
            logger.error("glCompileShader() failed:\n%s", buffer);
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
            // l.info("%s", message);
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
        pool.for_each([&](auto h, auto&) {
            handles.push_back(h.id);
        });

        for (auto handle : handles) {
            deleter({handle});
        }
    }

    constexpr bool is_power_of_two(uint32 value)
    {
        return value != 0 && (value & (value - 1)) == 0;
    }

} // namespace

namespace tavros::renderer::rhi
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
            if (auto* fb = m_resources.try_get(h)) {
                if (!fb->is_default) {
                    // We only delete non default framebuffers
                    // Default ones should be deleted elsewhere, in frame_composer
                    destroy_framebuffer(h);
                }
            } else {
                // This shouldn't happen, but just in case, we call a method that will throw an error in the log
                destroy_framebuffer(h);
            }
        });

        destroy_for(m_resources.buffers, [this](buffer_handle h) {
            destroy_buffer(h);
        });

        destroy_for(m_resources.geometries, [this](geometry_handle h) {
            destroy_geometry(h);
        });

        destroy_for(m_resources.shader_bindings, [this](shader_binding_handle h) {
            destroy_shader_binding(h);
        });

        destroy_for(m_resources.shaders, [this](shader_handle h) {
            destroy_shader(h);
        });

        destroy_for(m_resources.render_passes, [this](render_pass_handle h) {
            destroy_render_pass(h);
        });

        //// Should be removed in last turn because swapchain owns the OpenGL context
        destroy_for(m_resources.composers, [this](frame_composer_handle h) {
            destroy_frame_composer(h);
        });
    }

    frame_composer_handle graphics_device_opengl::create_frame_composer(const frame_composer_info& info, void* native_handle)
    {
        // Check if frame composer with native handle already created
        bool has_native_handle = false;
        m_resources.composers.for_each([&](auto h, auto& v) { if (native_handle == v.native_handle) { has_native_handle = true; } });
        if (has_native_handle) {
            ::logger.error("Failed to create frame composer: native handle %p already exists", native_handle);
            return frame_composer_handle::invalid();
        }

        // Create a new frame composer
        auto composer = frame_composer_opengl::create(this, info, native_handle);

        if (!composer) {
            // Detailed info sould be written at frame_composer_opengl
            ::logger.error("Failed to create frame composer");
            return frame_composer_handle::invalid();
        }

        // Initialize debug callback for OpenGL here, because it's not possible to do it in the constructor
        // the first call of frame_composer_opengl::create() will create the context
        init_gl_debug();

        frame_composer_handle handle = {m_resources.create({info, std::move(composer), native_handle})};
        ::logger.debug("Frame composer `%s` created", htos(handle));
        return handle;
    }

    void graphics_device_opengl::destroy_frame_composer(frame_composer_handle composer)
    {
        if (auto* fc = m_resources.try_get(composer)) {
            m_resources.remove(composer);
            ::logger.debug("Frame composer `%s` destroyed", htos(composer));
        } else {
            ::logger.error("Failed to destroy frame composer `%s`: not found", htos(composer));
        }
    }

    frame_composer* graphics_device_opengl::get_frame_composer_ptr(frame_composer_handle composer)
    {
        if (auto* fc = m_resources.try_get(composer)) {
            return fc->composer_ptr.get();
        } else {
            ::logger.error("Failed to get frame composer `%s`: not found", htos(composer));
            return nullptr;
        }
    }

    shader_handle graphics_device_opengl::create_shader(const shader_info& info)
    {
        if (info.entry_point != "main") {
            ::logger.error("Failed to create shader: only 'main' entry point is supported");
            return shader_handle::invalid();
        }

        auto shader_obj = compile_shader(info.source_code, to_gl_shader_stage(info.stage));
        if (shader_obj == 0) {
            ::logger.error("Failed to create shader: compilation failed");
            return shader_handle::invalid();
        }

        auto h = m_resources.create({info, shader_obj});
        ::logger.debug("Shader `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_shader(shader_handle shader)
    {
        if (auto* s = m_resources.try_get(shader)) {
            glDeleteShader(s->shader_obj);
            s->shader_obj = 0;
            m_resources.remove(shader);
            ::logger.debug("Shader `%s` destroyed", htos(shader));
        } else {
            ::logger.error("Failed to destroy shader `%s`: not found", htos(shader));
        }
    }

    sampler_handle graphics_device_opengl::create_sampler(const sampler_info& info)
    {
        GLuint sampler;
        glGenSamplers(1, &sampler);

        auto filter = to_gl_filter(info.filter);

        // Filtering
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, filter.min_filter);
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, filter.mag_filter);

        // Wrapping
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, to_gl_wrap_mode(info.wrap_mode.wrap_s));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, to_gl_wrap_mode(info.wrap_mode.wrap_t));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, to_gl_wrap_mode(info.wrap_mode.wrap_r));

        // LOD parameters
        glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, info.mip_lod_bias);
        glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, info.min_lod);
        glSamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, info.max_lod);

        // Depth compare
        if (info.depth_compare != compare_op::off) {
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, to_gl_compare_func(info.depth_compare));
        } else {
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }

        auto h = m_resources.create({info, sampler});
        ::logger.debug("Sampler `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_sampler(sampler_handle sampler)
    {
        if (auto* s = m_resources.try_get(sampler)) {
            glDeleteSamplers(1, &s->sampler_obj);
            m_resources.remove(sampler);
            ::logger.debug("Sampler `%s` destroyed", htos(sampler));
        } else {
            ::logger.error("Failed to destroy sampler `%s`: not found", htos(sampler));
        }
    }

    texture_handle graphics_device_opengl::create_texture(
        const texture_info& info
    )
    {
        auto gl_pixel_format = to_gl_pixel_format(info.format);
        auto gl_depth_stencil_format = to_depth_stencil_fromat(info.format);

        if (info.type == texture_type::texture_2d) {
            if (info.width == 0 || info.height == 0 || info.depth != 1) {
                ::logger.error("Failed to create `texture_2d`: width and height must be greater than 0, and depth must be 1");
                return texture_handle::invalid();
            }

            if (info.array_layers != 1) {
                ::logger.error("Failed to create `texture_2d`: array layers must be 1");
                return texture_handle::invalid();
            }

            if (info.sample_count == 0 || !is_power_of_two(info.sample_count)) {
                ::logger.error("Failed to create `texture_2d`: sample count must be power of two, and at least 1");
                return texture_handle::invalid();
            }

            if (info.sample_count == 1 && info.usage.has_flag(texture_usage::resolve_source)) {
                ::logger.error("Failed to create `texture_2d`: resolve source textures must have sample count > 1");
                return texture_handle::invalid();
            }

            if (info.sample_count > 1 && info.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error("Failed to create `texture_2d`: resolve destination textures must have sample_count == 1");
                return texture_handle::invalid();
            }

            if (info.sample_count > 1 && info.usage.has_flag(texture_usage::storage)) {
                ::logger.error("Failed to create `texture_2d`: multisample textures cannot be used as storage");
                return texture_handle::invalid();
            }

            if (info.sample_count > 1 && info.usage.has_flag(texture_usage::sampled)) {
                ::logger.error("Failed to create `texture_2d`: multisample textures cannot be used as sampled");
                return texture_handle::invalid();
            }

            if (info.sample_count > 1 && info.mip_levels > 1) {
                ::logger.error("Failed to create `texture_2d`: multisample textures cannot have mip levels");
                return texture_handle::invalid();
            }

            if (info.mip_levels == 0) {
                ::logger.error("Failed to create `texture_2d`: mip levels must be at least 1");
                return texture_handle::invalid();
            }

            // Resolve source texture must be a render target
            if (info.usage.has_flag(texture_usage::resolve_source) && !info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Failed to create texture: resolve source textures must be render targets");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::depth_stencil_target)) {
                // Depth stencil target texture must be a depth stencil format
                if (!gl_depth_stencil_format.is_depth_stencil_format) {
                    ::logger.error("Failed to create texture: depth/stencil target must have depth/stencil format");
                    return texture_handle::invalid();
                }

                // Depth stencil target texture cannot be used as sampled
                if (info.usage.has_flag(texture_usage::sampled)) {
                    ::logger.error("Failed to create texture: depth/stencil target textures cannot be used as sampled");
                    return texture_handle::invalid();
                }
            }

        } else if (info.type == texture_type::texture_3d) {
            if (info.width == 0 || info.height == 0 || info.depth == 0) {
                ::logger.error("Failed to create `texture_3d`: width, height and depth must be greater than 0");
                return texture_handle::invalid();
            }

            if (info.array_layers != 1) {
                ::logger.error("Failed to create `texture_3d`: array layers must be 1");
                return texture_handle::invalid();
            }

            if (info.sample_count != 1) {
                ::logger.error("Failed to create `texture_3d`: sample count must be 1");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Failed to create `texture_3d`: usage flag `render_target` is not allowed");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::depth_stencil_target)) {
                ::logger.error("Failed to create `texture_3d`: usage flags `depth_stencil_target` is not allowed");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::resolve_source)) {
                ::logger.error("Failed to create `texture_3d`: usage flags `resolve_source` is not allowed");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error("Failed to create `texture_3d`: usage flag `resolve_destination` is not allowed");
                return texture_handle::invalid();
            }

        } else if (info.type == texture_type::texture_cube) {
            if (info.width == 0 || info.height == 0 || info.width != info.height || info.depth != 1) {
                ::logger.error("Failed to create `texture_cube`: width and height must be equal and greater than 0, depth must be 1");
                return texture_handle::invalid();
            }

            if (info.array_layers % 6 != 0 || info.array_layers != 6) {
                ::logger.error("Failed to create `texture_cube`: array layers must be a multiple of 6 and at least 6");
                return texture_handle::invalid();
            }

            if (info.sample_count != 1) {
                ::logger.error("Failed to create `texture_cube`: sample count must be 1");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::depth_stencil_target)) {
                ::logger.error("Failed to create `texture_cube`: usage flags `depth_stencil_target` is not allowed");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::resolve_source)) {
                ::logger.error("Failed to create `texture_cube`: usage flags `resolve_source` is not allowed");
                return texture_handle::invalid();
            }

            if (info.usage.has_flag(texture_usage::resolve_destination)) {
                ::logger.error("Failed to create `texture_cube`: usage flag `resolve_destination` is not allowed");
                return texture_handle::invalid();
            }

        } else {
            ::logger.error("Failed to create texture: unknown texture type");
            return texture_handle::invalid();
        }


        GLenum gl_target = 0;
        if (info.type == texture_type::texture_2d) {
            if (info.sample_count > 1) {
                gl_target = GL_TEXTURE_2D_MULTISAMPLE;
            } else {
                gl_target = GL_TEXTURE_2D;
            }
        } else if (info.type == texture_type::texture_3d) {
            gl_target = GL_TEXTURE_3D;
        } else if (info.type == texture_type::texture_cube) {
            gl_target = GL_TEXTURE_CUBE_MAP;
        } else {
            TAV_UNREACHABLE();
        }

        GLuint tex;
        glGenTextures(1, &tex);

        auto texture_owner = core::make_scoped_owner(tex, [gl_target](GLuint id) {
            glBindTexture(gl_target, 0);
            glDeleteTextures(1, &id);
        });

        // Bind texture
        glBindTexture(gl_target, tex);

        if (gl_target == GL_TEXTURE_2D_MULTISAMPLE) {
            glTexImage2DMultisample(
                gl_target,
                info.sample_count,
                gl_pixel_format.internal_format,
                info.width,
                info.height,
                GL_TRUE
            );
        } else if (gl_target == GL_TEXTURE_2D) {
            glTexImage2D(
                gl_target,
                0, // mip level
                gl_pixel_format.internal_format,
                info.width,
                info.height,
                0, // border, always 0
                gl_pixel_format.format,
                gl_pixel_format.type,
                nullptr
            );
        } else if (gl_target == GL_TEXTURE_3D) {
            glTexImage3D(
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
            );
        } else if (gl_target == GL_TEXTURE_CUBE_MAP) {
            // Cubemap, allocate memory for all 6 faces
            for (int32 i = 0; i < 6; ++i) {
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, // mip level
                    gl_pixel_format.internal_format,
                    info.width,
                    info.height,
                    0, // border, always 0
                    gl_pixel_format.format,
                    gl_pixel_format.type,
                    nullptr
                );
            }
        } else {
            TAV_UNREACHABLE();
        }

        glBindTexture(gl_target, 0);

        auto h = m_resources.create({info, texture_owner.release(), gl_target});
        ::logger.debug("Texture `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_texture(texture_handle texture)
    {
        if (auto* tex = m_resources.try_get(texture)) {
            glDeleteTextures(1, &tex->texture_obj);
            m_resources.remove(texture);
            ::logger.debug("Texture `%s` destroyed", htos(texture));
        } else {
            ::logger.error("Failed to destroy texture `%s`: not found", htos(texture));
        }
    }

    pipeline_handle graphics_device_opengl::create_pipeline(
        const pipeline_info&                  info,
        const core::span<const shader_handle> shaders
    )
    {
        if (info.shaders.size() != 2) {
            ::logger.error("Failed to create pipeline: exactly 2 shaders required (vertex and fragment)");
            return pipeline_handle::invalid();
        }

        if (info.shaders.size() != shaders.size()) {
            ::logger.error("Failed to create pipeline: shaders size mismatch");
            return pipeline_handle::invalid();
        }

        GLuint vs = 0;
        GLuint fs = 0;
        for (auto i = 0; i < info.shaders.size(); ++i) {
            auto* s = m_resources.try_get(shaders[i]);
            if (!s) {
                ::logger.error("Failed to create pipeline: shader `%s` not found", htos(shaders[i]));
                return pipeline_handle::invalid();
            }

            if (s->info.stage != info.shaders[i].stage) {
                ::logger.error("Failed to create pipeline: shader stage mismatch");
                return pipeline_handle::invalid();
            }

            if (s->info.stage == shader_stage::vertex) {
                if (vs != 0) {
                    ::logger.error("Failed to create pipeline: multiple vertex shaders provided");
                    return pipeline_handle::invalid();
                }
                vs = s->shader_obj;
            } else if (s->info.stage == shader_stage::fragment) {
                if (fs != 0) {
                    ::logger.error("Failed to create pipeline: multiple fragment shaders provided");
                    return pipeline_handle::invalid();
                }
                fs = s->shader_obj;
            } else {
                ::logger.error("Failed to create pipeline: unsupported shader stage");
                return pipeline_handle::invalid();
            }
        }

        if (vs == 0) {
            ::logger.error("Failed to create pipeline: missing vertex shader");
            return pipeline_handle::invalid();
        }
        if (fs == 0) {
            ::logger.error("Failed to create pipeline: missing fragment shader");
            return pipeline_handle::invalid();
        }

        GLuint gl_program = link_program(vs, fs);

        // Validate program
        if (gl_program == 0) {
            ::logger.error("Failed to create pipeline: failed to link program");
            return pipeline_handle::invalid();
        }

        auto program_owner = core::make_scoped_owner(gl_program, [](GLuint id) {
            glDeleteProgram(id);
        });


        // Validate attributes

        // Map attributes to location index, for fast search
        tavros::core::unordered_map<uint32, vertex_attribute> mapped_attributes;
        for (const auto& attr : info.attributes) {
            auto it = mapped_attributes.find(attr.location);
            if (it != mapped_attributes.end()) {
                ::logger.error("Failed to create pipeline: attribute location `%u` is used multiple times", attr.location);
                return pipeline_handle::invalid();
            }
            mapped_attributes[attr.location] = attr;
        }

        // Get number of attributes in compiled shader
        GLint gl_prog_attrib_count;
        glGetProgramiv(gl_program, GL_ACTIVE_ATTRIBUTES, &gl_prog_attrib_count);

        // And validate each attribute
        size_t total_gl_attributes = 0;
        for (GLint i = 0; i < gl_prog_attrib_count; ++i) {
            GLchar attrib_name[256] = {0};
            GLint  size;
            GLenum type;
            glGetActiveAttrib(gl_program, i, sizeof(attrib_name), nullptr, &size, &type, attrib_name);

            GLint gl_attrib_location = glGetAttribLocation(gl_program, attrib_name);
            if (gl_attrib_location < 0) {
                // builtin attribute, just ignore it
                continue;
            }

            auto shader_type = gl_type_to_rhi_type(type);
            if (!shader_type.valid) {
                ::logger.error("Failed to create pipeline: unsupported attribute type by RHI, for attribute name `%s`", attrib_name);
                return pipeline_handle::invalid();
            }

            // Check attribute location
            auto it = mapped_attributes.find(static_cast<uint32>(gl_attrib_location));
            if (it == mapped_attributes.end()) {
                ::logger.error("Failed to create pipeline: attribute location `%i` not found in provided attributes", gl_attrib_location);
                return pipeline_handle::invalid();
            }

            // Check attribute type and format
            if (it->second.type != shader_type.type || it->second.format != shader_type.format) {
                ::logger.error(
                    "Failed to create pipeline: attribute `%s` at location `%i` has mismatched type/format. "
                    "Shader type/format = `%s`/`%s`, expected type/format = `%s`/`%s`",
                    attrib_name, gl_attrib_location,
                    to_string(shader_type.type).data(), to_string(shader_type.format).data(),
                    to_string(it->second.type).data(), to_string(it->second.format).data()
                );
                return pipeline_handle::invalid();
            }

            total_gl_attributes++;
        }

        if (total_gl_attributes != info.attributes.size()) {
            ::logger.error("Failed to create pipeline: mismach attributes size. Provided `%u`, expected `%u`", static_cast<uint32>(info.attributes.size()), static_cast<uint32>(total_gl_attributes));
            return pipeline_handle::invalid();
        }


        // create pipeline
        auto h = m_resources.create({info, program_owner.release()});
        ::logger.debug("Pipeline `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_pipeline(pipeline_handle handle)
    {
        if (auto* p = m_resources.try_get(handle)) {
            glDeleteProgram(p->program_obj);
            m_resources.remove(handle);
            ::logger.debug("Pipeline `%s` destroyed", htos(handle));
        } else {
            ::logger.error("Failed to destroy pipeline `%s`: not found", htos(handle));
        }
    }

    framebuffer_handle graphics_device_opengl::create_framebuffer(
        const framebuffer_info&                info,
        const core::span<const texture_handle> color_attachments,
        core::optional<texture_handle>         depth_stencil_attachment
    )
    {
        // Validate attachments size
        if (color_attachments.size() != info.color_attachment_formats.size()) {
            ::logger.error("Failed to create framebuffer: number of color attachments does not match the framebuffer info");
            return framebuffer_handle::invalid();
        }

        // Validate attachments
        core::static_vector<gl_texture*, k_max_color_attachments>    color_attachment_textures;
        core::static_vector<texture_handle, k_max_color_attachments> color_attachments_h;
        for (uint32 i = 0; i < color_attachments.size(); ++i) {
            const texture_handle tex_h = color_attachments[i];
            color_attachments_h.push_back(tex_h);

            auto* tex = m_resources.try_get(tex_h);
            if (!tex) {
                // Texture not found, so the framebuffer can't be created
                ::logger.error("Failed to create framebuffer: color attachment texture `%s` not found", htos(tex_h));
                return framebuffer_handle::invalid();
            }

            // Validate size
            if (tex->info.width != info.width || tex->info.height != info.height) {
                ::logger.error("Failed to create framebuffer: color attachment `%s` size `%u`x`%u` does not match framebuffer size `%u`x`%u`", htos(tex_h), tex->info.width, tex->info.height, info.width, info.height);
                return framebuffer_handle::invalid();
            }

            // Validate formats
            if (!is_color_format(info.color_attachment_formats[i])) {
                ::logger.error("Failed to create framebuffer: unsupported color attachment format for attachment `%u`", i);
                return framebuffer_handle::invalid();
            }

            if (!is_color_format(tex->info.format)) {
                ::logger.error("Failed to create framebuffer: unsupported color attachment format for texture `%s`", htos(tex_h));
                return framebuffer_handle::invalid();
            }

            if (info.color_attachment_formats[i] != tex->info.format) {
                ::logger.error("Failed to create framebuffer: color attachment texture `%s` format mismatch with framebuffer info", htos(tex_h));
                return framebuffer_handle::invalid();
            }

            // Validate MSAA
            if (info.sample_count != tex->info.sample_count) {
                ::logger.error("Failed to create framebuffer: color attachment texture `%s` sample count `%u` mismatch with framebuffer sample count `%u`", htos(tex_h), tex->info.sample_count, info.sample_count);
                return framebuffer_handle::invalid();
            }

            // All the attachments must be used as color attachments
            if (!tex->info.usage.has_flag(texture_usage::render_target)) {
                ::logger.error("Failed to create framebuffer: color attachment texture `%s` is not a render target", htos(tex_h));
                return framebuffer_handle::invalid();
            }

            color_attachment_textures.push_back(tex);
        }

        // Validate depth/stencil texture
        bool           depth_stencil_enabled = info.depth_stencil_attachment_format != pixel_format::none;
        bool           depth_stencil_provided = depth_stencil_attachment.has_value();
        texture_handle depth_stencil_attachment_h = texture_handle::invalid();

        if (depth_stencil_enabled && !depth_stencil_provided) {
            ::logger.error("Failed to create framebuffer: depth/stencil attachment is enabled but not provided");
            return framebuffer_handle::invalid();
        }
        if (!depth_stencil_enabled && depth_stencil_provided) {
            ::logger.error("Failed to create framebuffer: depth/stencil attachment is provided but not enabled");
            return framebuffer_handle::invalid();
        }

        GLuint fbo;
        glGenFramebuffers(1, &fbo);

        // Scope for framebuffer deletion if something goes wrong
        auto fbo_owner = core::make_scoped_owner(fbo, [](GLuint id) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &id);
        });

        // Bind framebuffer and attach textures
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Attach color textures to the framebuffer
        core::static_vector<GLenum, k_max_color_attachments> draw_buffers;
        for (uint32 i = 0; i < color_attachment_textures.size(); ++i) {
            auto*  tex = color_attachment_textures[i];
            GLenum attachment_type = GL_COLOR_ATTACHMENT0 + i;
            draw_buffers.push_back(attachment_type);
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_type, tex->target, tex->texture_obj, 0);
        }

        // Attach depth/stencil texture to the framebuffer if provided
        if (depth_stencil_enabled) {
            TAV_ASSERT(depth_stencil_provided);

            auto gl_format = to_depth_stencil_fromat(info.depth_stencil_attachment_format);
            if (!gl_format.is_depth_stencil_format) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment format is invalid");
                return framebuffer_handle::invalid();
            }

            // Get depth/stencil texture and attach it
            depth_stencil_attachment_h = depth_stencil_attachment.value();
            auto* tex = m_resources.try_get(depth_stencil_attachment_h);
            if (!tex) {
                ::logger.error("Failed to create framebuffer: depth/stencil texture `%s` not found", htos(depth_stencil_attachment_h));
                return framebuffer_handle::invalid();
            }

            // Validate depth/stencil size
            if (tex->info.width != info.width || tex->info.height != info.height) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment texture `%s` size `%u`x`%u` does not match framebuffer size `%u`x`%u`", htos(depth_stencil_attachment_h), tex->info.width, tex->info.height, info.width, info.height);
                return framebuffer_handle::invalid();
            }

            // Validate depth/stencil format
            if (tex->info.format != info.depth_stencil_attachment_format) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment texture `%s` format mismatch", htos(depth_stencil_attachment_h));
                return framebuffer_handle::invalid();
            }

            // Validate depth/stencil usage
            if (!tex->info.usage.has_flag(texture_usage::depth_stencil_target)) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment texture `%s` is not a depth/stencil target", htos(depth_stencil_attachment_h));
                return framebuffer_handle::invalid();
            }

            // Validate depth/stencil sample count
            if (tex->info.sample_count != info.sample_count) {
                ::logger.error("Failed to create framebuffer: depth/stencil attachment texture `%s` sample count mismatch", htos(depth_stencil_attachment_h));
                return framebuffer_handle::invalid();
            }

            // Everything is ok, attach depth/stencil texture
            glFramebufferTexture2D(GL_FRAMEBUFFER, gl_format.depth_stencil_attachment_type, tex->target, tex->texture_obj, 0);
        }

        // Framebuffer will be incomplete if the depth/stencil texture is not attached and
        // there are no color attachments
        if (draw_buffers.size() == 0 && info.depth_stencil_attachment_format == pixel_format::none) {
            ::logger.error("There are no attachments for framebuffer");
            return framebuffer_handle::invalid();
        }

        // Enable color attachments
        if (draw_buffers.empty()) {
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
            ::logger.error("Failed to create framebuffer: framebuffer is not complete");
            return framebuffer_handle::invalid();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        auto h = m_resources.create({info, fbo_owner.release(), false, color_attachments_h, depth_stencil_attachment_h});
        ::logger.debug("Framebuffer `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_framebuffer(framebuffer_handle framebuffer)
    {
        if (auto* fb = m_resources.try_get(framebuffer)) {
            glDeleteFramebuffers(1, &fb->framebuffer_obj);
            m_resources.remove(framebuffer);
            ::logger.debug("Framebuffer `%s` destroyed", htos(framebuffer));
        } else {
            ::logger.error("Failed to destroy framebuffer `%s`: not found", htos(framebuffer));
        }
    }

    buffer_handle graphics_device_opengl::create_buffer(const buffer_info& info)
    {
        if (info.size == 0) {
            ::logger.warning("Creating buffer with size 0: OpenGL will allocate an empty buffer");
        }

        GLenum gl_target = 0;
        GLenum gl_usage = 0;

        if (info.usage == buffer_usage::stage) {
            if (info.access != buffer_access::cpu_to_gpu) {
                ::logger.error("Failed to create buffer: stage buffer must have access type `cpu_to_gpu`");
                return buffer_handle::invalid();
            }

            gl_target = GL_COPY_WRITE_BUFFER;
            gl_usage = GL_STREAM_DRAW;
        } else {
            if (info.access == buffer_access::cpu_to_gpu) {
                ::logger.error("Failed to create buffer: access type `cpu_to_gpu` must be used only with buffer usage `stage`");
                return buffer_handle::invalid();
            }

            switch (info.usage) {
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
                TAV_UNREACHABLE();
            }

            switch (info.access) {
            case buffer_access::gpu_only:
                gl_usage = GL_STREAM_COPY; // GL_STATIC_DRAW, GL_STATIC_COPY, GL_STREAM_COPY - is also possible
                break;
            case buffer_access::gpu_to_cpu:
                gl_usage = GL_DYNAMIC_READ;
                break;
            default:
                TAV_UNREACHABLE();
            }
        }

        GLuint bo;
        glGenBuffers(1, &bo);

        auto bo_owner = core::make_scoped_owner(bo, [gl_target](GLuint id) {
            glBindBuffer(gl_target, 0);
            glDeleteBuffers(1, &id);
        });

        auto gl_size = static_cast<GLsizeiptr>(info.size);

        glBindBuffer(gl_target, bo);
        glBufferData(gl_target, gl_size, nullptr, gl_usage);
        glBindBuffer(gl_target, 0);

        auto h = m_resources.create({info, bo_owner.release(), gl_target, gl_usage});
        ::logger.debug("Buffer `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_buffer(buffer_handle buffer)
    {
        if (auto* b = m_resources.try_get(buffer)) {
            glDeleteBuffers(1, &b->buffer_obj);
            m_resources.remove(buffer);
            ::logger.debug("Buffer `%s` destroyed", htos(buffer));
        } else {
            ::logger.error("Failed to destroy buffer `%s`: not found", htos(buffer));
        }
    }

    geometry_handle graphics_device_opengl::create_geometry(
        const geometry_info&                  info,
        const core::span<const buffer_handle> vertex_buffers,
        core::optional<buffer_handle>         index_buffer
    )
    {
        // Check vertex buffers
        if (vertex_buffers.size() == 0) {
            ::logger.error("Failed to create geometry: no vertex buffers specified");
            return geometry_handle::invalid();
        }

        // Check buffer bindings
        for (auto i = 0; i < info.buffer_layouts.size(); ++i) {
            auto buffer_index = info.buffer_layouts[i].buffer_index;
            if (buffer_index >= vertex_buffers.size()) {
                uint32 max_index = static_cast<uint32>(vertex_buffers.size() - 1);
                ::logger.error("Failed to create geometry: invalid vertex buffer binding index `%u`, maximum allowed is `%u`", buffer_index, max_index);
                return geometry_handle::invalid();
            }
        }

        // Check attribute bindings
        for (auto i = 0; i < info.attribute_bindings.size(); ++i) {
            auto buffer_binding_index = info.attribute_bindings[i].buffer_layout_index;
            if (buffer_binding_index >= info.buffer_layouts.size()) {
                uint32 max_index = static_cast<uint32>(info.buffer_layouts.size() - 1);
                ::logger.error("Failed to create geometry: invalid vertex attribute binding index `%u`, maximum allowed is `%u`", buffer_binding_index, max_index);
                return geometry_handle::invalid();
            }
        }

        // Check index buffer
        if (info.has_index_buffer && index_buffer == core::nullopt) {
            ::logger.error("Failed to create geometry: index buffer is missing but required");
            return geometry_handle::invalid();
        }

        if (!info.has_index_buffer && index_buffer != core::nullopt) {
            ::logger.error("Failed to create geometry: index buffer was provided but not enabled");
            return geometry_handle::invalid();
        }

        // Create VAO
        GLuint vao;
        glGenVertexArrays(1, &vao);

        // Scope for vertex array deletion if something goes wrong
        auto vao_owner = core::make_scoped_owner(vao, [](GLuint id) {
            glBindVertexArray(0);
            glDeleteVertexArrays(1, &id);
        });

        glBindVertexArray(vao);


        // Setup attribute bindings
        for (auto attrib_i = 0; attrib_i < info.attribute_bindings.size(); ++attrib_i) {
            auto& attrib_bind = info.attribute_bindings[attrib_i];
            auto& attrib = attrib_bind.attribute;
            auto  buf_layout = info.buffer_layouts[attrib_bind.buffer_layout_index];

            // Get the buffer
            auto  vertex_buffer_h = vertex_buffers[buf_layout.buffer_index];
            auto* b = m_resources.try_get(vertex_buffer_h);
            if (!b) {
                ::logger.error("Failed to create geometry: vertex buffer `%s` not found", htos(vertex_buffer_h));
                return geometry_handle::invalid();
            }

            // Enable the vertex buffer
            glBindVertexBuffer(attrib_i, b->buffer_obj, buf_layout.base_offset, buf_layout.stride);

            auto gl_attrib_info = to_gl_attribute_info(attrib.type, attrib.format);
            for (uint32 col = 0; col < gl_attrib_info.cols; ++col) {
                GLuint location = attrib.location + col;
                GLuint offset = attrib_bind.offset + col * gl_attrib_info.rows * gl_attrib_info.size;

                // Enable attribute and set pointer
                glEnableVertexAttribArray(location);
                glVertexAttribFormat(location, gl_attrib_info.rows, gl_attrib_info.type, attrib.normalize, offset);
                glVertexAttribBinding(location, attrib_i);
            }
            glVertexBindingDivisor(attrib_i, attrib_bind.instance_divisor);
        }

        // Bind index buffer if present
        auto index_buffer_h = index_buffer.value();
        if (info.has_index_buffer) {
            auto* b = m_resources.try_get(index_buffer_h);
            if (!b) {
                ::logger.error("Failed to create geometry: index buffer `%s` not found", htos(index_buffer_h));
                return geometry_handle::invalid();
            }
            if (b->info.usage != buffer_usage::index) {
                ::logger.error("Failed to create geometry: buffer `%s` is not an index buffer", htos(index_buffer_h));
                return geometry_handle::invalid();
            }

            // Everything is ok, so bind index buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->buffer_obj);
        }

        glBindVertexArray(0);

        auto h = m_resources.create({info, vao_owner.release()});
        ::logger.debug("Geometry `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_geometry(geometry_handle geometry)
    {
        if (auto* g = m_resources.try_get(geometry)) {
            glDeleteVertexArrays(1, &g->vao_obj);
            m_resources.remove(geometry);
            ::logger.debug("Geometry `%s` destroyed", htos(geometry));
        } else {
            ::logger.error("Failed to destroy geometry `%s`: not found", htos(geometry));
        }
    }

    render_pass_handle graphics_device_opengl::create_render_pass(
        const render_pass_info&                info,
        const core::span<const texture_handle> resolve_textures
    )
    {
        // Validate attachments
        for (const auto& attachment : info.color_attachments) {
            if (!is_color_format(attachment.format)) {
                ::logger.error("Failed to create render pass: invalid color attachment format");
                return render_pass_handle::invalid();
            }
        }

        // Validate depth/stencil attachment
        auto depth_stencil_is_none = info.depth_stencil_attachment.format == pixel_format::none;
        if (!depth_stencil_is_none) {
            auto f = to_depth_stencil_fromat(info.depth_stencil_attachment.format);
            if (!f.is_depth_stencil_format) {
                ::logger.error("Failed to create render pass: invalid depth/stencil attachment format");
                return render_pass_handle::invalid();
            }
        }

        if (info.color_attachments.size() == 0 && depth_stencil_is_none) {
            ::logger.error("Failed to create render pass: no attachments specified");
            return render_pass_handle::invalid();
        }

        // Validate resolve attachments
        bool   is_used_for_resolve[k_max_color_attachments] = {false};
        uint32 need_resolve_textures_number = 0;
        for (auto i = 0; i < info.color_attachments.size(); ++i) {
            auto& attachment = info.color_attachments[i];
            if (attachment.store == store_op::resolve) {
                auto resolve_index = attachment.resolve_texture_index;

                // First of all, validate that resolve index is valid
                if (resolve_textures.empty()) {
                    ::logger.error("Failed to create render pass: no resolve textures available, but index `%u` was requested", resolve_index);
                    return render_pass_handle::invalid();
                }
                if (resolve_index >= resolve_textures.size()) {
                    uint32 max_index = static_cast<uint32>(resolve_textures.size() - 1);
                    ::logger.error("Failed to create render pass: invalid resolve texture index `%u`, maximum allowed is `%u`", resolve_index, max_index);
                    return render_pass_handle::invalid();
                }

                // Check for already used resolve index
                if (is_used_for_resolve[resolve_index]) {
                    ::logger.error("Failed to create render pass: resolve texture index `%u` is used more than once", resolve_index);
                    return render_pass_handle::invalid();
                }

                is_used_for_resolve[resolve_index] = true;

                auto& resolve_tex_h = resolve_textures[resolve_index];
                auto* resolve_tex = m_resources.try_get(resolve_tex_h);
                if (!resolve_tex) {
                    ::logger.error("Failed to create render pass: resolve texture `%s` not found", htos(resolve_tex_h));
                    return render_pass_handle::invalid();
                }


                if (resolve_tex->info.format != attachment.format) {
                    ::logger.error("Failed to create render pass: mismatched resolve texture format with color attachment `%u`", resolve_index);
                    return render_pass_handle::invalid();
                }
                if (!resolve_tex->info.usage.has_flag(texture_usage::resolve_destination)) {
                    ::logger.error("Failed to create render pass: resolve texture `%s` must have resolve_destination usage flag", htos(resolve_tex_h));
                    return render_pass_handle::invalid();
                }
                if (resolve_tex->info.sample_count != 1) {
                    ::logger.error("Failed to create render pass: resolve texture `%s` must be single-sampled", htos(resolve_tex_h));
                    return render_pass_handle::invalid();
                }

                need_resolve_textures_number++;
            }
        }

        // Make sure that all resolve textures are used
        if (resolve_textures.size() != need_resolve_textures_number) {
            ::logger.error("Failed to create render pass: not all resolve textures are used");
            return render_pass_handle::invalid();
        }

        core::static_vector<texture_handle, k_max_color_attachments> resolve_attachments_handles;
        for (auto i = 0; i < resolve_textures.size(); ++i) {
            resolve_attachments_handles.push_back(resolve_textures[i]);
        }

        auto h = m_resources.create({info, resolve_attachments_handles});
        ::logger.debug("Render pass `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_render_pass(render_pass_handle render_pass)
    {
        if (auto* rp = m_resources.try_get(render_pass)) {
            m_resources.remove(render_pass);
            ::logger.debug("Render pass `%s` destroyed", htos(render_pass));
        } else {
            ::logger.error("Failed to destroy render pass `%s`: not found", htos(render_pass));
        }
    }

    shader_binding_handle graphics_device_opengl::create_shader_binding(
        const shader_binding_info&             info,
        const core::span<const texture_handle> textures,
        const core::span<const sampler_handle> samplers,
        const core::span<const buffer_handle>  buffers
    )
    {
        // Validate texture bindings
        for (auto i = 0; i < info.texture_bindings.size(); ++i) {
            auto& binding = info.texture_bindings[i];

            // Textures
            if (textures.empty()) {
                ::logger.error("Failed to create shader binding: no textures available, but index `%u` was requested", binding.texture_index);
                return shader_binding_handle::invalid();
            }
            if (binding.texture_index >= textures.size()) {
                uint32 max_index = static_cast<uint32>(textures.size() - 1);
                ::logger.error("Failed to create shader binding: invalid texture binding index `%u`, maximum allowed is `%u`", binding.texture_index, max_index);
                return shader_binding_handle::invalid();
            }

            // Samplers
            if (samplers.empty()) {
                ::logger.error("Failed to create shader binding: no samplers available, but index `%u` was requested", binding.sampler_index);
                return shader_binding_handle::invalid();
            }
            if (binding.sampler_index >= samplers.size()) {
                uint32 max_index = static_cast<uint32>(samplers.size() - 1);
                ::logger.error("Failed to create shader binding: invalid sampler binding index `%u`, maximum allowed is `%u`", binding.sampler_index, max_index);
                return shader_binding_handle::invalid();
            }
        }

        // Validate buffer bindings
        for (auto i = 0; i < info.buffer_bindings.size(); ++i) {
            auto& binding = info.buffer_bindings[i];
            if (buffers.empty()) {
                ::logger.error("Failed to create shader binding: no buffers available, but index `%u` was requested", binding.buffer_index);
                return shader_binding_handle::invalid();
            }
            if (binding.buffer_index >= buffers.size()) {
                auto max_index = static_cast<uint32>(buffers.size()) - 1;
                ::logger.error("Failed to create shader binding: invalid buffer binding index `%u`, maximum allowed is `%u`", binding.buffer_index, max_index);
                return shader_binding_handle::invalid();
            }
        }

        core::static_vector<texture_handle, k_max_shader_textures> texture_handles;
        core::static_vector<sampler_handle, k_max_shader_textures> sampler_handles;
        core::static_vector<buffer_handle, k_max_shader_buffers>   buffer_handles;

        for (auto i = 0; i < textures.size(); ++i) {
            texture_handles.push_back(textures[i]);
        }

        for (auto i = 0; i < samplers.size(); ++i) {
            sampler_handles.push_back(samplers[i]);
        }

        for (auto i = 0; i < buffers.size(); ++i) {
            auto buffer_h = buffers[i];
            buffer_handles.push_back(buffer_h);
            auto* b = m_resources.try_get(buffer_h);
            if (b->info.usage != buffer_usage::uniform) {
                ::logger.error("Failed to create shader binding: buffer `%s` is not uniform buffer", htos(buffers[i]));
                return shader_binding_handle::invalid();
            }
        }

        auto h = m_resources.create({info, texture_handles, sampler_handles, buffer_handles});
        ::logger.debug("Shader binding `%s` created", htos(h));
        return h;
    }

    void graphics_device_opengl::destroy_shader_binding(shader_binding_handle shader_binding)
    {
        if (auto* sb = m_resources.try_get(shader_binding)) {
            m_resources.remove(shader_binding);
            ::logger.debug("Shader binding `%s` destroyed", htos(shader_binding));
        } else {
            ::logger.error("Failed to destroy shader binding `%s`: not found", htos(shader_binding));
        }
    }

    device_resources_opengl* graphics_device_opengl::get_resources()
    {
        return &m_resources;
    }

} // namespace tavros::renderer::rhi
