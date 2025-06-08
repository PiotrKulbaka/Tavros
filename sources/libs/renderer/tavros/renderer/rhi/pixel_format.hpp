#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * This enum defines a comprehensive list of texture formats including:
     * - Normalized formats (UNorm and SNorm)
     * - Integer formats (unsigned and signed)
     * - Floating-point formats (half and full precision)
     * - Depth and depth-stencil formats
     *
     * The enum values follow a naming convention:
     * - `r`, `rg`, `rgb`, `rgba` specify the number of color channels.
     * - Numbers (e.g., 8, 16, 32) specify the bit depth per component.
     * - Suffixes:
     *   - `un`: unsigned normalized (maps to [0, 1] in shader)
     *   - `in`: signed normalized (maps to [-1, 1] in shader)
     *   - `u`: unsigned integer
     *   - `i`: signed integer
     *   - `f`: floating-point (IEEE 754)
     *
     * Special depth and depth-stencil formats are also included.
     * These formats are used for depth testing and shadow mapping.
     */
    enum class pixel_format : uint8
    {
        r8un,              /// 8-bit R channel, unsigned normalized [0, 1]
        r8in,              /// 8-bit R channel, signed normalized [-1, 1]
        r16un,             /// 16-bit R channel, unsigned normalized [0, 1]
        r16in,             /// 16-bit R channel, signed normalized [-1, 1]

        rg8un,             /// 8-bit RG channels, unsigned normalized [0, 1]
        rg8in,             /// 8-bit RG channels, signed normalized [-1, 1]
        rg16un,            /// 16-bit RG channels, unsigned normalized [0, 1]
        rg16in,            /// 16-bit RG channels, signed normalized [-1, 1]

        rgb8un,            /// 8-bit RGB channels, unsigned normalized [0, 1]
        rgb8in,            /// 8-bit RGB channels, signed normalized [-1, 1]
        rgb16un,           /// 16-bit RGB channels, unsigned normalized [0, 1]
        rgb16in,           /// 16-bit RGB channels, signed normalized [-1, 1]

        rgba8un,           /// 8-bit RGBA channels, unsigned normalized [0, 1]
        rgba8in,           /// 8-bit RGBA channels, signed normalized [-1, 1]
        rgba16un,          /// 16-bit RGBA channels, unsigned normalized [0, 1]
        rgba16in,          /// 16-bit RGBA channels, signed normalized [-1, 1]

        r8u,               /// 8-bit R channel, unsigned integer [0, 255]
        r8i,               /// 8-bit R channel, signed integer [-128, 127]
        r16u,              /// 16-bit R channel, unsigned integer
        r16i,              /// 16-bit R channel, signed integer
        r32u,              /// 32-bit R channel, unsigned integer
        r32i,              /// 32-bit R channel, signed integer

        rg8u,              /// 8-bit RG channels, unsigned integer
        rg8i,              /// 8-bit RG channels, signed integer
        rg16u,             /// 16-bit RG channels, unsigned integer
        rg16i,             /// 16-bit RG channels, signed integer
        rg32u,             /// 32-bit RG channels, unsigned integer
        rg32i,             /// 32-bit RG channels, signed integer

        rgb8u,             /// 8-bit RGB channels, unsigned integer
        rgb8i,             /// 8-bit RGB channels, signed integer
        rgb16u,            /// 16-bit RGB channels, unsigned integer
        rgb16i,            /// 16-bit RGB channels, signed integer
        rgb32u,            /// 32-bit RGB channels, unsigned integer
        rgb32i,            /// 32-bit RGB channels, signed integer

        rgba8u,            /// 8-bit RGBA channels, unsigned integer
        rgba8i,            /// 8-bit RGBA channels, signed integer
        rgba16u,           /// 16-bit RGBA channels, unsigned integer
        rgba16i,           /// 16-bit RGBA channels, signed integer
        rgba32u,           /// 32-bit RGBA channels, unsigned integer
        rgba32i,           /// 32-bit RGBA channels, signed integer

        r16f,              /// 16-bit R channel, IEEE float
        r32f,              /// 32-bit R channel, IEEE float
        rg16f,             /// 16-bit RG channels, IEEE float
        rg32f,             /// 32-bit RG channels, IEEE float
        rgb16f,            /// 16-bit RGB channels, IEEE float
        rgb32f,            /// 32-bit RGB channels, IEEE float
        rgba16f,           /// 16-bit RGBA channels, IEEE float
        rgba32f,           /// 32-bit RGBA channels, IEEE float

        depth16,           /// 16-bit depth, fixed-point [0, 1]
        depth24,           /// 24-bit depth, fixed-point [0, 1]
        depth32f,          /// 32-bit depth, IEEE float
        depth24_stencil8,  /// Packed 24-bit depth + 8-bit stencil
        depth32f_stencil8, /// Packed 32-bit float depth + 8-bit stencil
    };

} // namespace tavros::renderer
