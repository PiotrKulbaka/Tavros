#include <tavros/renderer/texture/texture_desc.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/logger/diagnostics.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/tef/helpers.hpp>

namespace
{
    tavros::core::logger logger("texture_desc");

    using diagnostics = tavros::core::diagnostics;
    using texture_load_params = tavros::renderer::texture_desc::texture_load_params;

    std::optional<tavros::renderer::rhi::texture_type> to_texture_type(tavros::core::string_view sv) noexcept
    {
        using tt = tavros::renderer::rhi::texture_type;
        if (sv == "texture_2d") {
            return tt::texture_2d;
        } else if (sv == "texture_3d") {
            return tt::texture_3d;
        } else if (sv == "texture_cube") {
            return tt::texture_cube;
        }
        return std::nullopt;
    }

    std::optional<tavros::renderer::rhi::pixel_format> to_pixel_format(tavros::core::string_view sv) noexcept
    {
        using pf = tavros::renderer::rhi::pixel_format;

        if (sv == "none") {
            return pf::none;
        }

        // Normalized formats
        else if (sv == "r8un") {
            return pf::r8un;
        } else if (sv == "r8in") {
            return pf::r8in;
        } else if (sv == "r16un") {
            return pf::r16un;
        } else if (sv == "r16in") {
            return pf::r16in;
        }

        else if (sv == "rg8un") {
            return pf::rg8un;
        } else if (sv == "rg8in") {
            return pf::rg8in;
        } else if (sv == "rg16un") {
            return pf::rg16un;
        } else if (sv == "rg16in") {
            return pf::rg16in;
        }

        // Specific format, rarely found in the GPU API
        else if (sv == "rgb8un") {
            return pf::rgb8un;
        } else if (sv == "rgb8in") {
            return pf::rgb8in;
        } else if (sv == "rgb16un") {
            return pf::rgb16un;
        } else if (sv == "rgb16in") {
            return pf::rgb16in;
        }

        else if (sv == "rgba8un") {
            return pf::rgba8un;
        } else if (sv == "rgba8in") {
            return pf::rgba8in;
        } else if (sv == "rgba16un") {
            return pf::rgba16un;
        } else if (sv == "rgba16in") {
            return pf::rgba16in;
        }

        // Integer formats
        else if (sv == "r8u") {
            return pf::r8u;
        } else if (sv == "r8i") {
            return pf::r8i;
        } else if (sv == "r16u") {
            return pf::r16u;
        } else if (sv == "r16i") {
            return pf::r16i;
        } else if (sv == "r32u") {
            return pf::r32u;
        } else if (sv == "r32i") {
            return pf::r32i;
        }

        else if (sv == "rg8u") {
            return pf::rg8u;
        } else if (sv == "rg8i") {
            return pf::rg8i;
        } else if (sv == "rg16u") {
            return pf::rg16u;
        } else if (sv == "rg16i") {
            return pf::rg16i;
        } else if (sv == "rg32u") {
            return pf::rg32u;
        } else if (sv == "rg32i") {
            return pf::rg32i;
        }

        // Specific format, rarely found in the GPU API
        else if (sv == "rgb8u") {
            return pf::rgb8u;
        } else if (sv == "rgb8i") {
            return pf::rgb8i;
        } else if (sv == "rgb16u") {
            return pf::rgb16u;
        } else if (sv == "rgb16i") {
            return pf::rgb16i;
        } else if (sv == "rgb32u") {
            return pf::rgb32u;
        } else if (sv == "rgb32i") {
            return pf::rgb32i;
        }

        else if (sv == "rgba8u") {
            return pf::rgba8u;
        } else if (sv == "rgba8i") {
            return pf::rgba8i;
        } else if (sv == "rgba16u") {
            return pf::rgba16u;
        } else if (sv == "rgba16i") {
            return pf::rgba16i;
        } else if (sv == "rgba32u") {
            return pf::rgba32u;
        } else if (sv == "rgba32i") {
            return pf::rgba32i;
        }

        // Floating point formats
        else if (sv == "r16f") {
            return pf::r16f;
        } else if (sv == "r32f") {
            return pf::r32f;
        } else if (sv == "rg16f") {
            return pf::rg16f;
        } else if (sv == "rg32f") {
            return pf::rg32f;
        }

        // Specific format, rarely found in the GPU API
        else if (sv == "rgb16f") {
            return pf::rgb16f;
        } else if (sv == "rgb32f") {
            return pf::rgb32f;
        } else if (sv == "rgba16f") {
            return pf::rgba16f;
        } else if (sv == "rgba32f") {
            return pf::rgba32f;
        }

        // Depth / stencil formats
        else if (sv == "depth16") {
            return pf::depth16;
        } else if (sv == "depth24") {
            return pf::depth24;
        } else if (sv == "depth32f") {
            return pf::depth32f;
        }

        // Specific format, not typically used as a regular texture
        else if (sv == "stencil8") {
            return pf::stencil8;
        } else if (sv == "depth24_stencil8") {
            return pf::depth24_stencil8;
        } else if (sv == "depth32f_stencil8") {
            return pf::depth32f_stencil8;
        }

        return std::nullopt;
    }
} // namespace

