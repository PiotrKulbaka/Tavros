#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::renderer
{

    /**
     * @brief Predefined sampler presets covering the most common texture sampling scenarios
     */
    enum class sampler_preset : uint8
    {
        /// Automatically selects an appropriate preset.
        /// Resolves to trilinear_repeat by default.
        automatic = 0,

        /// Nearest filtering, clamp to edge. Suitable for pixel art, UI elements, and render target reads.
        nearest_clamp,

        /// Nearest filtering, repeat wrap. Suitable for tiled textures without filtering.
        nearest_repeat,

        /// Linear filtering, clamp to edge. Suitable for UI, fullscreen effects, and post-processing.
        linear_clamp,

        /// Linear filtering, repeat wrap. Suitable for generic textures without mipmaps.
        linear_repeat,

        /// Trilinear filtering, clamp to edge. Suitable for skyboxes, environment maps, and unique textures.
        trilinear_clamp,

        /// Trilinear filtering, repeat wrap. Default choice for most material textures.
        trilinear_repeat,

        /// Nearest filtering with depth comparison enabled. Suitable for shadow map sampling.
        shadow,

        /// Linear filtering with depth comparison enabled. Suitable for PCF shadow map sampling.
        shadow_pcf,

        /// Number of presets
        count,
    };


    /**
     * @brief Library of predefined sampler objects.
     *
     * Creates and owns a set of commonly used sampler states represented by
     * @ref sampler_preset. The library centralizes sampler creation and allows
     * the renderer to reuse a small set of immutable sampler objects instead of
     * creating duplicate sampler states for individual textures or materials.
     *
     * Samplers are created during construction and remain valid for the entire
     * lifetime of the library.
     */
    class sampler_library
    {
    public:
        /**
         * @brief Maximum number of predefined sampler presets.
         */
        static constexpr auto k_max_presets = static_cast<size_t>(sampler_preset::count);

    public:
        /**
         * @brief Creates the sampler library and initializes all predefined samplers.
         *
         * @param gdevice Graphics device used to create sampler objects.
         * @param anisotropy Maximum anisotropy level applied to presets that support
         *        anisotropic filtering. Presets that do not support anisotropy ignore
         *        this value.
         */
        sampler_library(rhi::graphics_device* gdevice, float anisotropy = 8.0f);

        /**
         * @brief Destroys all sampler objects owned by the library.
         */
        ~sampler_library() noexcept;

        /**
         * @brief Returns a sampler handle for the specified preset.
         *
         * @param preset Predefined sampler preset to retrieve.
         * @return Handle to the corresponding sampler object.
         */
        rhi::sampler_handle get_sampler(sampler_preset preset) const noexcept;

    private:
        rhi::graphics_device* m_gdevice;

        core::fixed_vector<rhi::sampler_handle, k_max_presets> m_samplers;
    };

} // namespace tavros::renderer
