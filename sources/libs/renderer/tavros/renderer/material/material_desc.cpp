#include <tavros/renderer/material/material_desc.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/logger/diagnostics.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/tef/helpers.hpp>

namespace
{

    tavros::core::logger logger("material_desc");

    using diagnostics = tavros::core::diagnostics;
    using shader_config = tavros::renderer::material_desc::shader_config;
    using color_attachment_state_config = tavros::renderer::material_desc::color_attachment_state_config;
    using color_attachments_state_config = tavros::renderer::material_desc::color_attachments_state_config;
    using depth_attachment_state_config = tavros::renderer::material_desc::depth_attachment_state_config;
    using stencil_attachment_state_config = tavros::renderer::material_desc::stencil_attachment_state_config;
    using topology_config = tavros::renderer::material_desc::topology_config;
    using rasterizer_config = tavros::renderer::material_desc::rasterizer_config;
    using material_desc = tavros::renderer::material_desc;

    // -----------------------------------------------------------------------
    //  Enum converters
    // -----------------------------------------------------------------------

    std::optional<tavros::renderer::rhi::blend_factor> to_blend_factor(tavros::core::string_view sv) noexcept
    {
        using bf = tavros::renderer::rhi::blend_factor;
        if (sv == "zero") {
            return bf::zero;
        }
        if (sv == "one") {
            return bf::one;
        }
        if (sv == "src_color") {
            return bf::src_color;
        }
        if (sv == "one_minus_src_color") {
            return bf::one_minus_src_color;
        }
        if (sv == "dst_color") {
            return bf::dst_color;
        }
        if (sv == "one_minus_dst_color") {
            return bf::one_minus_dst_color;
        }
        if (sv == "src_alpha") {
            return bf::src_alpha;
        }
        if (sv == "one_minus_src_alpha") {
            return bf::one_minus_src_alpha;
        }
        if (sv == "dst_alpha") {
            return bf::dst_alpha;
        }
        if (sv == "one_minus_dst_alpha") {
            return bf::one_minus_dst_alpha;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::blend_op> to_blend_op(tavros::core::string_view sv) noexcept
    {
        using bo = tavros::renderer::rhi::blend_op;
        if (sv == "add") {
            return bo::add;
        }
        if (sv == "subtract") {
            return bo::subtract;
        }
        if (sv == "reverse_subtract") {
            return bo::reverse_subtract;
        }
        if (sv == "min") {
            return bo::min;
        }
        if (sv == "max") {
            return bo::max;
        }
        return std::nullopt;
    }

    std::optional<tavros::core::flags<tavros::renderer::rhi::color_mask>> to_color_mask(tavros::core::string_view sv) noexcept
    {
        using m = tavros::renderer::rhi::color_mask;
        if (sv == "r") {
            return m::red;
        }
        if (sv == "g") {
            return m::green;
        }
        if (sv == "b") {
            return m::blue;
        }
        if (sv == "a") {
            return m::alpha;
        }
        if (sv == "rg") {
            return m::red | m::green;
        }
        if (sv == "rb") {
            return m::red | m::blue;
        }
        if (sv == "ra") {
            return m::red | m::alpha;
        }
        if (sv == "gb") {
            return m::green | m::blue;
        }
        if (sv == "ga") {
            return m::green | m::alpha;
        }
        if (sv == "ba") {
            return m::blue | m::alpha;
        }
        if (sv == "rgb") {
            return m::red | m::green | m::blue;
        }
        if (sv == "rga") {
            return m::red | m::green | m::alpha;
        }
        if (sv == "rba") {
            return m::red | m::blue | m::alpha;
        }
        if (sv == "gba") {
            return m::green | m::blue | m::alpha;
        }
        if (sv == "rgba") {
            return m::red | m::green | m::blue | m::alpha;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::compare_op> to_compare_op(tavros::core::string_view sv) noexcept
    {
        using co = tavros::renderer::rhi::compare_op;
        if (sv == "less") {
            return co::less;
        }
        if (sv == "equal") {
            return co::equal;
        }
        if (sv == "less_equal") {
            return co::less_equal;
        }
        if (sv == "greater") {
            return co::greater;
        }
        if (sv == "greater_equal") {
            return co::greater_equal;
        }
        if (sv == "not_equal") {
            return co::not_equal;
        }
        if (sv == "always") {
            return co::always;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::stencil_op> to_stencil_op(tavros::core::string_view sv) noexcept
    {
        using so = tavros::renderer::rhi::stencil_op;
        if (sv == "keep") {
            return so::keep;
        }
        if (sv == "zero") {
            return so::zero;
        }
        if (sv == "replace") {
            return so::replace;
        }
        if (sv == "increment_clamp") {
            return so::increment_clamp;
        }
        if (sv == "decrement_clamp") {
            return so::decrement_clamp;
        }
        if (sv == "invert") {
            return so::invert;
        }
        if (sv == "increment_wrap") {
            return so::increment_wrap;
        }
        if (sv == "decrement_wrap") {
            return so::decrement_wrap;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::primitive_topology> to_topology(tavros::core::string_view sv) noexcept
    {
        using t = tavros::renderer::rhi::primitive_topology;
        if (sv == "points") {
            return t::points;
        }
        if (sv == "lines") {
            return t::lines;
        }
        if (sv == "line_strip") {
            return t::line_strip;
        }
        if (sv == "triangles") {
            return t::triangles;
        }
        if (sv == "triangle_strip") {
            return t::triangle_strip;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::cull_face> to_cull_face(tavros::core::string_view sv) noexcept
    {
        using cf = tavros::renderer::rhi::cull_face;
        if (sv == "off") {
            return cf::off;
        }
        if (sv == "front") {
            return cf::front;
        }
        if (sv == "back") {
            return cf::back;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::front_face> to_front_face(tavros::core::string_view sv) noexcept
    {
        using ff = tavros::renderer::rhi::front_face;
        if (sv == "cw") {
            return ff::clockwise;
        }
        if (sv == "ccw") {
            return ff::counter_clockwise;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::polygon_mode> to_polygon_mode(tavros::core::string_view sv) noexcept
    {
        using pm = tavros::renderer::rhi::polygon_mode;
        if (sv == "fill") {
            return pm::fill;
        }
        if (sv == "lines") {
            return pm::lines;
        }
        if (sv == "points") {
            return pm::points;
        }
        return std::nullopt;
    }


    // -----------------------------------------------------------------------
    //  Shaders
    // -----------------------------------------------------------------------

    std::optional<shader_config> parse_shaders(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        if (!n->is_container()) {
            ds.error("'shaders' at '{}' must be an object.", n->path());
            return std::nullopt;
        }

        shader_config result;
        bool          valid = true;

        if (const auto* vert = n->resolve_path("vertex")) {
            if (!vert->is_string()) {
                ds.error("'shaders.vertex' at '{}' must be a string path.", vert->path());
                valid = false;
            } else {
                result.vertex_shader_path = vert->value_or<tavros::core::string_view>("");
                if (result.vertex_shader_path.empty()) {
                    ds.error("'shaders.vertex' at '{}' must not be empty.", vert->path());
                    valid = false;
                }
            }
        } else {
            ds.error("Missing required field 'vertex' in 'shaders' at '{}'.", n->path());
            valid = false;
        }

        if (const auto* frag = n->resolve_path("fragment")) {
            if (!frag->is_string()) {
                ds.error("'shaders.fragment' at '{}' must be a string path.", frag->path());
                valid = false;
            } else {
                result.fragment_shader_path = frag->value_or<tavros::core::string_view>("");
                if (result.fragment_shader_path.empty()) {
                    ds.error("'shaders.fragment' at '{}' must not be empty.", frag->path());
                    valid = false;
                }
            }
        } else {
            ds.error("Missing required field 'fragment' in 'shaders' at '{}'.", n->path());
            valid = false;
        }

        return valid ? std::optional{result} : std::nullopt;
    }


    // -----------------------------------------------------------------------
    //  Blend state
    // -----------------------------------------------------------------------

    // Parses explicit blend object: { src_color dst_color color_op src_alpha dst_alpha alpha_op }
    std::optional<tavros::renderer::rhi::blend_state> parse_blend_object(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        bool                               valid = true;
        tavros::renderer::rhi::blend_state result;
        result.blend_enabled = true;
        valid &= read_required_enum(n, "src_color", to_blend_factor, result.src_color_factor, "blend factor", ds);
        valid &= read_required_enum(n, "dst_color", to_blend_factor, result.dst_color_factor, "blend factor", ds);
        valid &= read_required_enum(n, "color_op", to_blend_op, result.color_blend_op, "blend op", ds);
        valid &= read_required_enum(n, "src_alpha", to_blend_factor, result.src_alpha_factor, "blend factor", ds);
        valid &= read_required_enum(n, "dst_alpha", to_blend_factor, result.dst_alpha_factor, "blend factor", ds);
        valid &= read_required_enum(n, "alpha_op", to_blend_op, result.alpha_blend_op, "blend op", ds);
        return valid ? std::optional{result} : std::nullopt;
    }

    // Parses blend field: "off" | preset string | object
    std::optional<tavros::renderer::rhi::blend_state> parse_blend(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        using bs = tavros::renderer::rhi::blend_state;
        using bf = tavros::renderer::rhi::blend_factor;
        using bo = tavros::renderer::rhi::blend_op;

        // Object form - fully custom
        if (n->is_container()) {
            return parse_blend_object(n, ds);
        }

        if (!n->is_string()) {
            ds.error("'blend' at '{}' must be a string preset or an object.", n->path());
            return std::nullopt;
        }

        auto sv = n->value_or<tavros::core::string_view>("");
        if (sv == "off") {
            return bs{false};
        } else if (sv == "alpha") {
            return bs{true, bf::src_alpha, bf::one_minus_src_alpha, bo::add, bf::one, bf::one_minus_src_alpha, bo::add};
        } else if (sv == "additive") {
            return bs{true, bf::one, bf::one, bo::add, bf::one, bf::one, bo::add};
        } else if (sv == "multiply") {
            return bs{true, bf::dst_color, bf::zero, bo::add, bf::dst_alpha, bf::zero, bo::add};
        } else if (sv == "min") {
            return bs{true, bf::one, bf::one, bo::min, bf::one, bf::one, bo::min};
        } else if (sv == "max") {
            return bs{true, bf::one, bf::one, bo::max, bf::one, bf::one, bo::max};
        } else if (sv == "premul_alpha") {
            return bs{true, bf::one, bf::one_minus_src_alpha, bo::add, bf::one, bf::one_minus_src_alpha, bo::add};
        }

        ds.error("Unknown blend preset '{}' at '{}'. Expected: off, alpha, additive, multiply, min, max, premul_alpha or an object.", sv, n->path());
        return std::nullopt;
    }

    // Parses a single color attachment blend state entry
    // Handles: "off" | { mask blend }
    std::optional<color_attachment_state_config> parse_color_attachment_state(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        color_attachment_state_config result;
        result.name = n->key();

        if (result.name.empty()) {
            ds.error("Unnamed color attachment state at '{}'. A key matching the render target attachment name is required.", n->path());
            return std::nullopt;
        }

        // "off" - attachment writes disabled entirely
        if (n->is_string()) {
            auto sv = n->value_or<tavros::core::string_view>("");
            if (sv == "off") {
                result.mask = tavros::core::flags<tavros::renderer::rhi::color_mask>();
                return result;
            }
            ds.error("Invalid value '{}' at '{}'. Expected \"off\" or an object.", sv, n->path());
            return std::nullopt;
        }

        if (!n->is_container()) {
            ds.error("Color attachment state at '{}' must be \"off\" or an object.", n->path());
            return std::nullopt;
        }

        bool valid = read_enum(n, "mask", tavros::renderer::rhi::k_rgba_color_mask, to_color_mask, result.mask, "color write mask", ds);

        if (const auto* blend_node = n->resolve_path("blend")) {
            if (auto b = parse_blend(blend_node, ds)) {
                result.blend = *b;
            } else {
                result.blend.blend_enabled = false;
            }
        } else {
            result.blend.blend_enabled = false;
        }

        return valid ? std::optional{result} : std::nullopt;
    }


    // -----------------------------------------------------------------------
    //  Depth / stencil
    // -----------------------------------------------------------------------

    std::optional<depth_attachment_state_config> parse_depth_attachment_state(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        // "off"
        if (n->is_string()) {
            auto sv = n->value_or<tavros::core::string_view>("");
            if (sv == "off") {
                return depth_attachment_state_config{false, false, tavros::renderer::rhi::compare_op::off};
            }
            ds.error("Invalid value '{}' at '{}'. Expected \"off\" or an object.", sv, n->path());
            return std::nullopt;
        }

        if (!n->is_container()) {
            ds.error("Depth attachment state at '{}' must be \"off\" or an object.", n->path());
            return std::nullopt;
        }

        depth_attachment_state_config result;
        constexpr auto                is_bool = [](const tavros::tef::node* node) noexcept { return node->is_boolean(); };
        bool                          valid = read_required_scalar<bool>(n, "test", result.test_enabled, is_bool, "bool", ds);
        valid &= read_required_scalar<bool>(n, "write", result.write_enabled, is_bool, "bool", ds);
        valid &= read_required_enum(n, "compare_op", to_compare_op, result.compare_op, "compare op", ds);

        return valid ? std::optional{result} : std::nullopt;
    }

    std::optional<tavros::renderer::rhi::stencil_state> parse_stencil_state(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        // "off"
        if (n->is_string()) {
            auto sv = n->value_or<tavros::core::string_view>("");
            if (sv == "off") {
                return tavros::renderer::rhi::stencil_state{}; // disabled defaults
            }
            ds.error("Invalid value '{}' at '{}'. Expected \"off\" or an object.", sv, n->path());
            return std::nullopt;
        }

        if (!n->is_container()) {
            ds.error("Stencil face state at '{}' must be \"off\" or an object.", n->path());
            return std::nullopt;
        }

        tavros::renderer::rhi::stencil_state result;

        constexpr auto is_u8 = [](const tavros::tef::node* node) noexcept {
            if (node->is_integer()) {
                auto v = node->value_or<int64>(0);
                return v >= 0 && v <= 255;
            }
        };

        bool valid = read_clamped<uint8>(n, "read_mask", 0xFF, 0, 255, result.read_mask, is_u8, "integer [0..255]", ds);
        valid &= read_clamped<uint8>(n, "write_mask", 0xFF, 0, 255, result.write_mask, is_u8, "integer [0..255]", ds);
        valid &= read_clamped<uint8>(n, "ref_value", 0, 0, 255, result.reference_value, is_u8, "integer [0..255]", ds);
        valid &= read_enum(n, "compare_op", tavros::renderer::rhi::compare_op::always, to_compare_op, result.compare, "compare op", ds);
        valid &= read_enum(n, "stencil_fail_op", tavros::renderer::rhi::stencil_op::keep, to_stencil_op, result.stencil_fail_op, "stencil op", ds);
        valid &= read_enum(n, "depth_fail_op", tavros::renderer::rhi::stencil_op::keep, to_stencil_op, result.depth_fail_op, "stencil op", ds);
        valid &= read_enum(n, "pass_op", tavros::renderer::rhi::stencil_op::keep, to_stencil_op, result.pass_op, "stencil op", ds);

        return valid ? std::optional{result} : std::nullopt;
    }

    std::optional<stencil_attachment_state_config> parse_stencil_attachment_state_config(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        stencil_attachment_state_config result;

        constexpr auto is_bool = [](const tavros::tef::node* node) noexcept { return node->is_boolean(); };
        bool           valid = read_required_scalar<bool>(n, "test", result.test_enabled, is_bool, "bool", ds);

        if (const auto* front = n->resolve_path("front")) {
            if (auto val = parse_stencil_state(front, ds)) {
                result.front = *val;
            } else {
                valid = false;
            }
        } else {
            ds.error("Missing required field 'front' at '{}'.", n->path());
            valid = false;
        }

        if (const auto* back = n->resolve_path("back")) {
            if (auto val = parse_stencil_state(back, ds)) {
                result.back = *val;
            } else {
                valid = false;
            }
        } else {
            ds.error("Missing required field 'back' at '{}'.", n->path());
            valid = false;
        }

        return valid ? std::optional{result} : std::nullopt;
    }


    // -----------------------------------------------------------------------
    //  Rasterizer
    // -----------------------------------------------------------------------

    std::optional<rasterizer_config> parse_rasterizer(const tavros::tef::node* n, diagnostics& ds) noexcept
    {
        if (!n) {
            return rasterizer_config{}; // all defaults
        }

        if (!n->is_container()) {
            ds.error("'rasterizer' at '{}' must be an object.", n->path());
            return std::nullopt;
        }

        rasterizer_config result;

        bool valid = read_enum(n, "cull", tavros::renderer::rhi::cull_face::off, to_cull_face, result.cull, "cull face", ds);
        valid &= read_enum(n, "front_face", tavros::renderer::rhi::front_face::counter_clockwise, to_front_face, result.face, "front face", ds);
        valid &= read_enum(n, "fill_mode", tavros::renderer::rhi::polygon_mode::fill, to_polygon_mode, result.polygon, "fill mode", ds);

        // depth_clamp: "off" | { min max }
        if (const auto* dc = n->resolve_path("depth_clamp")) {
            if (dc->is_string()) {
                auto sv = dc->value_or<tavros::core::string_view>("");
                if (sv != "off") {
                    ds.error("Invalid value '{}' at '{}'. Expected \"off\" or an object.", sv, dc->path());
                    valid = false;
                } else {
                    result.depth_clamp_enable = false;
                }
            } else if (dc->is_container()) {
                result.depth_clamp_enable = true;
                constexpr auto is_number = [](const tavros::tef::node* node) noexcept { return node->is_number(); };
                valid &= read_clamped<float>(dc, "min", 0.0f, 0.0f, 1.0f, result.depth_clamp_near, is_number, "number", ds);
                valid &= read_clamped<float>(dc, "max", 1.0f, 0.0f, 1.0f, result.depth_clamp_near, is_number, "number", ds);
            } else {
                ds.error("'depth_clamp' at '{}' must be \"off\" or an object.", dc->path());
                valid = false;
            }
        }

        // depth_bias: "off" | { constant slope clamp }
        if (const auto* db = n->resolve_path("depth_bias")) {
            if (db->is_string()) {
                auto sv = db->value_or<tavros::core::string_view>("");
                if (sv != "off") {
                    ds.error("Invalid value '{}' at '{}'. Expected \"off\" or an object.", sv, db->path());
                    valid = false;
                } else {
                    result.depth_bias_enable = false;
                }
            } else if (db->is_container()) {
                constexpr auto is_number = [](const tavros::tef::node* node) noexcept { return node->is_number(); };
                valid &= read_clamped<float>(db, "constant", 0.0f, 0.0f, 1.0f, result.depth_bias, is_number, "number", ds);
                valid &= read_clamped<float>(db, "slope", 0.0f, 0.0f, 1.0f, result.depth_bias_factor, is_number, "number", ds);
                valid &= read_clamped<float>(db, "clamp", 0.0f, 0.0f, 1.0f, result.depth_bias_clamp, is_number, "number", ds);
            } else {
                ds.error("'depth_bias' at '{}' must be \"off\" or an object.", db->path());
                valid = false;
            }
        }

        return valid ? std::optional{result} : std::nullopt;
    }


    // -----------------------------------------------------------------------
    //  Top-level material_desc
    // -----------------------------------------------------------------------

    std::optional<material_desc> parse_material_desc(const tavros::tef::node* n, tavros::core::string_view name, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        shader_config                   sc;
        color_attachments_state_config  ca;
        depth_attachment_state_config   da;
        stencil_attachment_state_config sa;
        topology_config                 tc;
        rasterizer_config               rc;

        bool valid = true;

        // shaders - required
        if (const auto* shaders = n->resolve_path("shaders")) {
            if (auto v = parse_shaders(shaders, ds)) {
                sc = std::move(*v);
            } else {
                valid = false;
            }
        } else {
            ds.error("Missing required field 'shaders' at '{}'.", n->path());
            valid = false;
        }

        // color attachments blend state - optional
        if (const auto* color = n->resolve_path("color")) {
            if (!color->is_container()) {
                ds.error("'color' at '{}' must be an object containing attachment states.", color->path());
                valid = false;
            } else {
                uint32 count = 0;
                for (const auto& ch : color->children()) {
                    ++count;
                    if (static_cast<size_t>(count) > tavros::renderer::rhi::k_max_color_attachments) {
                        ds.error("Too many color attachment states at '{}'. Maximum is {}.", color->path(), tavros::renderer::rhi::k_max_color_attachments);
                        valid = false;
                        break;
                    }
                    if (auto v = parse_color_attachment_state(&ch, ds)) {
                        ca.push_back(std::move(*v));
                    } else {
                        valid = false;
                    }
                }
            }
        }

        if (const auto* d = n->resolve_path("depth")) {
            if (auto v = parse_depth_attachment_state(d, ds)) {
                da = std::move(*v);
            } else {
                valid = false;
            }
        }

        if (const auto* s = n->resolve_path("stencil")) {
            if (auto v = parse_stencil_attachment_state_config(s, ds)) {
                sa = std::move(*v);
            } else {
                valid = false;
            }
        }

        valid &= read_required_enum(n, "topology", to_topology, tc.topology, "topology", ds);

        if (const auto* r = n->resolve_path("rasterizer")) {
            if (auto v = parse_rasterizer(r, ds)) {
                rc = std::move(*v);
            } else {
                valid = false;
            }
        } else {
            ds.error("Missing required field 'rasterizer' at '{}'.", n->path());
            valid = false;
        }

        if (valid) {
            return material_desc(name, sc, ca, da, sa, tc, rc);
        }
        return std::nullopt;
    }

} // namespace

namespace tavros::tef
{
    void schema<renderer::material_desc>::serialize(node* n, const tavros::renderer::material_desc& in, core::diagnostics& ds) noexcept
    {
        TAV_ASSERT(false);
        // TODO: Implement serialization
    }

    void schema<renderer::material_desc>::deserialize(const node* n, tavros::renderer::material_desc& out, core::diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);
        if (!n) {
            ds.error("Null node provided for deserializing material config.");
            return;
        }

        if (auto result = parse_material_desc(n, n->path(), ds)) {
            out = *result;
        } else {
            ds.error("Failed to parse material configuration at '{}'.", n->path());
        }
    }
} // namespace tavros::tef
