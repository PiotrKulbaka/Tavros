#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/ids/handle_base.hpp>
#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/core/math.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/gpu_stage_buffer.hpp>
#include <tavros/assets/image/image.hpp>
#include <tavros/assets/image/image_view.hpp>
#include <tavros/assets/asset_manager.hpp>

namespace tavros::renderer
{

    struct texture_tag : tavros::core::handle_type_registration<0x2001>
    {
    };

    /// @brief Opaque handle to a texture managed by texture_manager.
    using texture_handle = tavros::core::handle_base<texture_tag>;

    /// @brief CPU-side metadata for a GPU texture resource.
    struct gpu_texture_view : core::resource_base
    {
        /// Texture dimensionality type
        rhi::texture_type type = rhi::texture_type::texture_2d;

        /// Pixel format on the GPU
        rhi::pixel_format format = rhi::pixel_format::none;

        /// Width of a single tile/face in pixels
        uint32 width = 0;

        /// Height of a single tile/face in pixels
        uint32 height = 0;

        /// Depth in pixels (texture_3d only)
        uint32 depth = 1;

        /// Number of array layers (texture_2d only)
        uint32 array_layers = 1;

        /// Native GPU texture handle
        rhi::texture_handle gpu_handle;
    };

    /**
     * @brief Manages loading, caching, and lifetime of GPU textures.
     *
     * Supports loading 2D textures, texture arrays, and cubemaps from files
     * or pre-decoded image_view objects. Textures are identified by a string
     * key derived from the file path and load parameters. Loading the same
     * texture with identical parameters returns a cached handle with an
     * incremented reference count.
     *
     * Lifetime is managed via acquire() / release(). When the reference count
     * reaches zero, the GPU resource is destroyed immediately.
     *
     * @note All GPU operations are recorded into the provided command_queue
     *       and executed asynchronously. The staging buffer must remain valid
     *       until the submitted commands have completed on the GPU.
     */
    class texture_manager
    {
    public:
        /**
         * @brief Parameters controlling how a source image is interpreted
         *        and what GPU object is created from it.
         */
        struct load_params
        {
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
            uint32 array_cols = 0; // For texture arrays, number of columns in the source image (0 - detect automatically)
        };

    public:
        using registry_type = core::resource_registry<gpu_texture_view, texture_tag>;
        using const_iterator = registry_type::const_iterator;

        /**
         * @brief Constructs the texture manager.
         * @param am Asset manager used to read image files from disk. Must not be null.
         */
        explicit texture_manager(core::shared_ptr<assets::asset_manager> am) noexcept;

        /**
         * @brief Default destructor.
         */
        ~texture_manager() noexcept = default;

        /**
         * @brief Initializes the manager with a graphics device.
         *
         * Must be called before any load or upload operations.
         *
         * @param device Graphics device used to allocate GPU resources. Must not be null.
         */
        void init(rhi::graphics_device* device) noexcept;

        /**
         * @brief Shuts down the manager and releases all GPU resources.
         *
         * Destroys all cached textures and resets the device pointer.
         * The manager must not be used after this call without a subsequent init().
         */
        void shutdown() noexcept;

        /**
         * @brief Loads a texture from a file on disk.
         *
         * If a texture with the same key (path + params) is already cached,
         * its reference count is incremented and the existing handle is returned.
         *
         * @param stage  Staging buffer used to transfer pixel data to the GPU.
         * @param cmd    Command queue to record copy commands into.
         * @param path   Path to the image file, resolved through the asset manager.
         * @param params Load and creation parameters.
         * @return A valid handle on success, an invalid handle on failure.
         */
        texture_handle load(
            gpu_stage_buffer&   stage,
            rhi::command_queue& cmd,
            core::string_view   path,
            const load_params&  params = {}
        );

        /**
         * @brief Loads a texture from a pre-decoded image_view.
         *
         * If a texture with the given key is already cached, its reference count
         * is incremented and the existing handle is returned.
         * The image_view is assumed to already be in the correct orientation.
         *
         * @param stage  Staging buffer used to transfer pixel data to the GPU.
         * @param cmd    Command queue to record copy commands into.
         * @param im     View into an already-decoded image in CPU memory.
         * @param key    Unique cache key for this texture.
         * @param params Load and creation parameters.
         * @return A valid handle on success, an invalid handle on failure.
         */
        texture_handle load(
            gpu_stage_buffer&   stage,
            rhi::command_queue& cmd,
            assets::image_view  im,
            core::string_view   key,
            const load_params&  params = {}
        );

        /**
         * @brief Increments the reference count of a texture.
         *
         * Must be paired with a corresponding release() call.
         *
         * @param handle Handle to the texture to acquire.
         */
        void acquire(texture_handle handle) noexcept;

        /**
         * @brief Decrements the reference count of a texture.
         *
         * When the reference count reaches zero, the GPU resource is destroyed
         * and the handle becomes invalid.
         *
         * @param handle Handle to the texture to release.
         */
        void release(texture_handle handle);

        /**
         * @brief Returns CPU-side metadata for a texture.
         * @param handle Handle to the texture.
         * @return Pointer to gpu_texture_view, or nullptr if the handle is invalid.
         */
        [[nodiscard]] const gpu_texture_view* get(texture_handle handle) const noexcept;

        /**
         * @brief Returns the native GPU handle for a texture.
         * @param handle Handle to the texture.
         * @return A valid rhi::texture_handle, or an invalid one if the handle is invalid.
         */
        [[nodiscard]] rhi::texture_handle get_gpu_handle(texture_handle handle) const noexcept;

        /**
         * @brief Destroys all cached textures immediately.
         *
         * All GPU resources are freed regardless of their reference count.
         * All previously returned handles become invalid after this call.
         */
        void clear();

        /**
         * @brief Returns const iterator to the beginning.
         */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_registry.begin();
        }

        /**
         * @brief Returns const iterator to the beginning.
         */
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return m_registry.cbegin();
        }

        /**
         * @brief Returns const iterator to the end.
         */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_registry.end();
        }

        /**
         * @brief Returns const iterator to the end.
         */
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return m_registry.cend();
        }

    private:
        /**
         * @brief Uploads image data to the GPU and registers the result in the cache.
         *
         * Handles grid decomposition for texture arrays and face extraction for cubemaps.
         * For texture arrays, the source image is expected to contain all layers packed
         * into a grid of tiles. Grid dimensions are resolved from load_params or
         * auto-detected from array_layers.
         *
         * @param stage   Staging buffer for pixel data transfer.
         * @param cmd     Command queue for copy commands.
         * @param im      Source image data.
         * @param key     Cache key to register the result under.
         * @param params  Load and creation parameters.
         * @param y_flip  If true, tile row order is inverted when sampling the grid.
         *                Pass true when the source image was decoded with y_flip=true
         *                so that layer 0 corresponds to the top row of the grid.
         * @return A valid handle on success, an invalid handle on failure.
         */
        texture_handle upload(gpu_stage_buffer& stage, rhi::command_queue& cmd, assets::image_view im, core::string_view key, const load_params& params, bool y_flip);

    private:
        /// Asset manager for file I/O
        core::shared_ptr<assets::asset_manager> m_am;

        rhi::graphics_device* m_gdevice = nullptr;

        /// Registry of all cached textures
        registry_type m_registry;
    };

} // namespace tavros::renderer
