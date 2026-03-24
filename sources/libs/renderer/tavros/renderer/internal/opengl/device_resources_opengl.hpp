#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/resource/object_pool.hpp>
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

        GLuint     buffer_obj = 0;
        GLenum     gl_target = 0;
        GLenum     gl_usage = 0;
        GLbitfield gl_flags = 0;
        bool       is_mapped = false;
    };

    struct gl_render_pass
    {
        render_pass_create_info info;
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
        /// Maps a public handle type internal data type (gl_*)
        template<typename Handle>
        struct data_type_traits;

        /// Maps an internal data type (gl_*) public handle type
        template<typename T>
        struct handle_type_traits;

        /// Maps an internal data type (gl_*) its handle tag
        template<typename T>
        struct tag_type_traits;

        template<typename Handle>
        using object_type_of = typename data_type_traits<Handle>::type;

        template<typename T>
        using tag_of = typename tag_type_traits<T>::type;

        template<typename T>
        using handle_of = typename handle_type_traits<T>::type;

    public:
        explicit device_resources_opengl()
            : m_samplers()
            , m_composers()
            , m_shaders()
            , m_textures()
            , m_pipelines()
            , m_framebuffers()
            , m_buffers()
            , m_render_passes()
            , m_fences()
        {
        }

        ~device_resources_opengl() = default;

        /**
         * @brief Creates a new resource by moving data into the appropriate pool.
         * @tparam T Internal resource data type (e.g. gl_texture).
         * @param data Resource data to move into the pool.
         * @return Public handle to the created resource.
         */
        template<typename T>
        handle_of<T> create(T&& data)
        {
            auto h = get_pool<T>().push(std::forward<T>(data));
            return handle_of<T>{h.id};
        }

        /**
         * @brief Removes a resource identified by a public handle.
         * @tparam Handle Public handle type (e.g. texture_handle).
         * @param handle Handle to the resource to remove.
         */
        template<typename Handle>
        void remove(Handle handle)
        {
            using data_t = object_type_of<Handle>;
            using tag_t = tag_of<data_t>;
            get_pool<data_t>().erase(core::handle_base<tag_t>{handle.id});
        }

        /**
         * @brief Returns a pointer to the resource data, or nullptr if the handle is invalid.
         * @tparam Handle Public handle type (e.g. texture_handle).
         * @param handle Handle to look up.
         * @return Pointer to internal data, or nullptr.
         */
        template<typename Handle>
        object_type_of<Handle>* find(Handle handle) noexcept
        {
            using data_t = object_type_of<Handle>;
            using tag_t = tag_of<data_t>;
            return get_pool<data_t>().find(core::handle_base<tag_t>{handle.id});
        }

        template<typename Handle>
        const object_type_of<Handle>* find(Handle handle) const noexcept
        {
            using data_t = object_type_of<Handle>;
            using tag_t = tag_of<data_t>;
            return get_pool<data_t>().find(core::handle_base<tag_t>{handle.id});
        }

        /**
         * @brief Returns the pool for a given internal data type.
         * @tparam T Internal resource data type.
         */
        template<typename T>
        core::object_pool<T, tag_of<T>>& get_pool() noexcept;

        /**
         * @brief Returns the pool for a given internal data type.
         * @tparam T Internal resource data type.
         */
        template<typename T>
        const core::object_pool<T, tag_of<T>>& get_pool() const noexcept;

    private:
        core::object_pool<gl_sampler, sampler_tag>         m_samplers;
        core::object_pool<gl_composer, frame_composer_tag> m_composers;
        core::object_pool<gl_shader, shader_tag>           m_shaders;
        core::object_pool<gl_texture, texture_tag>         m_textures;
        core::object_pool<gl_pipeline, pipeline_tag>       m_pipelines;
        core::object_pool<gl_framebuffer, framebuffer_tag> m_framebuffers;
        core::object_pool<gl_buffer, buffer_tag>           m_buffers;
        core::object_pool<gl_render_pass, render_pass_tag> m_render_passes;
        core::object_pool<gl_fence, fence_tag>             m_fences;
    };

    // clang-format off
    // Handle -> data type
    template<> struct device_resources_opengl::data_type_traits<frame_composer_handle> { using type = gl_composer; };
    template<> struct device_resources_opengl::data_type_traits<shader_handle>         { using type = gl_shader; };
    template<> struct device_resources_opengl::data_type_traits<sampler_handle>        { using type = gl_sampler; };
    template<> struct device_resources_opengl::data_type_traits<texture_handle>        { using type = gl_texture; };
    template<> struct device_resources_opengl::data_type_traits<pipeline_handle>       { using type = gl_pipeline; };
    template<> struct device_resources_opengl::data_type_traits<framebuffer_handle>    { using type = gl_framebuffer; };
    template<> struct device_resources_opengl::data_type_traits<buffer_handle>         { using type = gl_buffer; };
    template<> struct device_resources_opengl::data_type_traits<render_pass_handle>    { using type = gl_render_pass; };
    template<> struct device_resources_opengl::data_type_traits<fence_handle>          { using type = gl_fence; };

    // Data type -> handle type
    template<> struct device_resources_opengl::handle_type_traits<gl_composer>       { using type = frame_composer_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_shader>         { using type = shader_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_sampler>        { using type = sampler_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_texture>        { using type = texture_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_pipeline>       { using type = pipeline_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_framebuffer>    { using type = framebuffer_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_buffer>         { using type = buffer_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_render_pass>    { using type = render_pass_handle; };
    template<> struct device_resources_opengl::handle_type_traits<gl_fence>          { using type = fence_handle; };

    // Data type -> tag type
    template<> struct device_resources_opengl::tag_type_traits<gl_composer>       { using type = frame_composer_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_shader>         { using type = shader_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_sampler>        { using type = sampler_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_texture>        { using type = texture_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_pipeline>       { using type = pipeline_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_framebuffer>    { using type = framebuffer_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_buffer>         { using type = buffer_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_render_pass>    { using type = render_pass_tag; };
    template<> struct device_resources_opengl::tag_type_traits<gl_fence>          { using type = fence_tag; };

    // get_pool specializations
    template<> inline core::object_pool<gl_composer, frame_composer_tag>&       device_resources_opengl::get_pool<gl_composer>() noexcept       { return m_composers; }
    template<> inline core::object_pool<gl_shader, shader_tag>&                 device_resources_opengl::get_pool<gl_shader>() noexcept         { return m_shaders; }
    template<> inline core::object_pool<gl_sampler, sampler_tag>&               device_resources_opengl::get_pool<gl_sampler>() noexcept        { return m_samplers; }
    template<> inline core::object_pool<gl_texture, texture_tag>&               device_resources_opengl::get_pool<gl_texture>() noexcept        { return m_textures; }
    template<> inline core::object_pool<gl_pipeline, pipeline_tag>&             device_resources_opengl::get_pool<gl_pipeline>() noexcept       { return m_pipelines; }
    template<> inline core::object_pool<gl_framebuffer, framebuffer_tag>&       device_resources_opengl::get_pool<gl_framebuffer>() noexcept    { return m_framebuffers; }
    template<> inline core::object_pool<gl_buffer, buffer_tag>&                 device_resources_opengl::get_pool<gl_buffer>() noexcept         { return m_buffers; }
    template<> inline core::object_pool<gl_render_pass, render_pass_tag>&       device_resources_opengl::get_pool<gl_render_pass>() noexcept    { return m_render_passes; }
    template<> inline core::object_pool<gl_fence, fence_tag>&                   device_resources_opengl::get_pool<gl_fence>() noexcept          { return m_fences; }

    template<> inline const core::object_pool<gl_composer, frame_composer_tag>&       device_resources_opengl::get_pool<gl_composer>() const noexcept       { return m_composers; }
    template<> inline const core::object_pool<gl_shader, shader_tag>&                 device_resources_opengl::get_pool<gl_shader>() const noexcept         { return m_shaders; }
    template<> inline const core::object_pool<gl_sampler, sampler_tag>&               device_resources_opengl::get_pool<gl_sampler>() const noexcept        { return m_samplers; }
    template<> inline const core::object_pool<gl_texture, texture_tag>&               device_resources_opengl::get_pool<gl_texture>() const noexcept        { return m_textures; }
    template<> inline const core::object_pool<gl_pipeline, pipeline_tag>&             device_resources_opengl::get_pool<gl_pipeline>() const noexcept       { return m_pipelines; }
    template<> inline const core::object_pool<gl_framebuffer, framebuffer_tag>&       device_resources_opengl::get_pool<gl_framebuffer>() const noexcept    { return m_framebuffers; }
    template<> inline const core::object_pool<gl_buffer, buffer_tag>&                 device_resources_opengl::get_pool<gl_buffer>() const noexcept         { return m_buffers; }
    template<> inline const core::object_pool<gl_render_pass, render_pass_tag>&       device_resources_opengl::get_pool<gl_render_pass>() const noexcept    { return m_render_passes; }
    template<> inline const core::object_pool<gl_fence, fence_tag>&                   device_resources_opengl::get_pool<gl_fence>() const noexcept          { return m_fences; }
    // clang-format on

} // namespace tavros::renderer::rhi
