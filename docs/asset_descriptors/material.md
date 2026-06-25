# Material Definition Reference

A **material** is a TEF object that fully describes the GPU pipeline state used to render a mesh.
It references shader sources and declares all fixed-function state: blending, depth, stencil,
rasterization, and primitive topology.

---

## Top-level structure

```
my_material = {
    shaders    = { ... }
    color      = { ... }
    depth      = "off" | { ... }
    stencil    = "off" | { ... }
    topology   = STRING
    rasterizer = { ... }
}
```

---

## `shaders`

Paths to GLSL source files. Currently two stages are supported.

```
shaders = {
    vertex   = "tavros/shaders/my.vert"
    fragment = "tavros/shaders/my.frag"
}
```

| Key        | Type   | Required | Description                  |
|------------|--------|----------|------------------------------|
| `vertex`   | string | yes      | Path to vertex shader source |
| `fragment` | string | yes      | Path to fragment shader source |

---

## `color`

Per-attachment blend and write-mask configuration. Up to **8** attachments can be listed.
Each key must match an attachment name declared in the render target (e.g. `base_color`, `normal`).

Each attachment value is either `"off"` (disable all writes) or an object:

```
color = {
    base_color = {
        mask  = "rgba"
        blend = "off" | PRESET | { ... }
    }
    normal = "off"
}
```

### `mask`

Controls which channels are written. Any combination of `r`, `g`, `b`, `a` is valid.

```
mask = "rgba"   # write all channels
mask = "rgb"    # skip alpha
mask = "r"      # red only
```

### `blend`

Can be `"off"`, a **preset string**, or a full **blend object**.

**Presets:**

| Value           | Meaning                                    |
|-----------------|--------------------------------------------|
| `"off"`         | No blending, source overwrites destination |
| `"alpha"`       | Standard alpha blending                    |
| `"additive"`    | Source added to destination                |
| `"multiply"`    | Source multiplied with destination         |
| `"premul_alpha"`| Pre-multiplied alpha blending              |
| `"min"`         | Per-channel minimum                        |
| `"max"`         | Per-channel maximum                        |

**Full blend object:**

```
blend = {
    src_color = "src_alpha"
    dst_color = "one_minus_src_alpha"
    color_op  = "add"
    src_alpha = "one"
    dst_alpha = "one_minus_src_alpha"
    alpha_op  = "add"
}
```

**Factor values** (`src_color`, `dst_color`, `src_alpha`, `dst_alpha`):

`zero` ; `one` ; `src_color` ; `one_minus_src_color` ; `dst_color` ; `one_minus_dst_color` ;
`src_alpha` ; `one_minus_src_alpha` ; `dst_alpha` ; `one_minus_dst_alpha`

**Operation values** (`color_op`, `alpha_op`):

`add` ; `subtract` ; `reverse_subtract` ; `min` ; `max`

---

## `depth`

Depth test and write configuration. Use `"off"` to disable entirely.

```
depth = {
    test       = true
    write      = true
    compare_op = "less"
}
```

| Key          | Type   | Description                        |
|--------------|--------|------------------------------------|
| `test`       | bool   | Enable depth testing               |
| `write`      | bool   | Write passing fragments to depth buffer |
| `compare_op` | string | Comparison function (see below)    |

**`compare_op` values:**

`less` ; `equal` ; `less_equal` ; `greater` ; `greater_equal` ; `not_equal` ; `always`

**Typical combinations:**

```
# Opaque geometry - default
depth = { test = true  write = true  compare_op = "less" }

# Transparent geometry - test but don't occlude
depth = { test = true  write = false compare_op = "less" }

# Fullscreen pass / skybox - skip depth entirely
depth = "off"
```

---

## `stencil`

Stencil test configuration. Use `"off"` to disable.

Front and back faces are configured independently. Each face value is either `"off"` or a face object.

```
stencil = {
    test  = true
    front = { ... }
    back  = { ... }
}
```

| Key     | Type | Description                          |
|---------|------|--------------------------------------|
| `test`  | bool | Enable stencil testing               |
| `front` | face | State applied to front-facing fragments |
| `back`  | face | State applied to back-facing fragments  |

### Face object

```
front = {
    read_mask       = 0xFF
    write_mask      = 0xFF
    ref_value       = 0
    compare_op      = "always"
    stencil_fail_op = "keep"
    depth_fail_op   = "keep"
    pass_op         = "keep"
}
```

