#pragma once

#include <tavros/core/fixed_string.hpp>
#include <tavros/tef/workspace.hpp>
#include <tavros/tef/schema.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer
{

    class texture_desc
    {
    public:
        struct texture_load_params
        {
            /// Texture path
            core::fixed_path path;

            /// X offset into the source image in pixels
            uint32 left = 0;

            /// Y offset into the source image in pixels
            uint32 top = 0;

            /// Width of the source region in pixels (if 0 -> im.width() - left)
            uint32 width = 0;

            /// Height of the source region in pixels (if 0 -> im.height() - top)
            uint32 height = 0;

            /// Depth of the source region in pixels (texture_3d only)
            uint32 depth = 1;

            /// Whether to generate a full mipmap chain
            bool gen_mipmaps = true;

            /// Type of the GPU texture to create
            rhi::texture_type type = rhi::texture_type::texture_2d;

            /// Target pixel format (if none -> inferred from source image)
            rhi::pixel_format pixel_format = rhi::pixel_format::none;

            /// Number of array layers to create (if 1 -> regular texture, if > 1 -> texture array)
            uint32 array_layers = 1;

            /// Number of tile rows in the source image grid (0 = auto-detected)
            uint32 array_rows = 0;

            /// Number of tile columns in the source image grid (0 = auto-detected)
            uint32 array_cols = 0;
        };

    public:
        texture_desc() noexcept = default;

        texture_desc(
            core::string_view          tex_name,
            const texture_load_params& lp
        ) noexcept
            : m_name(tex_name)
            , m_load_params(lp)
        {
        }

        const texture_load_params& load_params() const noexcept
        {
            return m_load_params;
        }

        core::string_view name() const noexcept
        {
            return m_name;
        }

    private:
        core::short_string  m_name;
        texture_load_params m_load_params;
    };

} // namespace tavros::renderer

namespace tavros::tef
{
    template<>
    struct schema<tavros::renderer::texture_desc>
    {
        static void serialize(node* n, const tavros::renderer::texture_desc& in, core::diagnostics& ds) noexcept;
        static void deserialize(const node* n, tavros::renderer::texture_desc& out, core::diagnostics& ds) noexcept;
    };
} // namespace tavros::tef
