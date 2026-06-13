#pragma once

#include <tavros/renderer/rhi/shader_reflect.hpp>
#include <tavros/core/containers/vector.hpp>
#include <glad/glad.h>

namespace tavros::renderer::rhi
{

    class gl_shader_program_reflect : public shader_reflect
    {
    public:
        struct ubo_range
        {
            uint32 begin = 0;
            uint32 end = 0;
        };

    public:
        gl_shader_program_reflect(GLuint prog, bool is_compute);
        ~gl_shader_program_reflect() noexcept override;

        bool is_valid() const noexcept;

        core::buffer_view<vertex_attribute_reflect> vertex_attributes() const noexcept override;
        core::buffer_view<shader_resource_reflect>  shader_resources() const noexcept override;
        core::buffer_view<constant_block_reflect>   constant_blocks() const noexcept override;
        core::buffer_view<member_reflect>           constant_block_members(size_t constant_block_index) const noexcept override;
        core::buffer_view<storage_block_reflect>    storage_blocks() const noexcept override;
        core::buffer_view<output_reflect>           outputs() const noexcept override;
        const compute_reflect&                      compute() const noexcept override;

    private:
        bool                                   m_valid = false;
        core::vector<vertex_attribute_reflect> m_vert_attribs;
        core::vector<shader_resource_reflect>  m_shader_res;
        core::vector<constant_block_reflect>   m_ubo_blocks;
        core::vector<member_reflect>           m_ubo_members;
        core::vector<ubo_range>                m_ubo_ranges;
        core::vector<storage_block_reflect>    m_ssbo_blocks;
        core::vector<output_reflect>           m_outputs;
        compute_reflect                        m_compute;
    };

} // namespace tavros::renderer::rhi
