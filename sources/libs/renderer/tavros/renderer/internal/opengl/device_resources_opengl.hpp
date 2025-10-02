#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/resources/resource_pool.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/rhi/frame_composer.hpp>

#include <glad/glad.h>

namespace tavros::renderer::rhi
{

    struct gl_composer
    {
        frame_composer_info              info;
        core::unique_ptr<frame_composer> composer_ptr;
        void*                            native_handle = nullptr;
    };

    struct gl_sampler
    {
        sampler_info info;
        GLuint       sampler_obj = 0;
    };

    struct gl_texture
    {
        texture_info info;
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
        texture_handle                                               depth_stencil_attachment = texture_handle::invalid();
    };

    struct gl_buffer
    {
        buffer_info info;
        GLuint      buffer_obj = 0;
        GLenum      gl_target = 0;
        GLenum      gl_usage = 0;
    };

    struct gl_geometry
    {
        geometry_info info;
        GLuint        vao_obj = 0;
    };

    struct gl_render_pass
    {
        render_pass_info                                             info;
        core::static_vector<texture_handle, k_max_color_attachments> resolve_attachments;
    };

    struct gl_shader_binding
    {
        shader_binding_info                                        info;
        core::static_vector<texture_handle, k_max_shader_textures> textures;
        core::static_vector<sampler_handle, k_max_shader_textures> samplers;
        core::static_vector<buffer_handle, k_max_shader_buffers>   buffers;
    };

    struct gl_shader
    {
        shader_info info;
        GLuint      shader_obj = 0;
    };


    class device_resources_opengl
    {
    public:
        device_resources_opengl()
            : samplers(&alc)
            , composers(&alc)
            , shader_bindings(&alc)
            , shaders(&alc)
            , textures(&alc)
            , pipelines(&alc)
            , framebuffers(&alc)
            , buffers(&alc)
            , geometries(&alc)
            , render_passes(&alc)
        {
        }

        ~device_resources_opengl() = default;

        // --- Create ---
        frame_composer_handle create(gl_composer&& data)
        {
            return {composers.add(std::move(data)).id};
        }

        shader_handle create(gl_shader&& data)
        {
            return {shaders.add(std::move(data)).id};
        }

        shader_binding_handle create(gl_shader_binding&& data)
        {
            return {shader_bindings.add(std::move(data)).id};
        }

        sampler_handle create(gl_sampler&& data)
        {
            return {samplers.add(std::move(data)).id};
        }

        texture_handle create(gl_texture&& data)
        {
            return {textures.add(std::move(data)).id};
        }

        pipeline_handle create(gl_pipeline&& data)
        {
            return {pipelines.add(std::move(data)).id};
        }

        framebuffer_handle create(gl_framebuffer&& data)
        {
            return {framebuffers.add(std::move(data)).id};
        }

        buffer_handle create(gl_buffer&& data)
        {
            return {buffers.add(std::move(data)).id};
        }

        geometry_handle create(gl_geometry&& data)
        {
            return {geometries.add(std::move(data)).id};
        }

        render_pass_handle create(gl_render_pass&& data)
        {
            return {render_passes.add(std::move(data)).id};
        }

        // --- Remove ---
        void remove(frame_composer_handle handle)
        {
            composers.erase({handle.id});
        }

        void remove(shader_handle handle)
        {
            shaders.erase({handle.id});
        }

        void remove(shader_binding_handle handle)
        {
            shader_bindings.erase({handle.id});
        }

        void remove(sampler_handle handle)
        {
            samplers.erase({handle.id});
        }

        void remove(texture_handle handle)
        {
            textures.erase({handle.id});
        }

        void remove(pipeline_handle handle)
        {
            pipelines.erase({handle.id});
        }

        void remove(framebuffer_handle handle)
        {
            framebuffers.erase({handle.id});
        }

        void remove(buffer_handle handle)
        {
            buffers.erase({handle.id});
        }

        void remove(geometry_handle handle)
        {
            geometries.erase({handle.id});
        }

        void remove(render_pass_handle handle)
        {
            render_passes.erase({handle.id});
        }

        // --- Try get ---
        gl_composer* try_get(frame_composer_handle handle)
        {
            return composers.try_get({handle.id});
        }

        gl_shader* try_get(shader_handle handle)
        {
            return shaders.try_get({handle.id});
        }

        gl_shader_binding* try_get(shader_binding_handle handle)
        {
            return shader_bindings.try_get({handle.id});
        }

        gl_sampler* try_get(sampler_handle handle)
        {
            return samplers.try_get({handle.id});
        }

        gl_texture* try_get(texture_handle handle)
        {
            return textures.try_get({handle.id});
        }

        gl_pipeline* try_get(pipeline_handle handle)
        {
            return pipelines.try_get({handle.id});
        }

        gl_framebuffer* try_get(framebuffer_handle handle)
        {
            return framebuffers.try_get({handle.id});
        }

        gl_buffer* try_get(buffer_handle handle)
        {
            return buffers.try_get({handle.id});
        }

        gl_geometry* try_get(geometry_handle handle)
        {
            return geometries.try_get({handle.id});
        }

        gl_render_pass* try_get(render_pass_handle handle)
        {
            return render_passes.try_get({handle.id});
        }

    public:
        core::mallocator                       alc;
        core::resource_pool<gl_sampler>        samplers;
        core::resource_pool<gl_composer>       composers;
        core::resource_pool<gl_shader_binding> shader_bindings;
        core::resource_pool<gl_shader>         shaders;
        core::resource_pool<gl_texture>        textures;
        core::resource_pool<gl_pipeline>       pipelines;
        core::resource_pool<gl_framebuffer>    framebuffers;
        core::resource_pool<gl_buffer>         buffers;
        core::resource_pool<gl_geometry>       geometries;
        core::resource_pool<gl_render_pass>    render_passes;
    };

} // namespace tavros::renderer::rhi
