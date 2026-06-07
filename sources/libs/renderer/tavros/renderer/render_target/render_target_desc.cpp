#include <tavros/renderer/render_target/render_target_desc.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/logger/diagnostics.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/tef/helpers.hpp>

namespace
{
    tavros::core::logger logger("render_target_factory");

    using diagnostics = tavros::core::diagnostics;
    using color_attachment_config = tavros::renderer::render_target_desc::color_attachment_config;
    using color_attachments_config = tavros::renderer::render_target_desc::color_attachments_config;
    using depth_attachment_config = tavros::renderer::render_target_desc::depth_attachment_config;
    using stencil_attachment_config = tavros::renderer::render_target_desc::stencil_attachment_config;
    using multisample_config = tavros::renderer::render_target_desc::multisample_config;
    using render_target_desc = tavros::renderer::render_target_desc;

    std::optional<tavros::renderer::rhi::load_op> to_load_op(tavros::core::string_view sv) noexcept
    {
        if (sv == "load") {
            return tavros::renderer::rhi::load_op::load;
        } else if (sv == "clear") {
            return tavros::renderer::rhi::load_op::clear;
        } else if (sv == "dont_care") {
            return tavros::renderer::rhi::load_op::dont_care;
        } else {
            return std::nullopt;
        }
    }

    std::optional<tavros::renderer::rhi::store_op> to_store_op(tavros::core::string_view sv) noexcept
    {
        if (sv == "store") {
            return tavros::renderer::rhi::store_op::store;
        } else if (sv == "dont_care") {
            return tavros::renderer::rhi::store_op::dont_care;
        } else {
            return std::nullopt;
        }
    }

    std::optional<tavros::renderer::rhi::pixel_format> to_color_format(tavros::core::string_view sv) noexcept
    {
        if (sv == "rgba8") {
            return tavros::renderer::rhi::pixel_format::rgba8un;
        } else if (sv == "rgba16f") {
            return tavros::renderer::rhi::pixel_format::rgba16f;
        } else if (sv == "rgba32f") {
            return tavros::renderer::rhi::pixel_format::rgba32f;
        } else if (sv == "rgb8") {
            return tavros::renderer::rhi::pixel_format::rgb8un;
        } else if (sv == "rgb16f") {
            return tavros::renderer::rhi::pixel_format::rgb16f;
        } else if (sv == "rgb32f") {
            return tavros::renderer::rhi::pixel_format::rgb32f;
        } else if (sv == "rg8") {
            return tavros::renderer::rhi::pixel_format::rg8un;
        } else if (sv == "rg16f") {
            return tavros::renderer::rhi::pixel_format::rg16f;
        } else if (sv == "rg32f") {
            return tavros::renderer::rhi::pixel_format::rg32f;
        } else if (sv == "r8") {
            return tavros::renderer::rhi::pixel_format::r8un;
        } else if (sv == "r16f") {
            return tavros::renderer::rhi::pixel_format::r16f;
        } else if (sv == "r32f") {
            return tavros::renderer::rhi::pixel_format::r32f;
        } else {
            return std::nullopt;
        }
    }

    std::optional<tavros::renderer::rhi::pixel_format> to_depth_format(tavros::core::string_view sv) noexcept
    {
        if (sv == "depth24") {
            return tavros::renderer::rhi::pixel_format::depth24;
        } else if (sv == "depth32f") {
            return tavros::renderer::rhi::pixel_format::depth32f;
        } else {
            return std::nullopt;
        }
    }

    std::optional<tavros::renderer::rhi::pixel_format> to_stencil_format(tavros::core::string_view sv) noexcept
    {
        if (sv == "stencil8") {
            return tavros::renderer::rhi::pixel_format::stencil8;
        } else {
            return std::nullopt;
        }
    }

    uint32 color_format_components_number(tavros::renderer::rhi::pixel_format fmt) noexcept
    {
        switch (fmt) {
        case tavros::renderer::rhi::pixel_format::rgba8un:
            return 4;
        case tavros::renderer::rhi::pixel_format::rgba16f:
            return 4;
        case tavros::renderer::rhi::pixel_format::rgba32f:
            return 4;
        case tavros::renderer::rhi::pixel_format::rgb8un:
            return 3;
        case tavros::renderer::rhi::pixel_format::rgb16f:
            return 3;
        case tavros::renderer::rhi::pixel_format::rgb32f:
            return 3;
        case tavros::renderer::rhi::pixel_format::rg8un:
            return 2;
        case tavros::renderer::rhi::pixel_format::rg16f:
            return 2;
        case tavros::renderer::rhi::pixel_format::rg32f:
            return 2;
        case tavros::renderer::rhi::pixel_format::r8un:
            return 1;
        case tavros::renderer::rhi::pixel_format::r16f:
            return 1;
        case tavros::renderer::rhi::pixel_format::r32f:
            return 1;
        default:
            TAV_UNREACHABLE();
        }
    }
} // namespace

