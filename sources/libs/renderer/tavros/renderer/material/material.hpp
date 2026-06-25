#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/renderer/shaders/shader_loader.hpp>

#include <tavros/core/resource/resource.hpp>
#include <tavros/renderer/material/material_desc.hpp>

#include <tavros/renderer/texture/texture.hpp>

namespace tavros::renderer
{

    class material
        : public core::basic_resource<material>,
          core::noncopyable
    {
    public:
        material(rhi::graphics_device* gdevice, const material_desc& desc, shader_loader& sl, uint32 msaa, rhi::pixel_format ds_format);

        material(material&&) noexcept;

        ~material() noexcept;

        rhi::pipeline_handle gpu_pipeline() const noexcept;

        const rhi::shader_reflect* shader_reflect() const noexcept;

    private:
        rhi::graphics_device* m_gdevice = nullptr;
        rhi::pipeline_handle  m_pipeline;
        rhi::shader_handle    m_shader;
    };

    using material_ref = core::basic_resource_ref<material>;


    /*class material_instance
        : public core::basic_resource<material>,
        core::noncopyable
    {
    public:
        material_ref materaial();

        void update() noexcept;

        core::buffer_view<rhi::texture_binding> texture_bindings() const noexcept;

        rhi::buffer_binding buffer_binding() const noexcept;
    private:
        core::fixed_vector<texture_ref, rhi::k_max_shader_textures>          m_textures;
        core::fixed_vector<rhi::texture_binding, rhi::k_max_shader_textures> m_texture_bindings;
        rhi::buffer_binding                                                  m_material_buffer;
    };*/

} // namespace tavros::renderer