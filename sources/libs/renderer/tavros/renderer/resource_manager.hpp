#pragma once

#include <tavros/renderer/texture/texture_registry.hpp>
#include <tavros/renderer/render_target/render_target_registry.hpp>
#include <tavros/renderer/material/material_registry.hpp>
#include <tavros/renderer/shaders/shader_loader.hpp>

namespace tavros::renderer
{

    class resource_manager : core::noncopyable, core::nonmovable
    {
    public:
        explicit resource_manager(rhi::graphics_device* gdevice, core::shared_ptr<assets::asset_manager> am) noexcept;

        ~resource_manager() noexcept = default;


        // ------------ textures ------------
        texture_handle load_texture(gpu_stage_buffer& stage, rhi::command_queue& cmd, core::string_view path, const texture_registry::load_params& params = {});

        texture_handle load_texture(gpu_stage_buffer& stage, rhi::command_queue& cmd, assets::image_view im, core::string_view key, const texture_registry::load_params& params = {});

        bool release(texture_handle h) noexcept;

        gpu_texture_view* find(texture_handle h) noexcept;

        const gpu_texture_view* find(texture_handle h) const noexcept;

        [[nodiscard]] rhi::texture_handle get_gpu_handle(texture_handle handle) const noexcept;

        texture_registry& textures() noexcept;

        const texture_registry& textures() const noexcept;


        // ------------ render targets ------------
        render_target_handle create_render_target(const tef::workspace& ws, core::string_view rt_path);

        render_target_handle create_render_target(core::string_view rt_name, const render_target_desc& desc);

        bool release(render_target_handle h) noexcept;

        render_target* find(render_target_handle h) noexcept;

        const render_target* find(render_target_handle h) const noexcept;

        render_target_registry& render_targets() noexcept;

        const render_target_registry& render_targets() const noexcept;


        // ------------ any other ------------
        assets::asset_manager& asset_manager() noexcept;

        const assets::asset_manager& asset_manager() const noexcept;

        renderer::shader_loader& shader_loader() noexcept;

        const renderer::shader_loader& shader_loader() const noexcept;

        rhi::graphics_device* graphics_device() noexcept;

    private:
        rhi::graphics_device*                   m_gdevice = nullptr;
        core::shared_ptr<assets::asset_manager> m_am;

        renderer::shader_loader m_sl;

        texture_registry       m_tex_reg;
        render_target_registry m_rt_reg;
        material_registry      m_mt_reg;
    };

} // namespace tavros::renderer

