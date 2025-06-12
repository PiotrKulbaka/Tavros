#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/core/containers/unordered_map.hpp>

#include <atomic>

#include <glad/glad.h>

namespace tavros::renderer
{
    class gl_command_list;

    struct gl_sampler
    {
        sampler_desc desc;
        GLuint       sampler_obj = 0;
    };

    struct gl_texture
    {
        texture_desc desc;
        GLuint       texture_obj = 0;
    };

    struct gl_pipeline
    {
        pipeline_desc desc;
        GLuint        program_obj = 0;
    };

    struct gl_framebuffer
    {
        framebuffer_desc desc;
        GLuint           framebuffer_obj = 0;
    };

    struct gl_buffer
    {
        buffer_desc desc;
        GLuint      buffer_obj = 0;
    };

    struct gl_geometry_binding
    {
        geometry_binding_desc desc;
        GLuint                vao_obj = 0;
    };


    template<class T>
    class resource_pool
    {
    public:
        using handle_t = uint32;

    public:
        resource_pool() = default;
        ~resource_pool() = default;

        T* try_get(handle_t handle)
        {
            auto it = m_storage.find(handle);
            if (it != m_storage.end()) {
                return &it->second;
            }
            return nullptr;
        }

        const T* try_get(handle_t handle) const
        {
            return try_get(handle);
        }

        [[nodiscard]] handle_t insert(const T& desc)
        {
            auto handle = m_counter.fetch_add(1);
            m_storage[handle] = desc;
            return handle;
        }

        void remove(handle_t handle)
        {
            m_storage.erase(handle);
        }

    private:
        std::atomic<handle_t>          m_counter = 1;
        core::unordered_map<uint32, T> m_storage;
    };


    class gl_graphics_device final : public tavros::renderer::graphics_device
    {
    public:
        gl_graphics_device();
        ~gl_graphics_device() override;

        sampler_handle create_sampler(const sampler_desc& desc) override;
        void           destroy_sampler(sampler_handle handle) override;

        texture_handle create_texture(
            const texture_desc& desc,
            const uint8*        pixels = nullptr,
            uint32 stride = 0
        ) override;
        void destroy_texture(texture_handle handle) override;

        pipeline_handle create_pipeline(const pipeline_desc& desc) override;
        void            destroy_pipeline(pipeline_handle pipeline) override;

        framebuffer_handle create_framebuffer(const framebuffer_desc& desc, const core::span<const texture_handle> color_attachments, core::optional<texture_handle> depth_stencil_attachment = core::nullopt) override;
        void               destroy_framebuffer(framebuffer_handle framebuffer) override;

        buffer_handle create_buffer(
            const buffer_desc& desc,
            const uint8* data, uint64 size
        ) override;
        void destroy_buffer(buffer_handle buffer) override;

        geometry_binding_handle create_geometry(
            const geometry_binding_desc&          desc,
            const core::span<const buffer_handle> vertex_buffers,
            core::optional<buffer_handle>         index_buffer = core::nullopt
        ) override;
        void destroy_geometry_binding(geometry_binding_handle geometry_binding) override;

    private:
        resource_pool<gl_sampler>          m_samplers;
        resource_pool<gl_texture>          m_textures;
        resource_pool<gl_pipeline>         m_pipelines;
        resource_pool<gl_framebuffer>      m_framebuffers;
        resource_pool<gl_buffer>           m_buffers;
        resource_pool<gl_geometry_binding> m_geometry_bindings;

        friend gl_command_list;
    };

} // namespace tavros::renderer
