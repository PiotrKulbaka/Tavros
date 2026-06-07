#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/fixed_string.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/tef/workspace.hpp>
#include <tavros/tef/schema.hpp>

namespace tavros::renderer
{

    struct material_config
    {
        struct shaders_config
        {
            core::fixed_path vertex_shader_path = "";
            core::fixed_path fragment_shader_path = "";
        };

        struct color_attachment_config
        {
            core::string64               name = "";
            core::flags<rhi::color_mask> mask = rhi::k_rgba_color_mask;
            rhi::blend_state             blend;
        };

        using color_attachments_config = core::fixed_vector<color_attachment_config, rhi::k_max_color_attachments>;

        struct depth_attachment_config
        {
            bool            test_enabled = false;
            bool            write_enabled = false;
            rhi::compare_op depth_compare = rhi::compare_op::off;
        };

        struct stencil_attachment_config
        {
            rhi::stencil_state front;
            rhi::stencil_state back;
        };

        struct topology_config
        {
            rhi::primitive_topology topology = rhi::primitive_topology::triangles;
        };

        struct rasterizer_config
        {
            rhi::cull_face    cull = rhi::cull_face::off;
            rhi::front_face   face = rhi::front_face::counter_clockwise;
            rhi::polygon_mode fill_mode = rhi::polygon_mode::fill;
            bool              depth_clamp_enabled = false;
            float             depth_clamp_min = 0.0f;
            float             depth_clamp_max = 1.0f;
            bool              depth_bias_enabled = false;
            float             depth_bias_constant = 0.0f;
            float             depth_bias_slope = 0.0f;
            float             depth_bias_clamp = 0.0f;
        };

        shaders_config            shaders;
        color_attachments_config  color;
        depth_attachment_config   depth;
        stencil_attachment_config stencil;
        topology_config           topology;
        rasterizer_config         rasterizer;
    };

} // namespace tavros::renderer

namespace tavros::tef
{
    template<>
    struct schema<tavros::renderer::material_config>
    {
        static void serialize(node* n, const tavros::renderer::material_config& in, core::diagnostics& ds) noexcept;
        static void deserialize(const node* n, tavros::renderer::material_config& out, core::diagnostics& ds) noexcept;
    };
} // namespace tavros::tef

namespace tavros::renderer
{


