#include <tavros/renderer/rhi/string_utils.hpp>

#include <tavros/core/debug/unreachable.hpp>

#include <tavros/renderer/rhi/buffer_info.hpp>
#include <tavros/renderer/rhi/texture_info.hpp>
#include <tavros/renderer/rhi/vertex_attribute.hpp>

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

    core::string_view to_string(attribute_type type)
    {
        switch (type) {
        case attribute_type::scalar:
            return "scalar";
        case attribute_type::vec2:
            return "vec2";
        case attribute_type::vec3:
            return "vec3";
        case attribute_type::vec4:
            return "vec4";
        case attribute_type::mat2:
            return "mat2";
        case attribute_type::mat2x3:
            return "mat2x3";
        case attribute_type::mat2x4:
            return "mat2x4";
        case attribute_type::mat3x2:
            return "mat3x2";
        case attribute_type::mat3:
            return "mat3";
        case attribute_type::mat3x4:
            return "mat3x4";
        case attribute_type::mat4x2:
            return "mat4x2";
        case attribute_type::mat4x3:
            return "mat4x3";
        case attribute_type::mat4:
            return "mat4";
        default:
            TAV_UNREACHABLE();
        }
    }

    core::string_view to_string(attribute_format format)
    {
        switch (format) {
        case attribute_format::u8:
            return "u8";
        case attribute_format::i8:
            return "i8";
        case attribute_format::u16:
            return "u16";
        case attribute_format::i16:
            return "i16";
        case attribute_format::u32:
            return "u32";
        case attribute_format::i32:
            return "i32";
        case attribute_format::f16:
            return "f16";
        case attribute_format::f32:
            return "f32";
        case attribute_format::f64:
            return "f64";
        default:
            TAV_UNREACHABLE();
        }
    }

} // namespace tavros::renderer::rhi
