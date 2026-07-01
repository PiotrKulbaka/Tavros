#include <tavros/renderer/resource_manager.hpp>

#include <tavros/core/exception.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/renderer/shaders/shader_source_provider.hpp>
#include <tavros/renderer/texture/texture_uploader.hpp>
#include <tavros/renderer/text/font/truetype_font.hpp>

namespace
{
    tavros::core::logger logger("resource_manager");

    namespace rhi = tavros::renderer::rhi;

    using smp_preset_t = tavros::renderer::sampler_preset;
    using smp_create_info = rhi::sampler_create_info;

    constexpr smp_create_info k_nearest_clamp = {
        .filter = {rhi::filter_mode::nearest, rhi::filter_mode::nearest, rhi::mipmap_filter_mode::off},
        .wrap_mode = {rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge},
    };

    constexpr smp_create_info k_nearest_repeat = {
        .filter = {rhi::filter_mode::nearest, rhi::filter_mode::nearest, rhi::mipmap_filter_mode::off},
        .wrap_mode = {rhi::wrap_mode::repeat, rhi::wrap_mode::repeat, rhi::wrap_mode::repeat},
    };

    constexpr smp_create_info k_linear_clamp = {
        .filter = {rhi::filter_mode::linear, rhi::filter_mode::linear, rhi::mipmap_filter_mode::off},
        .wrap_mode = {rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge},
    };

    constexpr smp_create_info k_linear_repeat = {
        .filter = {rhi::filter_mode::linear, rhi::filter_mode::linear, rhi::mipmap_filter_mode::off},
        .wrap_mode = {rhi::wrap_mode::repeat, rhi::wrap_mode::repeat, rhi::wrap_mode::repeat},
    };

    constexpr smp_create_info k_trilinear_clamp = {
        .filter = {rhi::filter_mode::linear, rhi::filter_mode::linear, rhi::mipmap_filter_mode::linear},
        .wrap_mode = {rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge},
    };

    constexpr smp_create_info k_trilinear_repeat = {
        .filter = {rhi::filter_mode::linear, rhi::filter_mode::linear, rhi::mipmap_filter_mode::linear},
        .wrap_mode = {rhi::wrap_mode::repeat, rhi::wrap_mode::repeat, rhi::wrap_mode::repeat},
    };

    constexpr smp_create_info k_shadow = {
        .filter = {rhi::filter_mode::nearest, rhi::filter_mode::nearest, rhi::mipmap_filter_mode::off},
        .wrap_mode = {rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge},
        .depth_compare = rhi::compare_op::less_equal,
    };

    constexpr smp_create_info k_shadow_pcf = {
        .filter = {rhi::filter_mode::linear, rhi::filter_mode::linear, rhi::mipmap_filter_mode::off},
        .wrap_mode = {rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge, rhi::wrap_mode::clamp_to_edge},
        .depth_compare = rhi::compare_op::less_equal,
    };

    struct preset_info
    {
        smp_preset_t           sampler = smp_preset_t::automatic;
        bool                   allow_anisotropy = false;
        const smp_create_info* preset = nullptr;
    };

    constexpr std::array<const preset_info, static_cast<size_t>(smp_preset_t::count)> k_preset_infos = {
        preset_info{smp_preset_t::automatic, true, &k_trilinear_repeat},
        preset_info{smp_preset_t::nearest_clamp, false, &k_nearest_clamp},
        preset_info{smp_preset_t::nearest_repeat, false, &k_nearest_repeat},
        preset_info{smp_preset_t::linear_clamp, true, &k_linear_clamp},
        preset_info{smp_preset_t::linear_repeat, true, &k_linear_repeat},
        preset_info{smp_preset_t::trilinear_clamp, true, &k_trilinear_clamp},
        preset_info{smp_preset_t::trilinear_repeat, true, &k_trilinear_repeat},
        preset_info{smp_preset_t::shadow, false, &k_shadow},
        preset_info{smp_preset_t::shadow_pcf, false, &k_shadow_pcf},
    };

    tavros::assets::image::pixel_format to_im_format(tavros::renderer::rhi::pixel_format fmt) noexcept
    {
        using ipf = tavros::assets::image::pixel_format;
        using rpf = tavros::renderer::rhi::pixel_format;
        switch (fmt) {
        case rpf::r8un:
            return ipf::r8;
        case rpf::rg8un:
            return ipf::rg8;
        case rpf::rgb8un:
            return ipf::rgb8;
        case rpf::rgba8un:
            return ipf::rgba8;
        case rpf::none:
            return ipf::none;
        default:
            return ipf::none;
        }
    }


    class shader_provider : public tavros::renderer::shader_source_provider
    {
    public:
        explicit shader_provider(tavros::assets::asset_manager* am)
            : m_am(am)
        {
        }

        tavros::core::string load(tavros::core::string_view path) override
        {
            return m_am->read_text(path);
        }

    private:
        tavros::assets::asset_manager* m_am;
    };