    /*

    first_material = {
        shader = {              # Currently, only two types of shaders are supported
            vertex = ""                 # Path to the vertex shader source
            fragment = ""               # Path to the fragment shader source
        }

        color = {   # up to 8 color attachments
            base_color = {                  # key - The name must match the name of the attachment in the render target. "off" or {mask = "" blend = STRING|OBJECT}
                mask = "rgba"               # write mask "r" "g" "b" "a" "rg" "rb" "ra" "gb" "ga" "ba" "rgb" "rga" "rba" "gba" "rgba"
                blend = {                               # "off" "alpha" "additive" "multiply" "min" "max" "premul_alpha" or {src_color = STRING dst_color = STRING color_op = STRING src_alpha = STRING dst_alpha = STRING alpha_op = STRING}
                    src_color = "src_alpha"             # "zero" "one" "src_color" "one_minus_src_color" "dst_color" "one_minus_dst_color" "src_alpha" "one_minus_src_alpha" "dst_alpha" "one_minus_dst_alpha"
                    dst_color = "one_minus_src_alpha"   # "zero" "one" "src_color" "one_minus_src_color" "dst_color" "one_minus_dst_color" "src_alpha" "one_minus_src_alpha" "dst_alpha" "one_minus_dst_alpha"
                    color_op = "add"                    # "add" "subtract" "reverse_subtract" "min" "max"
                    src_alpha = "one"                   # "zero" "one" "src_color" "one_minus_src_color" "dst_color" "one_minus_dst_color" "src_alpha" "one_minus_src_alpha" "dst_alpha" "one_minus_dst_alpha"
                    dst_alpha = "one_minus_src_alpha"   # "zero" "one" "src_color" "one_minus_src_color" "dst_color" "one_minus_dst_color" "src_alpha" "one_minus_src_alpha" "dst_alpha" "one_minus_dst_alpha"
                    alpha_op = "add"                    # "add" "subtract" "reverse_subtract" "min" "max"
                }
            }
            # normal = {...} etc...                     # - The name must match the name of the attachment in the render target.
        }

        depth = {                           # "off" or {test = BOOL write = BOOL compare_op = STRING}
            test = true                     # true false
            write = true                    # true false
            compare_op = "less"             # "less" "equal" "less_equal" "greater" "greater_equal" "not_equal" "always"
        }

        stencil = {                         # "off" or {front = STRING|OBJECT back = STRING|OBJECT}
            front = {                       # "off" or {read_mask = NUMBER write_mask = NUMBER ref_value = NUMBER compare_op = STRING stencil_fail_op = STRING depth_fail_op = STRING pass_op = STRING}
                read_mask = 0xFF            # Read mask [0..255]
                write_mask = 0xFF           # Write mask [0..255]
                ref_value = 0               # Reference value for stencil testing
                compare_op = "always"       # "less" "equal" "less_equal" "greater" "greater_equal" "not_equal" "always"
                stencil_fail_op = "keep"    # "keep" "zero" "replace" "increment_clamp" "decrement_clamp" "invert" "increment_wrap" "decrement_wrap"
                depth_fail_op = "keep"      # "keep" "zero" "replace" "increment_clamp" "decrement_clamp" "invert" "increment_wrap" "decrement_wrap"
                pass_op = "keep"            # "keep" "zero" "replace" "increment_clamp" "decrement_clamp" "invert" "increment_wrap" "decrement_wrap"
            }
            back = {                        # "off" or {read_mask = NUMBER write_mask = NUMBER ref_value = NUMBER compare_op = STRING stencil_fail_op = STRING depth_fail_op = STRING pass_op = STRING}
                read_mask = 0xFF            # Read mask [0..255]
                write_mask = 0xFF           # Write mask [0..255]
                ref_value = 0               # Reference value for stencil testing
                compare_op = "always"       # "less" "equal" "less_equal" "greater" "greater_equal" "not_equal" "always"
                stencil_fail_op = "keep"    # "keep" "zero" "replace" "increment_clamp" "decrement_clamp" "invert" "increment_wrap" "decrement_wrap"
                depth_fail_op = "keep"      # "keep" "zero" "replace" "increment_clamp" "decrement_clamp" "invert" "increment_wrap" "decrement_wrap"
                pass_op = "keep"            # "keep" "zero" "replace" "increment_clamp" "decrement_clamp" "invert" "increment_wrap" "decrement_wrap"
            }
        }

        topology = "triangles"              # "points" "lines" "line_strip" "triangles" "triangle_strip"

        rasterizer = {                      # {cull = STRING face = STRING polygon = STRING depth_clamp = STRING|OBJECT depth_bias = STRING|OBJECT scissor = STRING}
            cull = "off"                    # "off" "front" "back"
            front_face = "ccw"              # "cw" "ccw"
            fill_mode = "fill"              # "fill" "lines" "points"
            depth_clamp = {                 # "off" or {min = NUMBER max = NUMBER}
                min = 0.0
                max = 1.0
            }
            depth_bias = {                  # "off" or {constant = NUMBER slope = NUMBER clamp = NUMBER}
                constant = 0.0
                slope = 0.0
                clamp = 0.0
            }
        }
    }
    */

    class material
    {
    public:
        static core::unique_ptr<material> create(rhi::graphics_device* gdevice, const tef::workspace& ws, core::string_view mt_path);

    public:
        material(rhi::graphics_device* gdevice, core::string_view material_name, const material_config& cfg) noexcept;
        ~material() noexcept;

        core::string_view name() const noexcept;

        core::string_view vertex_shader_path() const noexcept;

        core::string_view fragment_shader_path() const noexcept;


        rhi::pipeline_handle pipeline() const noexcept
        {
            return m_pipeline;
        }

    private:
        rhi::pipeline_handle m_pipeline = {};
    };

    class material_instance_

} // namespace tavros::renderer