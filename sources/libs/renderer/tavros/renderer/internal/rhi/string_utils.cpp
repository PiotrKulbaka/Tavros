#include <tavros/renderer/rhi/string_utils.hpp>

#include <tavros/core/debug/unreachable.hpp>

#include <tavros/renderer/rhi/buffer_info.hpp>
#include <tavros/renderer/rhi/texture_info.hpp>

namespace tavros::renderer::rhi
{

    core::string_view to_string(buffer_usage usage)
    {
        switch (usage) {
        case buffer_usage::stage:
            return "stage";
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

    core::string_view to_string(buffer_access access)
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

    core::string_view to_string(texture_type type)
    {
        switch (type) {
        case texture_type::texture_2d:
            return "texture_2d";
        case texture_type::texture_3d:
            return "texture_3d";
        case texture_type::texture_cube:
            return "texture_cube";
        default:
            TAV_UNREACHABLE();
        }
    }

} // namespace tavros::renderer::rhi
