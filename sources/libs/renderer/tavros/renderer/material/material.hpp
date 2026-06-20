#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/material/material_desc.hpp>
#include <tavros/renderer/shaders/shader_source_provider.hpp>

namespace tavros::renderer
{

    class material
    {
    public:
        struct texture
        {
            core::short_string  name;
            core::medium_string path;
        };

        struct shader_stages
        {
            core::fixed_string<255> vertex_shader_path;
            core::fixed_string<255> fragment_shader_path;
        };

    public:
        // core::string_view


        core::string_view name() const noexcept;

    private:
        core::short_string m_name;
    };

    // class material_instance : public basic_resource
    //{
    // public:
    //     material_instance(core::string_view name, rhi::pipeline_handle pipeline, core::buffer_view<rhi::buffer_binding> buffers, core::buffer_view<rhi::texture_binding> textures) noexcept
    //         : basic_resource(name)
    //         , m_pipeline(pipeline)
    //         , m_buffers(buffers)
    //         , m_textures(textures)
    //     {
    //     }

    //    rhi::pipeline_handle pipeline() const noexcept
    //    {
    //        return m_pipeline;
    //    }

    //    core::buffer_view<rhi::buffer_binding> buffer_instances() const noexcept
    //    {
    //        return m_buffers;
    //    }

    //    core::buffer_view<rhi::texture_binding> texture_instances() const noexcept
    //    {
    //        return m_textures;
    //    }

    // private:
    //     rhi::pipeline_handle                                                 m_pipeline;
    //     core::fixed_vector<rhi::buffer_binding, rhi::k_max_shader_buffers>   m_buffers;
    //     core::fixed_vector<rhi::texture_binding, rhi::k_max_shader_textures> m_textures;
    // };


    // class mesh_instance : public basic_resource
    //{
    // public:
    //     mesh_instance(core::string_view name, core::buffer_view<rhi::bind_buffer_info> vertex_buffers, rhi::bind_index_buffer_info& index_buffer) noexcept
    //         : basic_resource(name)
    //         , m_vertex_buffers(vertex_buffers)
    //         , m_index_buffer(index_buffer)
    //     {
    //     }

    //    core::buffer_view<rhi::bind_buffer_info> vertex_buffers() const noexcept
    //    {
    //        return m_vertex_buffers;
    //    }

    //    rhi::bind_index_buffer_info index_buffer() const noexcept
    //    {
    //        return m_index_buffer;
    //    }

    // private:
    //     core::fixed_vector<rhi::bind_buffer_info, rhi::k_max_vertex_attributes> m_vertex_buffers;
    //     rhi::bind_index_buffer_info                                             m_index_buffer;
    // };


    class material_
    {
    public:
        material_(rhi::graphics_device* gdevice, const material_desc& desc, shader_source_provider* shader_provider, uint32 msaa, rhi::pixel_format ds_format);
        ~material_() noexcept;

        core::string_view name() const noexcept;

        rhi::pipeline_handle pipeline() const noexcept;

    private:
        core::fixed_string<255> m_name;
        rhi::graphics_device*   m_gdevice;
        rhi::pipeline_handle    m_pipeine;
    };

} // namespace tavros::renderer