    class font_placeholder final : public tavros::renderer::font
    {
    public:
        font_placeholder(tavros::renderer::font_atlas* atlas)
            : m_atlas(atlas)
        {
            m_atlas->register_font(this);
        }

        ~font_placeholder() noexcept override
        {
            m_atlas->unreg_font(this);
        }

        tavros::math::isize2 glyph_bitmap_size_internal(glyph_index, float, float) const noexcept override
        {
            return tavros::math::isize2(0, 0);
        }

        void bake_glyph_bitmap_internal(glyph_index, float, float, tavros::core::buffer_span<uint8>, uint32) const noexcept override
        {
        }

        float get_kerning_internal(char32 cp1, char32 cp2) const noexcept override
        {
            return 0.0f;
        }

    private:
        tavros::renderer::font_atlas* m_atlas;
    };

} // namespace

namespace tavros::renderer
{

    resource_manager::resource_manager(rhi::graphics_device* gdevice, core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws)
        : m_gdevice(gdevice)
        , m_upctx(gdevice)
        , m_am(am)
        , m_ws(ws)
        , m_sl(std::move(core::make_unique<shader_provider>(am.get())))
    {
        set_material_load_params({});

        if (!m_tex_placeholder) {
            uint8      data[] = {255, 0, 255};
            const auto im = assets::image_view(data, 1, 1, assets::image::pixel_format::rgb8);
            const auto desc = texture_desc{"$placeholder", texture_desc::texture_load_params{.width = 1, .height = 1}};
            m_tex_placeholder = core::make_unique<texture>(m_gdevice, m_upctx, im, desc, true);
            m_tex_reg.set_placeholder(m_tex_placeholder.get());
        }

        for (size_t i = 0; i < k_preset_infos.size(); ++i) {
            auto info = k_preset_infos[i];
            TAV_ASSERT(static_cast<size_t>(info.sampler) == i);
            auto create_info = *info.preset;
            create_info.anisotropy = info.allow_anisotropy ? m_anisotropy : 1.0f;
            auto h = m_gdevice->create_sampler(create_info);
            if (!h) {
                logger.fatal("Failed to create sampler preset {}", i);
            }
            m_samplers.push_back(h);
        }
    }

    resource_manager::~resource_manager() noexcept
    {
        for (auto s : m_samplers) {
            m_gdevice->destroy_sampler(s);
        }
    }

    void resource_manager::begin_frame() noexcept
    {
        m_fnt_reg.sync();
        m_tex_reg.sync();
        m_mt_reg.sync();
        m_rt_reg.sync();

        if (m_fnt_atlas.need_to_recreate_atlas()) {
            auto atlas = m_fnt_atlas.invalidate_old_and_bake_new_atlas(96.0f, 8.0f);

            auto tex = m_gdevice->create_texture(rhi::texture_create_info{.type = rhi::texture_type::texture_2d, .format = rhi::pixel_format::r8un, .width = atlas.width(), .height = atlas.height(), .usage = rhi::k_default_texture_usage});
            texture_uploader::upload_2d(tex, atlas, {}, 0, m_upctx);

            if (m_fonts_texture) {
                m_gdevice->destroy_texture(m_fonts_texture);
            }
            m_fonts_texture = tex;
        }

        m_upctx.flush();
    }

    void resource_manager::end_frame() noexcept
    {
    }

    void resource_manager::set_material_load_params(core::buffer_view<material::vertex_attribute> vert_attribs, uint32 msaa, rhi::pixel_format ds_format) noexcept
    {
        m_mt_load_vert_attribs = vert_attribs;
        m_mt_load_msaa = msaa;
        m_mt_load_ds_format = ds_format;
    }

    rhi::texture_handle resource_manager::fonts_texture() const noexcept
    {
        return m_fonts_texture;
    }

    font_ref resource_manager::load_font(core::string_view name)
    {
        if (!m_fnt_placeholder) {
            m_fnt_placeholder = core::make_unique<font_placeholder>(&m_fnt_atlas);
            m_fnt_reg.set_placeholder(m_fnt_placeholder.get());
        }
        auto slot = m_fnt_reg.make_slot(name);

        if (slot.second) {
            font_desc         desc;
            core::diagnostics ds;

            tavros::tef::schema<font_desc>::deserialize(m_ws->resolve_path(name), desc, ds);
            if (ds.error_count() > 0 || ds.fatal_count() > 0) {
                logger.error("Failed to load named font '{}'", name);
                logger.flush(ds);
                m_fnt_reg.publish_failed(slot.first);
                m_fnt_reg.sync();
                return slot.first;
            }

            if (ds.total_count() > 0) {
                logger.flush(ds);
            }

            core::dynamic_buffer<uint8> font_data;
            try {
                font_data = m_am->read_binary(desc.path());
            } catch (const core::file_error& e) {
                logger.error("Failed to open font '{}'", desc.path());
                m_fnt_reg.publish_failed(slot.first);
                m_fnt_reg.sync();
                return slot.first;
            }

            auto fnt = core::make_unique<truetype_font>(&m_fnt_atlas, std::move(font_data), desc.codepoint_ranges());
            m_fnt_reg.publish(slot.first, std::move(fnt));
            m_fnt_reg.sync();
        }

        return slot.first;
    }

