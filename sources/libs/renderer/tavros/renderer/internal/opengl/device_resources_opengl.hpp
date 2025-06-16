#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/resources/resource_pool.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/rhi/swapchain.hpp>

#include <glad/glad.h>

namespace tavros::renderer
{

    struct gl_swapchain
    {
        swapchain_desc              desc;
        core::shared_ptr<swapchain> swapchain_ptr;
        void*                       native_handle = nullptr;
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
        pipeline_desc desc;
        GLuint        program_obj = 0;
    };

    struct gl_framebuffer
    {
        framebuffer_desc desc;
        GLuint           framebuffer_obj = 0;
        bool             is_default = false;
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

    struct device_resources_opengl
    {
        core::resource_pool<gl_swapchain>        swapchains;
        core::resource_pool<gl_sampler>          samplers;
        core::resource_pool<gl_texture>          textures;
        core::resource_pool<gl_pipeline>         pipelines;
        core::resource_pool<gl_framebuffer>      framebuffers;
        core::resource_pool<gl_buffer>           buffers;
        core::resource_pool<gl_geometry_binding> geometry_bindings;
    };

} // namespace tavros::renderer
