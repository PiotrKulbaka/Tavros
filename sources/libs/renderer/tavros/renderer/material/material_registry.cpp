#include <tavros/renderer/material/material_registry.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/assert.hpp>

#include <tavros/renderer/resource_manager.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>

namespace
{
    tavros::core::logger logger("material_registry");
}

namespace tavros::renderer
{

    material_registry::material_registry(resource_manager* rm) noexcept
        : m_rm(rm)
    {
    }

    material_registry::handle_type material_registry::create(const tef::workspace& ws, render_target_handle rt_handle, core::string_view mt_path)
    {
        return {};
    }

    material_registry::handle_type material_registry::create(core::string_view mt_name, render_target_handle rt_handle, const material_desc& desc)
    {
        const auto* rt = m_rm->render_targets().find(rt_handle);
        if (!rt) {
            logger.error("render target not found");
            return {};
        }

        auto vs_src = m_rm->shader_loader().load(desc.shaders().vertex_shader_path, {tavros::renderer::shader_language::glsl_460, ""});
        auto fs_src = m_rm->shader_loader().load(desc.shaders().fragment_shader_path, {tavros::renderer::shader_language::glsl_460, ""});
        auto sh = m_rm->graphics_device()->create_shader({vs_src, fs_src});

        if (!sh) {
            logger.error("Failed to create material. Shader program is not compiled");
            return {};
        }

        const auto* reflect = m_rm->graphics_device()->get_shader_reflect_ptr(sh);


        rhi::pipeline_create_info info;

        // Shader program
        info.shader_program = sh;

        // Vertex attributes
        // info.bindings = // Vertex attributes

        // Color attachments
        // info.color_attachments = //
        /// Format of the attachment

        /*struct color_attachment_config
        {
            core::fixed_string<63> name = "";
            rhi::pixel_format      format = rhi::pixel_format::none;
            rhi::load_op           load = rhi::load_op::dont_care;
            rhi::store_op          store = rhi::store_op::dont_care;
            float                  clear_value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        };*/
        core::fixed_vector<rhi::color_attachment_state, rhi::k_max_color_attachments> ca;
        /*
        for (const auto& out : reflect->outputs()) {
            out.format
        }


        rt->first.color_attachments
        rhi::color_attachment_state cas;
        cas.format = ;
        cas.mask = ;
        cas.blend = ;

        rt->second.color_attachment_at

        pixel_format format = pixel_format::none;
        core::flags<color_mask> mask = k_rgba_color_mask;
        blend_state blend;*/

        /// Depth stencil
        info.depth_stencil_attachment.format = rhi::combine_depth_stencil_formats(rt->first.depth_attachment().format, rt->first.stencil_attachment().format);
        info.depth_stencil_attachment.depth_test_enable = desc.depth_attachment_state().test_enabled;
        info.depth_stencil_attachment.depth_write_enable = desc.depth_attachment_state().write_enabled;
        info.depth_stencil_attachment.depth_compare = desc.depth_attachment_state().compare_op;
        info.depth_stencil_attachment.stencil_test_enable = desc.stencil_attachment_state().test_enabled;
        info.depth_stencil_attachment.stencil_front = desc.stencil_attachment_state().front;
        info.depth_stencil_attachment.stencil_back = desc.stencil_attachment_state().back;

        // Topology
        info.topology = desc.topology().topology;

        // Rasterizer
        info.rasterizer = desc.rasterizer();

        // Multisample
        //info.multisample.sample_count = rt->first.multisample().sample_count;
        info.multisample.sample_shading_enabled = false;
        info.multisample.min_sample_shading = 1.0f;


        return {};
    }

    void material_registry::release_resource(gpu_material_view& mt) noexcept
    {
    }

} // namespace tavros::renderer
