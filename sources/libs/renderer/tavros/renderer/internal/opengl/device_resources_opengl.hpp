#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/object/object_pool.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/rhi/frame_composer.hpp>

#include <glad/glad.h>

namespace tavros::renderer::rhi
{

    struct gl_composer
    {
        frame_composer_create_info       info;
        core::unique_ptr<frame_composer> composer_ptr;
    };

    struct gl_sampler
    {
        sampler_create_info info;

        GLuint sampler_obj = 0;
    };

    struct gl_texture
    {
        texture_create_info info;

        GLuint texture_obj = 0;
        GLenum target = 0;
        GLuint renderbuffer_obj = 0;
        uint32 max_mip = 0;
    };

    struct gl_pipeline
    {
        pipeline_create_info info;

        GLuint program_obj = 0;
        GLuint vao_obj = 0;
    };

    struct gl_framebuffer
    {
        framebuffer_create_info info;

        GLuint       framebuffer_obj = 0;
        bool         is_default = false;
        pixel_format default_fb_color_format = pixel_format::none;
        pixel_format default_fb_ds_format = pixel_format::none;
    };

    struct gl_buffer
    {
        buffer_create_info info;

        GLuint buffer_obj = 0;
        GLenum gl_target = 0;
        GLenum gl_usage = 0;
        bool   is_mapped = false;
    };

    struct gl_render_pass
    {
        render_pass_create_info info;
    };

    struct gl_shader_binding
    {
        shader_binding_create_info info;
    };

    struct gl_shader
    {
        shader_create_info info;

        GLuint shader_obj = 0;
    };

    struct gl_fence
    {
        GLsync fence_obj = nullptr;
    };


    class device_resources_opengl
    {
    public:
        template<typename T>
        struct data_type_traits;
        template<typename T>
        struct handle_type_traits;

        template<class T>
        using object_type_of = typename data_type_traits<T>::type;

    public:
        explicit device_resources_opengl(core::allocator* alc)
            : m_samplers(alc)
            , m_composers(alc)
            , m_shader_bindings(alc)
            , m_shaders(alc)
            , m_textures(alc)
            , m_pipelines(alc)
            , m_framebuffers(alc)
            , m_buffers(alc)
            , m_render_passes(alc)
            , m_fences(alc)
        {
        }

        ~device_resources_opengl() = default;

        template<typename T>
        auto create(T&& data)
        {
            auto& pool = get_pool<std::decay_t<T>>();
            return typename handle_type_traits<T>::type{pool.add(std::forward<T>(data)).id};
        }

        template<typename Handle>
        void remove(Handle handle)
        {
            auto& pool = get_pool<object_type_of<Handle>>();
            pool.erase(core::object_handle<object_type_of<Handle>>(handle.id));
        }

        template<typename Handle>
        auto try_get(Handle handle)
        {
            auto& pool = get_pool<object_type_of<Handle>>();
            return pool.try_get(core::object_handle<object_type_of<Handle>>(handle.id));
        }

        template<typename T>
        core::object_pool<T>& get_pool();

    private:
        core::object_pool<gl_sampler>        m_samplers;
        core::object_pool<gl_composer>       m_composers;
        core::object_pool<gl_shader_binding> m_shader_bindings;
        core::object_pool<gl_shader>         m_shaders;
        core::object_pool<gl_texture>        m_textures;
        core::object_pool<gl_pipeline>       m_pipelines;
        core::object_pool<gl_framebuffer>    m_framebuffers;
        core::object_pool<gl_buffer>         m_buffers;
        core::object_pool<gl_render_pass>    m_render_passes;
        core::object_pool<gl_fence>          m_fences;
    };

    // clang-format off
    template<> struct device_resources_opengl::data_type_traits<frame_composer_handle> { using type = gl_composer; };
    template<> struct device_resources_opengl::data_type_traits<shader_handle> { using type = gl_shader; };
    template<> struct device_resources_opengl::data_type_traits<shader_binding_handle> { using type = gl_shader_binding; };
    template<> struct device_resources_opengl::data_type_traits<sampler_handle> { using type = gl_sampler; };
    template<> struct device_resources_opengl::data_type_traits<texture_handle> { using type = gl_texture; };
    template<> struct device_resources_opengl::data_type_traits<pipeline_handle> { using type = gl_pipeline; };
    template<> struct device_resources_opengl::data_type_traits<framebuffer_handle> { using type = gl_framebuffer; };
    template<> struct device_resources_opengl::data_type_traits<buffer_handle> { using type = gl_buffer; };
    template<> struct device_resources_opengl::data_type_traits<render_pass_handle> { using type = gl_render_pass; };
    template<> struct device_resources_opengl::data_type_traits<fence_handle> { using type = gl_fence; };

    template<> struct device_resources_opengl::handle_type_traits<gl_composer> { using type = frame_composer_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_shader> { using type = shader_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_shader_binding> { using type = shader_binding_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_sampler> { using type = sampler_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_texture> { using type = texture_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_pipeline> { using type = pipeline_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_framebuffer> { using type = framebuffer_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_buffer> { using type = buffer_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_render_pass> { using type = render_pass_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_fence> { using type = fence_handle; };

    template<> inline core::object_pool<gl_composer>& device_resources_opengl::get_pool<gl_composer>() { return m_composers; }
    template<> inline core::object_pool<gl_shader>& device_resources_opengl::get_pool<gl_shader>() { return m_shaders; }
    template<> inline core::object_pool<gl_shader_binding>& device_resources_opengl::get_pool<gl_shader_binding>() { return m_shader_bindings; }
    template<> inline core::object_pool<gl_sampler>& device_resources_opengl::get_pool<gl_sampler>() { return m_samplers; }
    template<> inline core::object_pool<gl_texture>& device_resources_opengl::get_pool<gl_texture>() { return m_textures; }
    template<> inline core::object_pool<gl_pipeline>& device_resources_opengl::get_pool<gl_pipeline>() { return m_pipelines; }
    template<> inline core::object_pool<gl_framebuffer>& device_resources_opengl::get_pool<gl_framebuffer>() { return m_framebuffers; }
    template<> inline core::object_pool<gl_buffer>& device_resources_opengl::get_pool<gl_buffer>() { return m_buffers; }
    template<> inline core::object_pool<gl_render_pass>& device_resources_opengl::get_pool<gl_render_pass>() { return m_render_passes; }
    template<> inline core::object_pool<gl_fence>& device_resources_opengl::get_pool<gl_fence>() { return m_fences; }
    // clang-format on

} // namespace tavros::renderer::rhi