    void resource_manager::release_font(font_ref fnt)
    {
        m_fnt_reg.release(fnt);
    }

    texture_ref resource_manager::load_texture(core::string_view name)
    {
        auto slot = m_tex_reg.make_slot(name);

        if (slot.second) {
            texture_desc      desc;
            core::diagnostics ds;

            tavros::tef::schema<texture_desc>::deserialize(m_ws->resolve_path(name), desc, ds);
            if (ds.error_count() > 0 || ds.fatal_count() > 0) {
                logger.error("Failed to load named texture '{}'", name);
                logger.flush(ds);
                m_tex_reg.publish_failed(slot.first);
                return slot.first;
            }

            if (ds.total_count() > 0) {
                logger.flush(ds);
            }

            // Decode image on CPU
            assets::image im;
            try {
                auto im_fmt = to_im_format(desc.load_params().pixel_format);
                auto data = m_am->read_binary(desc.load_params().path);
                im = assets::image::decode(data, im_fmt, true);
            } catch (const core::file_error& e) {
                logger.error("Failed to open image '{}'", desc.load_params().path);
                return slot.first;
            }

            auto tex = core::make_unique<texture>(m_gdevice, m_upctx, im, desc, true);
            m_tex_reg.publish(slot.first, std::move(tex));
        }

        return slot.first;
    }

    texture_ref resource_manager::create_texture(assets::image_view im, const texture_desc& desc)
    {
        auto slot = m_tex_reg.make_slot(desc.name());

        if (slot.second) {
            auto tex = core::make_unique<texture>(m_gdevice, m_upctx, im, desc, true);
            m_tex_reg.publish(slot.first, std::move(tex));
        }

        return slot.first;
    }

    void resource_manager::release_texture(texture_ref tex)
    {
        m_tex_reg.release(tex);
    }

    material_ref resource_manager::load_material(core::string_view name)
    {
        auto slot = m_mt_reg.make_slot(name);

        if (slot.second) {
            material_desc     desc;
            core::diagnostics ds;

            tavros::tef::schema<material_desc>::deserialize(m_ws->resolve_path(name), desc, ds);
            if (ds.error_count() > 0 || ds.fatal_count() > 0) {
                logger.error("Failed to load material '{}'", name);
                logger.flush(ds);
                m_mt_reg.publish_failed(slot.first);
                return slot.first;
            }

            if (ds.total_count() > 0) {
                logger.flush(ds);
            }

            auto mt = core::make_unique<material>(m_gdevice, desc, m_sl, m_mt_load_vert_attribs, m_mt_load_msaa, m_mt_load_ds_format);
            m_mt_reg.publish(slot.first, std::move(mt));
        }

        return slot.first;
    }

    material_ref resource_manager::create_material(const material_desc& desc)
    {
        auto slot = m_mt_reg.make_slot(desc.name());

        if (slot.second) {
            auto mt = core::make_unique<material>(m_gdevice, desc, m_sl, m_mt_load_vert_attribs, m_mt_load_msaa, m_mt_load_ds_format);
            m_mt_reg.publish(slot.first, std::move(mt));
        }

        return slot.first;
    }

    void resource_manager::release_material(material_ref mt)
    {
        m_mt_reg.release(mt);
    }

    render_target_ref resource_manager::load_render_target(core::string_view name)
    {
        auto slot = m_rt_reg.make_slot(name);

        if (slot.second) {
            render_target_desc desc;
            core::diagnostics  ds;

            tavros::tef::schema<render_target_desc>::deserialize(m_ws->resolve_path(name), desc, ds);
            if (ds.error_count() > 0 || ds.fatal_count() > 0) {
                logger.error("Failed to load named render_target '{}'", name);
                logger.flush(ds);
                m_rt_reg.publish_failed(slot.first);
                return slot.first;
            }

            if (ds.total_count() > 0) {
                logger.flush(ds);
            }

            auto rt = core::make_unique<render_target>(m_gdevice, desc);
            m_rt_reg.publish(slot.first, std::move(rt));
            m_rt_reg.sync();
        }

        return slot.first;
    }

    render_target_ref resource_manager::create_render_target(const render_target_desc& desc)
    {
        auto slot = m_rt_reg.make_slot(desc.name());

        if (slot.second) {
            auto rt = core::make_unique<render_target>(m_gdevice, desc);
            m_rt_reg.publish(slot.first, std::move(rt));
            m_rt_reg.sync();
        }

        return slot.first;
    }

    void resource_manager::release_render_target(render_target_ref rt)
    {
        m_rt_reg.release(rt);
    }

    rhi::sampler_handle resource_manager::sampler(sampler_preset preset) const noexcept
    {
        return m_samplers[static_cast<uint32>(preset)];
    }

} // namespace tavros::renderer
