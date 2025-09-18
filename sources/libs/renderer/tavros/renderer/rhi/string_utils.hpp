#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/renderer/rhi/buffer_info.hpp>

namespace tavros::renderer
{

    constexpr core::string_view to_string(buffer_usage usage)
    {
        switch (usage) {
        case buffer_usage::index:
            return "index";
        case buffer_usage::vertex:
            return "vertex";
        case buffer_usage::uniform:
            return "uniform";
        default:
            TAV_UNREACHABLE();
        }
    }

    constexpr core::string_view to_string(buffer_access access)
    {
        switch (access) {
        case buffer_access::gpu_only:
            return "gpu_only";
        case buffer_access::cpu_to_gpu:
            return "cpu_to_gpu";
        case buffer_access::gpu_to_cpu:
            return "gpu_to_cpu";
        default:
            TAV_UNREACHABLE();
        }
    }

} // namespace tavros::renderer
