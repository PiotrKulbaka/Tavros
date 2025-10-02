#pragma once

#include <tavros/core/resources/resource_view.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/texture_info.hpp>

namespace tavros::renderer
{

    struct texture_create_info
    {
        /// Type of the texture
        rhi::texture_type type = rhi::texture_type::texture_2d;

        /// Pixel format defining color channels, bit depth, and data layout
        rhi::pixel_format format = rhi::pixel_format::rgba8un;

        /// Texture width in pixels. Must be > 0
        uint32 width = 0;

        /// Texture height in pixels. Must be > 0
        uint32 height = 0;

        /// Texture depth (for 3D textures) Must be > 0, for texture_2d and texture_cube must be 1
        uint32 depth = 1;

        /// Number of mipmap levels, 1 indicates no mipmaps
        uint32 mip_levels = 1;

        ///  Number of array layers (for texture arrays). 1 means a single texture, must be >= 1
        uint32 array_layers = 1;
    };

    /**
     * @brief Represents a GPU texture resource.
     *
     * The `texture` class encapsulates a GPU texture allocated via the RHI.
     * It stores type, format, dimensions, and a GPU handle to the texture.
     * Instances are created and managed exclusively by `render_system`.
     * Users access textures via `texture_view`, which provides a safe, non-owning handle.
     */
    class texture
    {
    public:
        /**
         * @brief Destructor.
         *
         * Does not release GPU resources directly. The owning `render_system`
         * is responsible for proper cleanup.
         */
        ~texture() noexcept = default;

        /**
         * @brief Returns the type of the texture (2D, 3D, Cube, etc.)
         */
        [[nodiscard]] rhi::texture_type type() const noexcept
        {
            return m_info.type;
        }

        /**
         * @brief Returns the pixel format of the texture.
         */
        [[nodiscard]] rhi::pixel_format format() const noexcept
        {
            return m_info.format;
        }

        /**
         * @brief Returns the width of the texture in pixels.
         */
        [[nodiscard]] uint32 width() const noexcept
        {
            return m_info.width;
        }

        /**
         * @brief Returns the height of the texture in pixels.
         */
        [[nodiscard]] uint32 height() const noexcept
        {
            return m_info.height;
        }

        /**
         * @brief Returns the depth of the texture.
         *
         * For 2D and cube textures, depth is always 1.
         */
        [[nodiscard]] uint32 depth() const noexcept
        {
            return m_info.depth;
        }

        /**
         * @brief Returns the GPU handle for this texture.
         */
        [[nodiscard]] rhi::texture_handle handle() const
        {
            return m_handle;
        }

    private:
        /**
         * @brief Constructs a texture with a GPU handle and creation info.
         * @param handle The RHI texture handle.
         * @param info Creation parameters (type, format, size, etc.)
         *
         * Only `render_system` can call this constructor.
         */
        texture(rhi::texture_handle handle, const texture_create_info& info) noexcept
            : m_handle(handle)
            , m_info(info)
        {
        }

        friend class core::resource_pool<texture>; // for empalce_add in render_system

    private:
        rhi::texture_handle m_handle = rhi::texture_handle::invalid();
        texture_create_info m_info;
    };

    /**
     * @brief Lightweight, non-owning view of a texture resource.
     *
     * `texture_view` is a typedef for `resource_view<texture>`.
     * It allows safe access to a `texture` managed by `render_system`
     * without owning it. The underlying texture may be invalidated if
     * the `render_system` destroys it, so `valid()` should be checked before use.
     */
    using texture_view = core::resource_view<texture>;

} // namespace tavros::renderer
