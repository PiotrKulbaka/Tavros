#include <tavros/renderer/texture/sampler_library.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("sampler_library");

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

    constexpr std::array<const preset_info, tavros::renderer::sampler_library::k_max_presets> k_preset_infos = {
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

} // namespace

namespace tavros::renderer
{

    sampler_library::sampler_library(rhi::graphics_device* gdevice, float anisotropy)
        : m_gdevice(gdevice)
    {
        TAV_ASSERT(gdevice);

        for (size_t i = 0; i < k_preset_infos.size(); ++i) {
            auto info = k_preset_infos[i];
            TAV_ASSERT(static_cast<size_t>(info.sampler) == i);
            auto create_info = *info.preset;
            create_info.anisotropy = info.allow_anisotropy ? anisotropy : 1.0f;
            auto h = m_gdevice->create_sampler(create_info);
            if (!h) {
                logger.fatal("Failed to create sampler preset {}", i);
            }
            m_samplers.push_back(h);
        }
    }

    sampler_library::~sampler_library() noexcept
    {
        for (auto s : m_samplers) {
            m_gdevice->destroy_sampler(s);
        }
    }

    rhi::sampler_handle sampler_library::get_sampler(sampler_preset preset) const noexcept
    {
        return m_samplers[static_cast<uint32>(preset)];
    }

} // namespace tavros::renderer
