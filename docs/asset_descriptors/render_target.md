# Render Target Definition Reference

A **render target** describes a set of buffers (attachments) that a render pass writes into.
It is configured as a TEF object and can contain up to 8 color attachments, a depth buffer, and a stencil buffer.

At least one of `color`, `depth`, or `stencil` must be defined.

---

## Top-level structure

```
my_render_target = {
    color   = { ... }   # optional, up to 8 attachments
    depth   = { ... }   # optional
    stencil = { ... }   # optional
}
```

---

## `color`

Container for color attachments. Each key is an arbitrary attachment name; it must match the name
used by the material in its own `color` section.

```
color = {
    base_color = { ... }
    normal     = { ... }
    # up to 8 attachments
}
```

### Color attachment fields

```
base_color = {
    format = "rgba8"
    load   = "clear"
    store  = "store"
    clear  = 0.0 0.0 0.0 0.0
}
```

| Field   | Required | Default   | Description                                                      |
|---------|----------|-----------|------------------------------------------------------------------|
| `format`| **yes**  | -         | Pixel format of the buffer                                       |
| `load`  | no       | `"clear"` | What to do with the buffer at the start of the pass             |
| `store` | no       | `"store"` | What to do with the buffer at the end of the pass               |
| `clear` | no       | `0 0 0 0` | Clear value: 4 numbers (r g b a), channel count matches `format` |

**`format` - color formats:**

| Value      | Channels | Type          |
|------------|----------|---------------|
| `"rgba8"`  | 4        | 8-bit unorm   |
| `"rgba16f"`| 4        | 16-bit float  |
| `"rgba32f"`| 4        | 32-bit float  |
| `"rgb8"`   | 3        | 8-bit unorm   |
| `"rgb16f"` | 3        | 16-bit float  |
| `"rgb32f"` | 3        | 32-bit float  |
| `"rg8"`    | 2        | 8-bit unorm   |
| `"rg16f"`  | 2        | 16-bit float  |
| `"rg32f"`  | 2        | 32-bit float  |
| `"r8"`     | 1        | 8-bit unorm   |
| `"r16f"`   | 1        | 16-bit float  |
| `"r32f"`   | 1        | 32-bit float  |

**`load` - load operation:**

| Value         | Description                                                    |
|---------------|----------------------------------------------------------------|
| `"clear"`     | Fill the buffer with the `clear` value before the pass        |
| `"load"`      | Preserve the existing contents of the buffer                  |
| `"dont_care"` | Contents are undefined - faster, but previous data is lost    |

**`store` - store operation:**

| Value         | Description                                                    |
|---------------|----------------------------------------------------------------|
| `"store"`     | Write the pass result into the buffer                         |
| `"dont_care"` | Pass result is not needed - buffer is not written back        |

---

## `depth`

Depth buffer configuration. If the section is omitted, depth is not used.

```
depth = {
    format = "depth32f"
    load   = "clear"
    store  = "dont_care"
    clear  = 1.0
}
```

| Field   | Required | Default        | Description                                   |
|---------|----------|----------------|-----------------------------------------------|
| `format`| **yes**  | -              | Pixel format of the depth buffer              |
| `load`  | no       | `"clear"`      | Operation at the start of the pass            |
| `store` | no       | `"dont_care"`  | Operation at the end of the pass              |
| `clear` | no       | `1.0`          | Clear value, a number clamped to [0.0, 1.0]   |

**`format` - depth formats:**

| Value       | Description          |
|-------------|----------------------|
| `"depth24"` | 24-bit depth         |
| `"depth32f"`| 32-bit float depth   |

> `store = "dont_care"` is the typical choice when depth is only needed within the pass itself
> (e.g. forward rendering with no subsequent depth reads).

---

## `stencil`

Stencil buffer configuration. If the section is omitted, stencil is not used.

```
stencil = {
    format = "stencil8"
    load   = "clear"
    store  = "dont_care"
    clear  = 0
}
```

| Field   | Required | Default        | Description                                        |
|---------|----------|----------------|----------------------------------------------------|
| `format`| **yes**  | -              | Pixel format of the stencil buffer                 |
| `load`  | no       | `"clear"`      | Operation at the start of the pass                 |
| `store` | no       | `"dont_care"`  | Operation at the end of the pass                   |
| `clear` | no       | `0`            | Clear value, an integer clamped to [0, 255]        |

**`format` - stencil formats:**

| Value        | Description    |
|--------------|----------------|
| `"stencil8"` | 8-bit stencil  |

---

## Examples

### Main offscreen buffer (color + depth)

```
main_offscreen_rt = {
    color = {
        base_color = {
            format = "rgba8"
            load   = "clear"
            store  = "store"
            clear  = 0.0 0.0 0.0 0.0
        }
    }

    depth = {
        format = "depth32f"
        load   = "clear"
        store  = "dont_care"
        clear  = 1.0
    }
}
```

### G-Buffer (multiple color attachments)

```
gbuffer_rt = {
    color = {
        base_color = {
            format = "rgba8"
            load   = "clear"
            store  = "store"
            clear  = 0.0 0.0 0.0 1.0
        }
        normal = {
            format = "rgba16f"
            load   = "clear"
            store  = "store"
            clear  = 0.0 0.0 0.0 0.0
        }
        emissive = {
            format = "rgb16f"
            load   = "clear"
            store  = "store"
            clear  = 0.0 0.0 0.0 0.0
        }
    }

    depth = {
        format = "depth32f"
        load   = "clear"
        store  = "store"     # depth is read in subsequent passes
        clear  = 1.0
    }
}
```

### Shadow map (depth only)

```
shadow_map_rt = {
    depth = {
        format = "depth32f"
        load   = "clear"
        store  = "store"
        clear  = 1.0
    }
}
```

### Stencil masking pass

```
stencil_mask_rt = {
    color = {
        base_color = {
            format = "rgba8"
            load   = "load"      # preserve the previous frame
            store  = "store"
        }
    }

    stencil = {
        format = "stencil8"
        load   = "clear"
        store  = "dont_care"
        clear  = 0
    }
}
```
