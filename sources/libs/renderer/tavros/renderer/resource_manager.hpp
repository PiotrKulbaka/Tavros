#pragma once

#include <tavros/assets/asset_manager.hpp>
#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/renderer/upload_context.hpp>

#include <tavros/renderer/text/font/font_atlas.hpp>
#include <tavros/renderer/texture/texture.hpp>
#include <tavros/renderer/material/material.hpp>
#include <tavros/renderer/render_target/render_target.hpp>

namespace tavros::renderer
{

    /**
     * @brief Predefined sampler presets covering the most common texture sampling scenarios
     */
    enum class sampler_preset : uint8
    {
        /// Automatically selects an appropriate preset.
        /// Resolves to trilinear_repeat by default.
        automatic = 0,

        /// Nearest filtering, clamp to edge. Suitable for pixel art, UI elements, and render target reads.
        nearest_clamp,

        /// Nearest filtering, repeat wrap. Suitable for tiled textures without filtering.
        nearest_repeat,

        /// Nearest filtering, repeat mirrored wrap. Suitable for tiled textures without filtering.
        nearest_mirrored_repeat,

        /// Linear filtering, clamp to edge. Suitable for UI, fullscreen effects, and post-processing.
        linear_clamp,

        /// Linear filtering, repeat wrap. Suitable for generic textures without mipmaps.
        linear_repeat,

        /// Linear filtering, mirrored repeat wrap. Suitable for generic textures without mipmaps.
        linear_mirrored_repeat,

        /// Trilinear filtering, clamp to edge. Suitable for skyboxes, environment maps, and unique textures.
        trilinear_clamp,

        /// Trilinear filtering, repeat wrap. Default choice for most material textures.
        trilinear_repeat,

        /// Nearest filtering with depth comparison enabled. Suitable for shadow map sampling.
        shadow,

        /// Linear filtering with depth comparison enabled. Suitable for PCF shadow map sampling.
        shadow_pcf,

        /// Number of presets
        count,
    };

    enum class default_texture_type : uint8
    {
        white,
        black,
        transparent,
        normal,
        gray,
        debug,
    };

    class resource_manager final : core::noncopyable, core::nonmovable
    {
    public:
        resource_manager(rhi::graphics_device* gdevice, core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws);

        ~resource_manager() noexcept;

        void begin_frame() noexcept;
        void end_frame() noexcept;

        void set_material_load_params(core::buffer_view<const material::vertex_attribute> vert_attribs, uint32 msaa = 1, rhi::pixel_format ds_format = rhi::pixel_format::none) noexcept;

        rhi::texture_handle default_texture(default_texture_type type) const noexcept;

        rhi::texture_handle fonts_texture() const noexcept;

        font_ref load_font_from_file(core::string_view name, core::string_view path);
        font_ref load_font(core::string_view name);
        font_ref find_font(core::string_view name) noexcept;
        font_ref default_font() const noexcept;

        void release_font(font_ref fnt);

        texture_ref load_texture(core::string_view name);
        texture_ref create_texture(assets::image_view im, const texture_desc& desc);

        void release_texture(texture_ref tex);

        material_ref load_material(core::string_view name);
        material_ref create_material(const material_desc& desc);

        void release_material(material_ref mt);

        render_target_ref load_render_target(core::string_view name);
        render_target_ref create_render_target(const render_target_desc& desc);

        void release_render_target(render_target_ref rt);

        /**
         * @brief Returns a sampler handle for the specified preset.
         *
         * @param preset Predefined sampler preset to retrieve.
         * @return Handle to the corresponding sampler object.
         */
        rhi::sampler_handle sampler(sampler_preset preset) const noexcept;

    private:
        void update_font_atlas();

        void create_default_textures();
        void destroy_default_textures();

    private:
        using attribs_vec_t = core::fixed_vector<material::vertex_attribute, rhi::k_max_vertex_attributes>;
        using sampler_presets_vec_t = core::fixed_vector<rhi::sampler_handle, static_cast<size_t>(sampler_preset::count)>;

        float m_anisotropy = 8.0f; // for samplers

        bool m_is_init = false;
        bool m_flip_on_load; // flip textures vertically on load

        font_ref m_default_font;

        rhi::graphics_device*                   m_gdevice = nullptr;
        core::shared_ptr<assets::asset_manager> m_am;
        core::shared_ptr<tef::workspace>        m_ws;
        shader_loader                           m_sl;

        attribs_vec_t     m_mt_load_vert_attribs;
        uint32            m_mt_load_msaa;
        rhi::pixel_format m_mt_load_ds_format;

        rhi::texture_handle m_fonts_texture;

        rhi::texture_handle m_white_texture;
        rhi::texture_handle m_black_texture;
        rhi::texture_handle m_transparent_texture;
        rhi::texture_handle m_normal_texture;
        rhi::texture_handle m_gray_texture;
        rhi::texture_handle m_debug_texture;

        upload_context m_upctx;

        font_atlas m_fnt_atlas;

        core::resource_registry<font>          m_fnt_reg;
        core::resource_registry<texture>       m_tex_reg;
        core::resource_registry<material>      m_mt_reg;
        core::resource_registry<render_target> m_rt_reg;

        sampler_presets_vec_t m_samplers;
    };

} // namespace tavros::renderer