namespace tavros::tef
{

    std::optional<tavros::renderer::render_target_desc::color_attachment_config> parse_color_attachment(const node* n, diagnostics& ds)
    {
        TAV_ASSERT(n);

        if (!n->is_container()) {
            ds.error("Color attachment at '{}' must be an object.", n->path());
            return std::nullopt;
        }

        color_attachment_config result;
        bool                    valid = true;

        result.name = n->key();
        if (result.name.empty()) {
            ds.error("Unnamed color attachment at '{}'. A key is required.", n->path());
            valid = false;
        }

        uint32 components = 0;
        if (auto fmt = read_required_enum(n, "format", to_color_format, "color format", ds)) {
            result.format = *fmt;
            components = color_format_components_number(*fmt);
        } else {
            valid = false;
        }

        if (auto op = read_enum(n, "load", renderer::rhi::load_op::clear, to_load_op, "load operation", ds)) {
            result.load = *op;
        } else {
            valid = false;
        }

        if (auto op = read_enum(n, "store", renderer::rhi::store_op::store, to_store_op, "store operation", ds)) {
            result.store = *op;
        } else {
            valid = false;
        }

        // clear - by default clear value is 0.0 0.0 0.0 0.0
        result.clear_value[0] = result.clear_value[1] = result.clear_value[2] = result.clear_value[3] = 0.0f;
        if (const auto* clear = n->resolve_path("clear"); clear && components > 0) {
            const auto* cur = clear;
            for (uint32 c = 0; c < components; ++c) {
                if (!cur) {
                    ds.error("Not enough clear values at '{}'. Expected {} numeric values.", clear->path(), components);
                    valid = false;
                    break;
                }
                if (!cur->is_number()) {
                    ds.error("Invalid clear value at '{}'. Expected a numeric value.", cur->path());
                    valid = false;
                    break;
                }
                result.clear_value[c] = cur->value_or(0.0f);
                cur = cur->next();
            }
            warn_if_extra_values(cur, components, ds);
        }

        return valid ? std::optional{result} : std::nullopt;
    }


    std::optional<depth_attachment_config> parse_depth_attachment(const node* n, diagnostics& ds)
    {
        if (!n) {
            return depth_attachment_config{renderer::rhi::pixel_format::none};
        }

        TAV_ASSERT(n->key() == "depth");

        if (!n->is_container()) {
            ds.error("Depth attachment at '{}' must be an object.", n->path());
            return std::nullopt;
        }

        depth_attachment_config result;
        bool                    valid = true;

        if (auto fmt = read_required_enum(n, "format", to_depth_format, "depth format", ds)) {
            result.format = *fmt;
        } else {
            valid = false;
        }

        if (auto op = read_enum(n, "load", renderer::rhi::load_op::clear, to_load_op, "load operation", ds)) {
            result.load = *op;
        } else {
            valid = false;
        }

        if (auto op = read_enum(n, "store", renderer::rhi::store_op::dont_care, to_store_op, "store operation", ds)) {
            result.store = *op;
        } else {
            valid = false;
        }

        constexpr auto is_number = [](const node* n) noexcept { return n->is_number(); };
        if (auto val = read_clamped<float>(n, "clear", 1.0f, 0.0f, 1.0f, is_number, "numeric value", ds)) {
            result.clear_value = *val;
        } else {
            valid = false;
        }

        return valid ? std::optional{result} : std::nullopt;
    }


    std::optional<stencil_attachment_config> parse_stencil_attachment(const node* n, diagnostics& ds)
    {
        if (!n) {
            return stencil_attachment_config{renderer::rhi::pixel_format::none};
        }

        TAV_ASSERT(n->key() == "stencil");

        if (!n->is_container()) {
            ds.error("Stencil attachment at '{}' must be an object.", n->path());
            return std::nullopt;
        }

        stencil_attachment_config result;
        bool                      valid = true;

        if (auto fmt = read_required_enum(n, "format", to_stencil_format, "stencil format", ds)) {
            result.format = *fmt;
        } else {
            valid = false;
        }

        if (auto op = read_enum(n, "load", renderer::rhi::load_op::clear, to_load_op, "load operation", ds)) {
            result.load = *op;
        } else {
            valid = false;
        }

        if (auto op = read_enum(n, "store", renderer::rhi::store_op::dont_care, to_store_op, "store operation", ds)) {
            result.store = *op;
        } else {
            valid = false;
        }

        constexpr auto is_integer = [](const node* n) noexcept { return n->is_integer(); };
        if (auto val = read_clamped<uint64>(n, "clear", uint64{0}, uint64{0}, uint64{255}, is_integer, "integer value", ds)) {
            result.clear_value = static_cast<uint32>(*val);
        } else {
            valid = false;
        }

        return valid ? std::optional{result} : std::nullopt;
    }


