#include <tavros/renderer/resource_manager.hpp>

namespace tavros::renderer
{

    resource_manager::resource_manager(rhi::graphics_device* gdevice, core::shared_ptr<assets::asset_manager> am) noexcept
        : m_gdevice(gdevice)
        , m_am(std::move(am))
        , m_tex_reg(this)
        , m_rt_reg(this)
    {
    }

    assets::asset_manager& resource_manager::asset_manager() noexcept
    {
        return *m_am;
    }

    const assets::asset_manager& resource_manager::asset_manager() const noexcept
    {
        return *m_am;
    }

    texture_handle resource_manager::load_texture(gpu_stage_buffer& stage, rhi::command_queue& cmd, core::string_view path, const texture_registry::load_params& params)
    {
        return m_tex_reg.load(stage, cmd, path, params);
    }

    texture_handle resource_manager::load_texture(gpu_stage_buffer& stage, rhi::command_queue& cmd, assets::image_view im, core::string_view key, const texture_registry::load_params& params)
    {
        return m_tex_reg.load(stage, cmd, im, key, params);
    }

    bool resource_manager::release(texture_handle h) noexcept
    {
        return m_tex_reg.release(h);
    }

    gpu_texture_view* resource_manager::find(texture_handle h) noexcept
    {
        return m_tex_reg.find(h);
    }

    const gpu_texture_view* resource_manager::find(texture_handle h) const noexcept
    {
        return m_tex_reg.find(h);
    }

    [[nodiscard]] rhi::texture_handle resource_manager::get_gpu_handle(texture_handle handle) const noexcept
    {
        return m_tex_reg.get_gpu_handle(handle);
    }

    texture_registry& resource_manager::textures() noexcept
    {
        return m_tex_reg;
    }

    const texture_registry& resource_manager::textures() const noexcept
    {
        return m_tex_reg;
    }

    render_target_handle resource_manager::create_render_target(const tef::workspace& ws, core::string_view rt_path)
    {
        return m_rt_reg.create(ws, rt_path);
    }

    render_target_handle resource_manager::create_render_target(core::string_view rt_name, const render_target_desc& desc)
    {
        return m_rt_reg.create(rt_name, desc);
    }

    bool resource_manager::release(render_target_handle h) noexcept
    {
        return m_rt_reg.release(h);
    }

    render_target* resource_manager::find(render_target_handle h) noexcept
    {
        return m_rt_reg.find(h);
    }

    const render_target* resource_manager::find(render_target_handle h) const noexcept
    {
        return m_rt_reg.find(h);
    }

    render_target_registry& resource_manager::render_targets() noexcept
    {
        return m_rt_reg;
    }

    const render_target_registry& resource_manager::render_targets() const noexcept
    {
        return m_rt_reg;
    }

    rhi::graphics_device* resource_manager::graphics_device() noexcept
    {
        return m_gdevice;
    }

} // namespace tavros::renderer
