# 05 - Rendering, Models & Camera API

BineCrabricCPP includes a 3D rendering pipeline hook system, custom model baking builders, sprite-to-voxel extruders, texture pixel analysis tools, and camera controls.

---

## 1. Render Pipeline & Pass Hooks (`minecraft.render`)

Mods hook into world render passes by subscribing to `world_render` for specific stages and moments.

### Render Pipeline Stages (`minecraft.render.stages`)

| Stage Name | Execution Context | Typical Use Cases |
| :--- | :--- | :--- |
| `"sky"` | Sky dome rendering phase. | Custom twilight, celestial bodies, colorful skies. |
| `"stars"` | Starfield render pass. | Astronomy transforms, custom constellations. |
| `"terrain_opaque"` | Opaque chunk geometry. | Custom terrain quads, ground overlays. |
| `"entities"` | World entity rendering pass. | Custom 3D entity renders, floating drops. |
| `"particles_lit"` | Lit particle pass. | Fire, glowing ember quads. |
| `"particles"` | Standard particle pass. | Custom atmospheric & combat particles. |
| `"terrain_translucent"` | Translucent blocks (water, ice, stained glass). | Custom fluid quads, underwater effects. |
| `"weather"` | Rain and snow passes. | Weather particles, storm effects. |
| `"clouds"` | Cloud layer rendering. | Multi-layered cloud meshes, volumetric clouds. |
| `"hand"` | First-person held item pass. | Custom item animations, held lanterns. |
| `"framebuffer"` | Screen-space post-processing pass. | Screen shaders, depth fog, vignette overlays. |

### Render Moments (`minecraft.render.moments`)
- `"before"`: Executes before native C++ rendering for the stage. Setting `event.cancelled = true` suppresses native rendering.
- `"after"`: Executes after native C++ rendering completes for the stage.

### Immediate 3D Quad Drawing

#### `minecraft.render.draw_quads(quads_array)`
- **Signature**: `minecraft.render.draw_quads(quads: table) -> integer`
- **Behavior**: Submits an array of 3D quad tables into the active tessellator pass during `world_render`.
- **Quad Schema**:
  ```lua
  {
    texture = "textures/effects/glow.png",
    r = 1.0, g = 0.8, b = 0.2, a = 0.9,
    vertices = {
      { x = 0, y = 0, z = 0, u = 0, v = 0 },
      { x = 1, y = 0, z = 0, u = 1, v = 0 },
      { x = 1, y = 1, z = 0, u = 1, v = 1 },
      { x = 0, y = 1, z = 0, u = 0, v = 1 }
    }
  }
  ```

---

## 2. Model Baking & Voxel Builders (`minecraft.model`)

Lua mods can generate and bake 3D quad models on top of the native GPU model system.

### `minecraft.model.build(spec)`
- **Signature**: `minecraft.model.build(spec: table) -> model_handle`
- Bakes arbitrary 3D quads into an engine model handle.

### `minecraft.model.voxels(opts)`
- **Signature**: `minecraft.model.voxels(opts: table) -> model_handle, err_msg`
- **Behavior**: Constructs a 3D voxel mesh from an integer cell lattice with automatic interior-face culling.
- **Parameters**:
  - `cells`: Array of voxel cells `{ { x = 0, y = 0, z = 0, r = 1, g = 1, b = 1, a = 1 }, ... }`.
  - `resolution`: Voxel grid density (default `16`).
  - `scale`: Scale factor (defaults to `1 / resolution`).
  - `origin_x`, `origin_y`, `origin_z`: Offset coordinates (default `0`).
  - `key`: Cache key identifier.

### `minecraft.model.voxel(spec)`
- **Signature**: `minecraft.model.voxel(spec: table) -> model_handle, err_msg`
- **Behavior**: Reads a 2D PNG sprite texture or terrain atlas tile, analyzes pixel alpha values, and extrudes it into a 1-voxel-thick 3D mesh.
- **Parameters**:
  - `texture`: Texture image file path.
  - `atlas_index`: Terrain atlas index (-1 if standalone mod texture).
  - `mod_texture`: Boolean indicating if `texture` is a mod path.
  - `grid`: Resolution grid (default `16`).
  - `alpha_cutoff`: Alpha threshold 0..255 below which pixels are discarded (default `30`).

---

## 3. Texture Metadata & Pixel Inspection (`minecraft.texture`)

### `minecraft.texture.size(path)`
- **Signature**: `minecraft.texture.size(path: string) -> { width: integer, height: integer }`
- **Returns**: Pixel dimensions of target image asset.

### `minecraft.texture.pixel(path, x, y)`
- **Signature**: `minecraft.texture.pixel(path: string, x: integer, y: integer) -> { r: int, g: int, b: int, a: int }`
- **Returns**: RGBA color component values (0..255) of pixel at `(x, y)`.

---

## 4. Camera & Environment API (`minecraft.camera`, `minecraft.colors`)

### Camera Setup & Dynamic FOV
- Subscribe to `minecraft.events.camera_setup` to modify camera position `(x, y, z)` or rotation `(yaw, pitch)`.
- Subscribe to `minecraft.events.fov` to modify field of view dynamically (e.g., sprint zoom, scope effects):
  ```lua
  minecraft.on(minecraft.events.fov, { priority = 100 }, function(event)
    event.fov_multiplier = event.fov_multiplier * 1.15 -- 15% FOV boost
    return event
  end)
  ```

### Sky & Fog Color Overrides
- Modify sky and fog colors dynamically by subscribing to `world_color` or `fog_settings` events.