    std::optional<multisample_config> parse_multisample(const node* n, diagnostics& ds)
    {
        if (!n) {
            return multisample_config{1};
        }

        TAV_ASSERT(n->key() == "multisample");

        if (!n->is_container()) {
            ds.error("Multisample settings at '{}' must be an object.", n->path());
            return std::nullopt;
        }

        multisample_config result;
        if (const auto* sc = n->resolve_path("sample_count")) {
            if (!sc->is_integer()) {
                ds.error("Invalid value at '{}'. Expected an integer sample count.", sc->path());
                return std::nullopt;
            }
            auto            val = sc->value_or<int64>(1);
            constexpr int64 valid_counts[] = {1, 2, 4, 8, 16, 32};
            bool            found = false;
            for (auto c : valid_counts) {
                if (val == c) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ds.error("Invalid value {} at '{}'. Expected one of: 1, 2, 4, 8, 16, 32.", val, sc->path());
                return std::nullopt;
            }
            warn_if_extra_values(sc, 1, ds);
            result.sample_count = static_cast<uint32>(val);
        }

        return result;
    }

    std::optional<render_target_desc> parse_render_target_config(const node* n, diagnostics& ds)
    {
        if (!n) {
            ds.error("Null node provided for render target configuration.");
            return std::nullopt;
        }

        color_attachments_config  ca;
        depth_attachment_config   da;
        stencil_attachment_config sa;
        multisample_config        msaa;
        bool                      valid = true;

        // color attachments - optional, but if not specified, depth or/and stencil attachments must be provided
        if (const auto* color = n->resolve_path("color")) {
            if (color->is_container()) {
                uint32 count = 0;
                for (const auto& ch : color->children()) {
                    ++count;
                    if (static_cast<size_t>(count) > renderer::rhi::k_max_color_attachments) {
                        ds.error("Too many color attachments at '{}'. Maximum supported is {}.", color->path(), renderer::rhi::k_max_color_attachments);
                        valid = false;
                        break;
                    }

                    if (auto color_attachment_opt = parse_color_attachment(&ch, ds)) {
                        ca.push_back(*color_attachment_opt);
                    } else {
                        valid = false;
                    }
                }

            } else {
                ds.error("Field 'color' at '{}' must be an object containing attachments.", color->path());
                valid = false;
            }
        }

        // depth attachment - optional, but if not specified, at least one color or stencil attachment must be provided
        if (auto depth = parse_depth_attachment(n->resolve_path("depth"), ds)) {
            da = *depth;
        } else {
            valid = false;
        }

        // stencil attachment - optional, but if not specified, at least one color or depth attachment must be provided
        if (auto stencil = parse_stencil_attachment(n->resolve_path("stencil"), ds)) {
            sa = *stencil;
        } else {
            valid = false;
        }


        // multisample - optional, if not specified, sample count is 1 (no multisampling)
        if (auto multisample = parse_multisample(n->resolve_path("multisample"), ds)) {
            msaa = *multisample;
        } else {
            valid = false;
        }

        // Final validation: if no color attachments, depth or stencil attachment must be provided
        if (ca.empty() && da.format == tavros::renderer::rhi::pixel_format::none && sa.format == tavros::renderer::rhi::pixel_format::none) {
            ds.error("Invalid render target configuration at '{}'. At least one color, depth, or stencil attachment must be defined.", n->path());
            valid = false;
        }

        if (valid) {
            return render_target_desc(n->path(), ca, da, sa, msaa);
        }
        return std::nullopt;
    }


    void schema<render_target_desc>::serialize(node* n, const render_target_desc& in, core::diagnostics& ds) noexcept
    {
        TAV_ASSERT(false);

        // TODO: Implement serialization
    }

    void schema<render_target_desc>::deserialize(const node* n, render_target_desc& out, core::diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);
        if (!n) {
            ds.error("Null node provided for deserializing render target config.");
            return;
        }

        if (auto result = parse_render_target_config(n, ds)) {
            out = *result;
        } else {
            ds.error("Failed to parse render target configuration at '{}'.", n->path());
        }
    }

} // namespace tavros::tef