| Key              | Range    | Description                                           |
|------------------|----------|-------------------------------------------------------|
| `read_mask`      | 0-255    | ANDed with ref and stored value before comparison     |
| `write_mask`     | 0-255    | Mask applied to stencil writes                        |
| `ref_value`      | 0-255    | Reference value used in comparison                    |
| `compare_op`     | string   | How ref is compared to stored value                   |
| `stencil_fail_op`| string   | Op when stencil test fails                            |
| `depth_fail_op`  | string   | Op when stencil passes but depth fails                |
| `pass_op`        | string   | Op when both tests pass                               |

**`compare_op`** - same set as depth: `less` ; `equal` ; `less_equal` ; `greater` ; `greater_equal` ; `not_equal` ; `always`

**Stencil op values** (`stencil_fail_op`, `depth_fail_op`, `pass_op`):

`keep` ; `zero` ; `replace` ; `increment_clamp` ; `decrement_clamp` ; `invert` ; `increment_wrap` ; `decrement_wrap`

---

## `topology`

Primitive assembly mode.

```
topology = "triangles"
```

| Value             | Description                        |
|-------------------|------------------------------------|
| `"points"`        | Each vertex is a point             |
| `"lines"`         | Every 2 vertices form a line       |
| `"line_strip"`    | Connected line sequence            |
| `"triangles"`     | Every 3 vertices form a triangle   |
| `"triangle_strip"`| Connected triangle sequence        |

---

## `rasterizer`

Controls how primitives are rasterized.

```
rasterizer = {
    cull        = "back"
    front_face  = "ccw"
    fill_mode   = "fill"
    depth_clamp = "off" | { min = NUMBER  max = NUMBER }
    depth_bias  = "off" | { constant = NUMBER  slope = NUMBER  clamp = NUMBER }
}
```

| Key           | Default  | Description                              |
|---------------|----------|------------------------------------------|
| `cull`        | `"off"`  | Face culling: `"off"` ; `"front"` ; `"back"` |
| `front_face`  | `"ccw"`  | Winding order considered front: `"ccw"` ; `"cw"` |
| `fill_mode`   | `"fill"` | `"fill"` ; `"lines"` ; `"points"`       |
| `depth_clamp` | `"off"`  | Clamp depth to [min, max] instead of clipping |
| `depth_bias`  | `"off"`  | Add a bias to fragment depth values      |

### `depth_clamp`

```
depth_clamp = { min = 0.0  max = 1.0 }
```

### `depth_bias`

Useful for shadow mapping to avoid self-shadowing artefacts.

```
depth_bias = {
    constant = 1.0    # flat offset added to every fragment
    slope    = 1.5    # scales with surface slope relative to light
    clamp    = 0.0    # maximum absolute bias (0.0 = unclamped)
}
```

---

## Complete examples

### Opaque mesh

```
opaque_mesh = {
    shaders = {
        vertex   = "tavros/shaders/static_mesh.vert"
        fragment = "tavros/shaders/pbr.frag"
    }

    color = {
        base_color = { mask = "rgba"  blend = "off" }
    }

    depth = { test = true  write = true  compare_op = "less" }

    stencil = "off"

    topology = "triangles"

    rasterizer = {
        cull       = "back"
        front_face = "ccw"
        fill_mode  = "fill"
        depth_clamp = "off"
        depth_bias  = "off"
    }
}
```

### Transparent geometry

```
transparent_quad = {
    shaders = {
        vertex   = "tavros/shaders/quad.vert"
        fragment = "tavros/shaders/unlit.frag"
    }

    color = {
        base_color = { mask = "rgba"  blend = "alpha" }
    }

    depth    = { test = true  write = false  compare_op = "less" }
    stencil  = "off"
    topology = "triangles"

    rasterizer = {
        cull       = "off"
        front_face = "ccw"
        fill_mode  = "fill"
        depth_clamp = "off"
        depth_bias  = "off"
    }
}
```

### Wireframe debug

```
debug_wireframe = {
    shaders = {
        vertex   = "tavros/shaders/static_mesh.vert"
        fragment = "tavros/shaders/solid_color.frag"
    }

    color = {
        base_color = { mask = "rgb"  blend = "off" }
    }

    depth    = { test = true  write = false  compare_op = "less_equal" }
    stencil  = "off"
    topology = "triangles"

    rasterizer = {
        cull       = "off"
        front_face = "ccw"
        fill_mode  = "lines"
        depth_clamp = "off"
        depth_bias  = { constant = 1.0  slope = 1.0  clamp = 0.0 }
    }
}
```
