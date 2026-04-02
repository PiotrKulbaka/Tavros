#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>

namespace app::pipeline_builder
{

    namespace rhi = tavros::renderer::rhi;

    // -------------------------------------------------------------------------
    // Common depth-stencil presets
    // -------------------------------------------------------------------------

    inline rhi::depth_stencil_state depth_test_rw() noexcept
    {
        rhi::depth_stencil_state s;
        s.depth_test_enable = true;
        s.depth_write_enable = true;
        s.depth_compare = rhi::compare_op::less;
        return s;
    }

    inline rhi::depth_stencil_state depth_test_ro() noexcept
    {
        rhi::depth_stencil_state s;
        s.depth_test_enable = true;
        s.depth_write_enable = false;
        s.depth_compare = rhi::compare_op::less;
        return s;
    }

    inline rhi::depth_stencil_state depth_off() noexcept
    {
        rhi::depth_stencil_state s;
        s.depth_test_enable = false;
        s.depth_write_enable = false;
        return s;
    }

    inline rhi::depth_stencil_state depth_on_write_off() noexcept
    {
        rhi::depth_stencil_state s;
        s.depth_test_enable = true;
        s.depth_write_enable = false;
        s.depth_compare = rhi::compare_op::less;
        return s;
    }

    inline rhi::depth_stencil_state depth_test_leq() noexcept
    {
        rhi::depth_stencil_state s;
        s.depth_compare = rhi::compare_op::less_equal;
        s.depth_test_enable = true;
        s.depth_write_enable = false;
        return s;
    }

    // -------------------------------------------------------------------------
    // Common blend presets
    // -------------------------------------------------------------------------

    /// Standard alpha blending.
    inline rhi::blend_state alpha_blend() noexcept
    {
        return {true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask};
    }

    /// Additive blending (fire/glow effects).
    inline rhi::blend_state additive_blend() noexcept
    {
        return {true, rhi::blend_factor::src_alpha, rhi::blend_factor::one, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one, rhi::blend_op::add, rhi::k_rgba_color_mask};
    }

    /// No blending.
    inline rhi::blend_state no_blend() noexcept
    {
        return {false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask};
    }

    // -------------------------------------------------------------------------
    // Common rasterizer presets
    // -------------------------------------------------------------------------

    inline rhi::rasterizer_state cull_back_ccw() noexcept
    {
        rhi::rasterizer_state s;
        s.cull = rhi::cull_face::back;
        s.face = rhi::front_face::counter_clockwise;
        s.polygon = rhi::polygon_mode::fill;
        return s;
    }

    inline rhi::rasterizer_state cull_off() noexcept
    {
        rhi::rasterizer_state s;
        s.cull = rhi::cull_face::off;
        s.polygon = rhi::polygon_mode::fill;
        return s;
    }

    inline rhi::rasterizer_state cull_off_scissor() noexcept
    {
        auto s = cull_off();
        s.scissor_enable = true;
        return s;
    }

    // -------------------------------------------------------------------------
    // Default multisample (off)
    // -------------------------------------------------------------------------

    inline rhi::multisample_state no_msaa() noexcept
    {
        rhi::multisample_state s;
        s.sample_shading_enabled = false;
        s.sample_count = 1;
        s.min_sample_shading = 0.0f;
        return s;
    }

} // namespace app::pipeline_builder