namespace tavros::tef
{
    std::optional<texture_load_params> parse_texture_load_params(const node* n, diagnostics& ds)
    {
        if (!n) {
            ds.error("Null node provided for texture config.");
            return std::nullopt;
        }

        if (!n->is_container()) {
            ds.error("Texture at '{}' must be an object.", n->path());
            return std::nullopt;
        }

        texture_load_params out;

        constexpr auto is_integer = [](const node* n) noexcept { return n->is_integer(); };
        constexpr auto is_bool = [](const node* n) noexcept { return n->is_boolean(); };

        bool valid = read_required_string(n, "path", out.path, "texture path", ds);
        valid &= read_clamped<uint32>(n, "left", uint32{0}, uint32{0}, uint32{0xffffffff}, out.left, is_integer, "integer value", ds);
        valid &= read_clamped<uint32>(n, "top", uint32{0}, uint32{0}, uint32{0xffffffff}, out.top, is_integer, "integer value", ds);
        valid &= read_clamped<uint32>(n, "width", uint32{0}, uint32{0}, uint32{0xffffffff}, out.width, is_integer, "integer value", ds);
        valid &= read_clamped<uint32>(n, "height", uint32{0}, uint32{0}, uint32{0xffffffff}, out.height, is_integer, "integer value", ds);
        valid &= read_clamped<uint32>(n, "depth", uint32{1}, uint32{1}, uint32{0xffffffff}, out.depth, is_integer, "integer value", ds);
        valid &= read_scalar<bool>(n, "gen_mipmaps", false, out.gen_mipmaps, is_bool, "boolean value", ds);
        valid &= read_enum(n, "type", renderer::rhi::texture_type::texture_2d, to_texture_type, out.type, "texture type", ds);
        valid &= read_enum(n, "pixel_format", renderer::rhi::pixel_format::none, to_pixel_format, out.pixel_format, "pixel format", ds);
        valid &= read_clamped<uint32>(n, "array_layers", uint32{1}, uint32{1}, uint32{0xffffffff}, out.array_layers, is_integer, "integer value", ds);
        valid &= read_clamped<uint32>(n, "array_rows", uint32{1}, uint32{1}, uint32{0xffffffff}, out.array_rows, is_integer, "integer value", ds);
        valid &= read_clamped<uint32>(n, "array_cols", uint32{1}, uint32{1}, uint32{0xffffffff}, out.array_cols, is_integer, "integer value", ds);

        if (valid) {
            return out;
        }
        return std::nullopt;
    }

    void schema<tavros::renderer::texture_desc>::serialize(node* n, const tavros::renderer::texture_desc& in, core::diagnostics& ds) noexcept
    {
        TAV_UNUSED(n);
        TAV_UNUSED(in);
        TAV_UNUSED(ds);
        TAV_ASSERT(false);
        // TODO: Implement serialization
    }

    void schema<tavros::renderer::texture_desc>::deserialize(const node* n, tavros::renderer::texture_desc& out, core::diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);
        if (!n) {
            ds.error("Null node provided for deserializing texture config.");
            return;
        }

        if (auto result = parse_texture_load_params(n, ds)) {
            out = tavros::renderer::texture_desc(n->path(), std::move(*result));
        } else {
            ds.error("Failed to parse texture config at '{}'.", n->path());
        }
    }

} // namespace tavros::tef
