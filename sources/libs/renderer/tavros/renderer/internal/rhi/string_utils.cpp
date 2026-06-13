#include <tavros/renderer/rhi/string_utils.hpp>

#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

    enum class buffer_usage : uint8;
    enum class buffer_access : uint8;
    enum class index_buffer_format : uint8;
    enum class scalar_type : uint8;
    enum class composite_format : uint8;
    enum class primitive_topology : uint8;
    enum class compare_op : uint8;
    enum class stencil_op : uint8;
    enum class blend_factor : uint8;
    enum class blend_op : uint8;
    enum class color_mask : uint8;
    enum class cull_face : uint8;
    enum class front_face : uint8;
    enum class polygon_mode : uint8;
    enum class pixel_format : uint8;
    enum class texture_type : uint8;
    enum class texture_usage : uint8;
    enum class filter_mode : uint8;
    enum class mipmap_filter_mode : uint8;
    enum class wrap_mode : uint8;
    enum class shader_stage : uint8;
    enum class render_backend_type : uint8;
    enum class load_op : uint8;
    enum class store_op : uint8;

    core::string_view to_string(buffer_usage val) noexcept
    {
        switch (val) {
        case buffer_usage::stage:
            return "stage";
        case buffer_usage::index:
            return "index";
        case buffer_usage::vertex:
            return "vertex";
        case buffer_usage::constant:
            return "constant";
        case buffer_usage::storage:
            return "storage";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(buffer_access val) noexcept
    {
        switch (val) {
        case buffer_access::gpu_only:
            return "gpu_only";
        case buffer_access::cpu_to_gpu:
            return "cpu_to_gpu";
        case buffer_access::gpu_to_cpu:
            return "gpu_to_cpu";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(index_buffer_format val) noexcept
    {
        switch (val) {
        case index_buffer_format::u16:
            return "u16";
        case index_buffer_format::u32:
            return "u32";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(scalar_type format) noexcept
    {
        switch (format) {
        case scalar_type::u8:
            return "u8";
        case scalar_type::i8:
            return "i8";
        case scalar_type::u16:
            return "u16";
        case scalar_type::i16:
            return "i16";
        case scalar_type::u32:
            return "u32";
        case scalar_type::i32:
            return "i32";
        case scalar_type::f16:
            return "f16";
        case scalar_type::f32:
            return "f32";
        case scalar_type::f64:
            return "f64";
        default:
            TAV_UNREACHABLE();
        }
    }

    core::string_view to_string(composite_format type) noexcept
    {
        switch (type) {
        case composite_format::scalar:
            return "scalar";
        case composite_format::vec2:
            return "vec2";
        case composite_format::vec3:
            return "vec3";
        case composite_format::vec4:
            return "vec4";
        case composite_format::mat2:
            return "mat2";
        case composite_format::mat2x3:
            return "mat2x3";
        case composite_format::mat2x4:
            return "mat2x4";
        case composite_format::mat3x2:
            return "mat3x2";
        case composite_format::mat3:
            return "mat3";
        case composite_format::mat3x4:
            return "mat3x4";
        case composite_format::mat4x2:
            return "mat4x2";
        case composite_format::mat4x3:
            return "mat4x3";
        case composite_format::mat4:
            return "mat4";
        default:
            TAV_UNREACHABLE();
        }
    }

    core::string_view to_string(primitive_topology val) noexcept
    {
        switch (val) {
        case primitive_topology::points:
            return "points";
        case primitive_topology::lines:
            return "lines";
        case primitive_topology::line_strip:
            return "line_strip";
        case primitive_topology::triangles:
            return "triangles";
        case primitive_topology::triangle_strip:
            return "triangle_strip";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(compare_op val) noexcept
    {
        switch (val) {
        case compare_op::off:
            return "off";
        case compare_op::less:
            return "less";
        case compare_op::equal:
            return "equal";
        case compare_op::less_equal:
            return "less_equal";
        case compare_op::greater:
            return "greater";
        case compare_op::greater_equal:
            return "greater_equal";
        case compare_op::not_equal:
            return "not_equal";
        case compare_op::always:
            return "always";
        default:
            TAV_UNREACHABLE();
        }
    }

    core::string_view to_string(stencil_op val) noexcept
    {
        switch (val) {
        case stencil_op::keep:
            return "keep";
        case stencil_op::zero:
            return "zero";
        case stencil_op::replace:
            return "replace";
        case stencil_op::increment_clamp:
            return "increment_clamp";
        case stencil_op::decrement_clamp:
            return "decrement_clamp";
        case stencil_op::invert:
            return "invert";
        case stencil_op::increment_wrap:
            return "increment_wrap";
        case stencil_op::decrement_wrap:
            return "decrement_wrap";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(blend_factor val) noexcept
    {
        switch (val) {
        case blend_factor::zero:
            return "zero";
        case blend_factor::one:
            return "one";
        case blend_factor::src_color:
            return "src_color";
        case blend_factor::one_minus_src_color:
            return "one_minus_src_color";
        case blend_factor::dst_color:
            return "dst_color";
        case blend_factor::one_minus_dst_color:
            return "one_minus_dst_color";
        case blend_factor::src_alpha:
            return "src_alpha";
        case blend_factor::one_minus_src_alpha:
            return "one_minus_src_alpha";
        case blend_factor::dst_alpha:
            return "dst_alpha";
        case blend_factor::one_minus_dst_alpha:
            return "one_minus_dst_alpha";
            // case blend_factor::constant_color:           return "constant_color";
            // case blend_factor::one_minus_constant_color: return "one_minus_constant_color";
            // case blend_factor::constant_alpha:           return "constant_alpha";
            // case blend_factor::one_minus_constant_alpha: return "one_minus_constant_alpha";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(blend_op val) noexcept
    {
        switch (val) {
        case blend_op::add:
            return "add";
        case blend_op::subtract:
            return "subtract";
        case blend_op::reverse_subtract:
            return "reverse_subtract";
        case blend_op::min:
            return "min";
        case blend_op::max:
            return "max";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(color_mask val) noexcept
    {
        switch (val) {
        case color_mask::red:
            return "red";
        case color_mask::green:
            return "green";
        case color_mask::blue:
            return "blue";
        case color_mask::alpha:
            return "alpha";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(cull_face val) noexcept
    {
        switch (val) {
        case cull_face::off:
            return "off";
        case cull_face::front:
            return "front";
        case cull_face::back:
            return "back";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(front_face val) noexcept
    {
        switch (val) {
        case front_face::clockwise:
            return "clockwise";
        case front_face::counter_clockwise:
            return "counter_clockwise";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(polygon_mode val) noexcept
    {
        switch (val) {
        case polygon_mode::fill:
            return "fill";
        case polygon_mode::lines:
            return "lines";
        case polygon_mode::points:
            return "points";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(pixel_format val) noexcept
    {
        switch (val) {
        case pixel_format::none:
            return "none";

        case pixel_format::r8un:
            return "r8un";
        case pixel_format::r8in:
            return "r8in";
        case pixel_format::r16un:
            return "r16un";
        case pixel_format::r16in:
            return "r16in";

        case pixel_format::rg8un:
            return "rg8un";
        case pixel_format::rg8in:
            return "rg8in";
        case pixel_format::rg16un:
            return "rg16un";
        case pixel_format::rg16in:
            return "rg16in";

        case pixel_format::rgb8un:
            return "rgb8un";
        case pixel_format::rgb8in:
            return "rgb8in";
        case pixel_format::rgb16un:
            return "rgb16un";
        case pixel_format::rgb16in:
            return "rgb16in";

        case pixel_format::rgba8un:
            return "rgba8un";
        case pixel_format::rgba8in:
            return "rgba8in";
        case pixel_format::rgba16un:
            return "rgba16un";
        case pixel_format::rgba16in:
            return "rgba16in";

        case pixel_format::r8u:
            return "r8u";
        case pixel_format::r8i:
            return "r8i";
        case pixel_format::r16u:
            return "r16u";
        case pixel_format::r16i:
            return "r16i";
        case pixel_format::r32u:
            return "r32u";
        case pixel_format::r32i:
            return "r32i";

        case pixel_format::rg8u:
            return "rg8u";
        case pixel_format::rg8i:
            return "rg8i";
        case pixel_format::rg16u:
            return "rg16u";
        case pixel_format::rg16i:
            return "rg16i";
        case pixel_format::rg32u:
            return "rg32u";
        case pixel_format::rg32i:
            return "rg32i";

        case pixel_format::rgb8u:
            return "rgb8u";
        case pixel_format::rgb8i:
            return "rgb8i";
        case pixel_format::rgb16u:
            return "rgb16u";
        case pixel_format::rgb16i:
            return "rgb16i";
        case pixel_format::rgb32u:
            return "rgb32u";
        case pixel_format::rgb32i:
            return "rgb32i";

        case pixel_format::rgba8u:
            return "rgba8u";
        case pixel_format::rgba8i:
            return "rgba8i";
        case pixel_format::rgba16u:
            return "rgba16u";
        case pixel_format::rgba16i:
            return "rgba16i";
        case pixel_format::rgba32u:
            return "rgba32u";
        case pixel_format::rgba32i:
            return "rgba32i";

        case pixel_format::r16f:
            return "r16f";
        case pixel_format::r32f:
            return "r32f";
        case pixel_format::rg16f:
            return "rg16f";
        case pixel_format::rg32f:
            return "rg32f";
        case pixel_format::rgb16f:
            return "rgb16f";
        case pixel_format::rgb32f:
            return "rgb32f";
        case pixel_format::rgba16f:
            return "rgba16f";
        case pixel_format::rgba32f:
            return "rgba32f";

        case pixel_format::depth16:
            return "depth16";
        case pixel_format::depth24:
            return "depth24";
        case pixel_format::depth32f:
            return "depth32f";
        case pixel_format::stencil8:
            return "stencil8";
        case pixel_format::depth24_stencil8:
            return "depth24_stencil8";
        case pixel_format::depth32f_stencil8:
            return "depth32f_stencil8";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(texture_type val) noexcept
    {
        switch (val) {
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

    core::string_view to_string(texture_usage val) noexcept
    {
        switch (val) {
        case texture_usage::render_target:
            return "render_target";
        case texture_usage::sampled:
            return "sampled";
        case texture_usage::storage:
            return "storage";
        case texture_usage::transfer_source:
            return "transfer_source";
        case texture_usage::transfer_destination:
            return "transfer_destination";
        case texture_usage::resolve_source:
            return "resolve_source";
        case texture_usage::resolve_destination:
            return "resolve_destination";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(filter_mode val) noexcept
    {
        switch (val) {
        case filter_mode::nearest:
            return "nearest";
        case filter_mode::linear:
            return "linear";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(mipmap_filter_mode val) noexcept
    {
        switch (val) {
        case mipmap_filter_mode::off:
            return "off";
        case mipmap_filter_mode::nearest:
            return "nearest";
        case mipmap_filter_mode::linear:
            return "linear";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(wrap_mode val) noexcept
    {
        switch (val) {
        case wrap_mode::repeat:
            return "repeat";
        case wrap_mode::mirrored_repeat:
            return "mirrored_repeat";
        case wrap_mode::clamp_to_edge:
            return "clamp_to_edge";
        case wrap_mode::clamp_to_border:
            return "clamp_to_border";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(render_backend_type val) noexcept
    {
        switch (val) {
        case render_backend_type::opengl:
            return "opengl";
        case render_backend_type::vulkan:
            return "vulkan";
        case render_backend_type::directx12:
            return "directx12";
        case render_backend_type::metal:
            return "metal";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(load_op val) noexcept
    {
        switch (val) {
        case load_op::load:
            return "load";
        case load_op::clear:
            return "clear";
        case load_op::dont_care:
            return "dont_care";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(store_op val) noexcept
    {
        switch (val) {
        case store_op::store:
            return "store";
        case store_op::resolve:
            return "resolve";
        case store_op::dont_care:
            return "dont_care";
        }
        TAV_UNREACHABLE();
    }

    core::string_view to_string(shader_resource_type val) noexcept
    {
        switch (val) {
        case shader_resource_type::sampler_2d:
            return "sampler_2d";
        case shader_resource_type::sampler_2d_shadow:
            return "sampler_2d_shadow";
        case shader_resource_type::sampler_2d_array:
            return "sampler_2d_array";
        case shader_resource_type::sampler_2d_array_shadow:
            return "sampler_2d_array_shadow";
        case shader_resource_type::usampler_2d:
            return "usampler_2d";
        case shader_resource_type::usampler_2d_array:
            return "usampler_2d_array";
        case shader_resource_type::isampler_2d:
            return "isampler_2d";
        case shader_resource_type::isampler_2d_array:
            return "isampler_2d_array";
        case shader_resource_type::sampler_3d:
            return "sampler_3d";
        case shader_resource_type::usampler_3d:
            return "usampler_3d";
        case shader_resource_type::isampler_3d:
            return "isampler_3d";
        case shader_resource_type::sampler_cube:
            return "sampler_cube";
        case shader_resource_type::sampler_cube_shadow:
            return "sampler_cube_shadow";
        case shader_resource_type::sampler_cube_array:
            return "sampler_cube_array";
        case shader_resource_type::sampler_cube_array_shadow:
            return "sampler_cube_array_shadow";
        case shader_resource_type::usampler_cube:
            return "usampler_cube";
        case shader_resource_type::usampler_cube_array:
            return "usampler_cube_array";
        case shader_resource_type::isampler_cube:
            return "isampler_cube";
        case shader_resource_type::isampler_cube_array:
            return "isampler_cube_array";
        case shader_resource_type::sampler_buffer:
            return "sampler_buffer";
        case shader_resource_type::usampler_buffer:
            return "usampler_buffer";
        case shader_resource_type::isampler_buffer:
            return "isampler_buffer";
        case shader_resource_type::image_2d:
            return "image_2d";
        case shader_resource_type::image_2d_array:
            return "image_2d_array";
        case shader_resource_type::uimage_2d:
            return "uimage_2d";
        case shader_resource_type::uimage_2d_array:
            return "uimage_2d_array";
        case shader_resource_type::iimage_2d:
            return "iimage_2d";
        case shader_resource_type::iimage_2d_array:
            return "iimage_2d_array";
        case shader_resource_type::image_3d:
            return "image_3d";
        case shader_resource_type::uimage_3d:
            return "uimage_3d";
        case shader_resource_type::iimage_3d:
            return "iimage_3d";
        case shader_resource_type::image_cube:
            return "image_cube";
        case shader_resource_type::image_cube_array:
            return "image_cube_array";
        case shader_resource_type::uimage_cube:
            return "uimage_cube";
        case shader_resource_type::uimage_cube_array:
            return "uimage_cube_array";
        case shader_resource_type::iimage_cube:
            return "iimage_cube";
        case shader_resource_type::iimage_cube_array:
            return "iimage_cube_array";
        case shader_resource_type::image_buffer:
            return "image_buffer";
        case shader_resource_type::uimage_buffer:
            return "uimage_buffer";
        case shader_resource_type::iimage_buffer:
            return "iimage_buffer";
        }
        TAV_UNREACHABLE();
    }

    pixel_format combine_depth_stencil_formats(pixel_format df, pixel_format sf) noexcept
    {
        TAV_ASSERT(df == pixel_format::depth24 || df == pixel_format::depth32f || df == pixel_format::none);
        TAV_ASSERT(sf == pixel_format::stencil8 || sf == pixel_format::none);

        if (df == pixel_format::depth24) {
            if (sf == pixel_format::stencil8) {
                return pixel_format::depth24_stencil8;
            } else if (sf == pixel_format::none) {
                return pixel_format::depth24;
            }
        } else if (df == pixel_format::depth32f) {
            if (sf == pixel_format::stencil8) {
                return pixel_format::depth32f_stencil8;
            } else if (sf == pixel_format::none) {
                return pixel_format::depth32f;
            }
        } else if (df == pixel_format::none) {
            if (sf == pixel_format::stencil8) {
                return pixel_format::stencil8;
            } else if (sf == pixel_format::none) {
                return pixel_format::none;
            }
        }
        TAV_ASSERT(false);
        return pixel_format::none;
    }

} // namespace tavros::renderer::rhi
