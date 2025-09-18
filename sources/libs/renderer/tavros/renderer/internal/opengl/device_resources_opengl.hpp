#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/resources/resource_pool.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/rhi/frame_composer.hpp>

#include <glad/glad.h>

namespace tavros::renderer
{

    struct gl_composer
    {
        frame_composer_desc              desc;
        core::unique_ptr<frame_composer> composer_ptr;
        void*                            native_handle = nullptr;
    };

    struct gl_sampler
    {
        sampler_desc desc;
        GLuint       sampler_obj = 0;
    };

    struct gl_texture
    {
        texture_desc desc;
        GLuint       texture_obj = 0;
        GLenum       target = 0;
    };

    struct gl_pipeline
    {
        pipeline_info info;
        GLuint        program_obj = 0;
    };

    struct gl_framebuffer
    {
        framebuffer_info info;
        GLuint           framebuffer_obj = 0;
        bool             is_default = false;

        core::static_vector<texture_handle, k_max_color_attachments> color_attachments;
        texture_handle                                               depth_stencil_attachment = {0};
    };

    struct gl_buffer
    {
        buffer_info info;
        GLuint      buffer_obj = 0;
        GLenum      gl_target = 0;
        GLenum      gl_usage = 0;
    };

    struct gl_geometry_binding
    {
        geometry_binding_desc desc;
        GLuint                vao_obj = 0;
    };

    struct gl_render_pass
    {
        render_pass_desc                                             desc;
        core::static_vector<texture_handle, k_max_color_attachments> resolve_attachments;
    };

    struct gl_shader_binding
    {
        shader_binding_desc                                        desc;
        core::static_vector<texture_handle, k_max_shader_textures> textures;
        core::static_vector<sampler_handle, k_max_shader_textures> samplers;
        core::static_vector<buffer_handle, k_max_shader_buffers>   buffers;
    };

    struct gl_shader
    {
        shader_info info;
        GLuint      shader_obj = 0;
    };

    struct device_resources_opengl
    {
    public:
        gl_shader* try_get(shader_handle shader)
        {
            return shaders.try_get(shader.id);
        }

        void remove(shader_handle handle)
        {
            shaders.remove(handle.id);
        }

        shader_handle create(gl_shader&& data)
        {
            return {shaders.insert(std::move(data))};
        }

    public:
        core::resource_pool<gl_composer>         composers;
        core::resource_pool<gl_sampler>          samplers;
        core::resource_pool<gl_texture>          textures;
        core::resource_pool<gl_pipeline>         pipelines;
        core::resource_pool<gl_framebuffer>      framebuffers;
        core::resource_pool<gl_buffer>           buffers;
        core::resource_pool<gl_geometry_binding> geometry_bindings;
        core::resource_pool<gl_render_pass>      render_passes;
        core::resource_pool<gl_shader_binding>   shader_bindings;
        core::resource_pool<gl_shader>           shaders;
    };

} // namespace tavros::renderer
