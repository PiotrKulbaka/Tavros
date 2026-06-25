# Texture Definition Reference

A **texture** descriptor tells the loader where to find an image file and how to interpret
and upload it to the GPU. All parameters except `path` are optional.

---

## Structure

```
my_texture = {
    path        = "tavros/textures/my_image.png"
    type        = "texture_2d"
    gen_mipmaps = true
}
```

---

## Fields

| Field          | Required | Default        | Description                                                      |
|----------------|----------|----------------|------------------------------------------------------------------|
| `path`         | **yes**  | -              | Path to the source image file                                    |
| `type`         | no       | `"texture_2d"` | Texture dimensionality / shape                                   |
| `pixel_format` | no       | `"none"`       | Override pixel format; `"none"` means infer from source image    |
| `left`         | no       | `0`            | X offset into the source image (pixels)                          |
| `top`          | no       | `0`            | Y offset into the source image (pixels)                          |
| `width`        | no       | `0` (full)     | Crop width; `0` means use full image width from `left`           |
| `height`       | no       | `0` (full)     | Crop height; `0` means use full image height from `top`          |
| `depth`        | no       | `1`            | Depth extent, used for `texture_3d`                              |
| `gen_mipmaps`  | no       | `false`        | Generate a full mip chain on upload                              |
| `array_layers` | no       | `1`            | Number of array layers (see [Array textures](#array-textures))   |
| `array_cols`   | no       | `1`            | Columns in the sprite-sheet grid                                 |
| `array_rows`   | no       | `1`            | Rows in the sprite-sheet grid                                    |

---

## `type`

| Value            | Description                                                        |
|------------------|--------------------------------------------------------------------|
| `"texture_2d"`   | Standard 2D texture. Also used for 2D texture arrays              |
| `"texture_cube"` | Cubemap. Source image must be a horizontal strip of 6 equal faces  |
| `"texture_3d"`   | 3D volumetric texture. Use `depth` to set the Z extent             |

---

## `pixel_format`

Overrides the pixel format used when uploading to the GPU. When set to `"none"` (default),
the format is inferred from the source image channels.

**Normalized formats** (values mapped to `[0.0, 1.0]` or `[-1.0, 1.0]`):

| Value      | Channels | Bits | Type           |
|------------|----------|------|----------------|
| `"r8un"`   | 1        | 8    | unsigned norm  |
| `"r8in"`   | 1        | 8    | signed norm    |
| `"r16un"`  | 1        | 16   | unsigned norm  |
| `"r16in"`  | 1        | 16   | signed norm    |
| `"rg8un"`  | 2        | 8    | unsigned norm  |
| `"rg8in"`  | 2        | 8    | signed norm    |
| `"rg16un"` | 2        | 16   | unsigned norm  |
| `"rg16in"` | 2        | 16   | signed norm    |
| `"rgb8un"` | 3        | 8    | unsigned norm  |
| `"rgb8in"` | 3        | 8    | signed norm    |
| `"rgb16un"`| 3        | 16   | unsigned norm  |
| `"rgb16in"`| 3        | 16   | signed norm    |
| `"rgba8un"`| 4        | 8    | unsigned norm  |
| `"rgba8in"`| 4        | 8    | signed norm    |
| `"rgba16un"`| 4       | 16   | unsigned norm  |
| `"rgba16in"`| 4       | 16   | signed norm    |

> RGB formats (`rgb*`) are rarely natively supported by GPUs. Prefer `rgba*` equivalents where possible.

**Integer formats** (raw integer values, not normalized):

| Value       | Channels | Bits | Type     |
|-------------|----------|------|----------|
| `"r8u"`     | 1        | 8    | unsigned |
| `"r8i"`     | 1        | 8    | signed   |
| `"r16u"`    | 1        | 16   | unsigned |
| `"r16i"`    | 1        | 16   | signed   |
| `"r32u"`    | 1        | 32   | unsigned |
| `"r32i"`    | 1        | 32   | signed   |
| `"rg8u"`    | 2        | 8    | unsigned |
| `"rg8i"`    | 2        | 8    | signed   |
| `"rg16u"`   | 2        | 16   | unsigned |
| `"rg16i"`   | 2        | 16   | signed   |
| `"rg32u"`   | 2        | 32   | unsigned |
| `"rg32i"`   | 2        | 32   | signed   |
| `"rgba8u"`  | 4        | 8    | unsigned |
| `"rgba8i"`  | 4        | 8    | signed   |
| `"rgba16u"` | 4        | 16   | unsigned |
| `"rgba16i"` | 4        | 16   | signed   |
| `"rgba32u"` | 4        | 32   | unsigned |
| `"rgba32i"` | 4        | 32   | signed   |

**Floating point formats:**

| Value       | Channels | Bits |
|-------------|----------|------|
| `"r16f"`    | 1        | 16   |
| `"r32f"`    | 1        | 32   |
| `"rg16f"`   | 2        | 16   |
| `"rg32f"`   | 2        | 32   |
| `"rgb16f"`  | 3        | 16   |
| `"rgb32f"`  | 3        | 32   |
| `"rgba16f"` | 4        | 16   |
| `"rgba32f"` | 4        | 32   |

**Depth / stencil formats** (not typically used as regular sampled textures):

| Value                  | Description                  |
|------------------------|------------------------------|
| `"depth16"`            | 16-bit depth                 |
| `"depth24"`            | 24-bit depth                 |
| `"depth32f"`           | 32-bit float depth           |
| `"stencil8"`           | 8-bit stencil                |
| `"depth24_stencil8"`   | 24-bit depth + 8-bit stencil |
| `"depth32f_stencil8"`  | 32-bit float depth + stencil |

---

## `gen_mipmaps`

When `true`, a full mip chain is generated from the base level on upload.
The number of levels is derived automatically: `floor(log2(max(w, h))) + 1`.

---

## Source rect (`left`, `top`, `width`, `height`)

Defines a sub-region of the source image to use. Useful when a single image file contains
multiple logical textures.

```
ui_button = {
    path   = "tavros/textures/ui_atlas.png"
    left   = 256
    top    = 0
    width  = 128
    height = 64
}
```

If `width` or `height` is `0`, the remaining extent from the given offset is used.

---

## Array textures

When `array_layers > 1`, the source image is treated as a sprite sheet and each tile
becomes one layer of a `texture_2d` array.

The grid layout is controlled by `array_cols` and `array_rows`:

| `array_cols` | `array_rows` | Behaviour                                              |
|--------------|--------------|--------------------------------------------------------|
| `0`          | `0`          | Grid is inferred automatically (square-ish layout)     |
| N            | `0`          | Rows are derived: `ceil(array_layers / N)`             |
| N            | M            | Grid is used as specified                              |
| `0`          | M            | **Error** - ambiguous, `array_cols` must be set first  |

Tile dimensions are computed as:
```
tile_w = source_width  / array_cols
tile_h = source_height / array_rows
```

```
walk_anim = {
    path         = "characters/hero_walk.png"
    array_layers = 4
    array_cols   = 4
    array_rows   = 1
    gen_mipmaps  = false
}
```

---

## Cubemap (`texture_cube`)

The source image must be a **horizontal strip** of exactly 6 equal square faces
in the order: `+X`, `-X`, `+Y`, `-Y`, `+Z`, `-Z`.

Required constraint: `width == height * 6`.

```
sky_cubemap = {
    path        = "tavros/textures/sky_faces.png"
    type        = "texture_cube"
    gen_mipmaps = true
}
```

> Face order swap between `+Y` / `-Y` (indices 2 and 3) is applied automatically by the loader.

---

## Examples

### Simple 2D texture

```
dark_sky = {
    path        = "tavros/textures/dark_sky.png"
    gen_mipmaps = true
}
```

### Normal map with explicit format

```
rock_normal = {
    path         = "tavros/textures/rock_normal.png"
    pixel_format = "rgba8un"
    gen_mipmaps  = true
}
```

### Cropped region from an atlas

```
ui_button = {
    path   = "tavros/textures/ui_atlas.png"
    left   = 256
    top    = 0
    width  = 128
    height = 64
}
```

### 2D texture array from a sprite sheet

```
explosion_frames = {
    path         = "tavros/textures/explosion_sheet.png"
    array_layers = 16
    array_cols   = 4
    array_rows   = 4
    gen_mipmaps  = false
}
```

### Cubemap

```
environment_map = {
    path        = "tavros/textures/env_faces.png"
    type        = "texture_cube"
    gen_mipmaps = true
}
```
