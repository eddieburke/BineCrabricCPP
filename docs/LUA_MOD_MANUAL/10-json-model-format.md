# 10 — JSON Model Format

Reference for the JSON model schema consumed by `minecraft.model.load(path)`
(see [06-rendering.md](06-rendering.md)). Source of truth is
`native/src/net/minecraft/mod/model/ModModels.cpp` (parsing: `parseJsonModel`/
`parseElement`/`parseFace`/`parseRotation` around lines 90-235; baking:
`bakeJsonModel` around line 398). This doc exists so agents don't have to
re-read that file every time — if the two disagree, the `.cpp` wins.

## Top-level shape

```json
{
  "format_version": "1.21.11",
  "parent": "optional/parent/model",
  "textures": { "all": "mods/mymod/foo.png" },
  "elements": [ /* JsonModelElement, see below */ ]
}
```

- `format_version` is read but not validated against — any value is accepted.
- `parent`: resolves and flattens one level of inheritance (see **Parent
  chains** below). `elements` and `textures` from the child always win;
  parent-only entries fill the gaps.
- `textures`: a plain string map, `"key": "reference"`. Values follow the
  **Texture references** rules below (`#other-key`, `ns:name`, plain path).
- `elements`: array of boxes that make up the model. A model with no elements
  (after parent flattening) bakes to nothing.

## Element

```json
{
  "from": [x1, y1, z1],
  "to": [x2, y2, z2],
  "shade": true,
  "rotation": { "origin": [8,8,8], "axis": "y", "angle": 45, "rescale": false },
  "faces": { "north": { /* JsonModelFaceSpec */ }, "...": {} }
}
```

- `from`/`to`: required, 3 numbers each, in **0–16 units per block** (vanilla
  pixel-space), not 0–1. Baking divides by 16 to get block-space quads.
  Coordinates may go outside `0..16` (parts sticking out of the block) — the
  bake math doesn't clamp them.
- `shade`: `true` (default) applies vanilla directional face-shading
  (`kFaceShade`: down 0.5, up 1.0, north/south 0.8, west/east 0.6). Set
  `false` for a flat, unshaded element (all faces full brightness).
- `rotation` (optional): see **Rotation** below.
- `faces`: an object keyed by face name — only `down`, `up`, `north`,
  `south`, `west`, `east` are recognized (`bottom` is *not* accepted as an
  alias here, unlike `cullface` values — see below). **A face not listed is
  simply not emitted** — there is no implicit "close the box" behavior.
  Nothing else in the engine adds missing faces for you.

### Faces are genuinely one-sided — declare both sides yourself

There is no "double-sided" or "no-cull" flag anywhere in this format. Vanilla
back-face culling (GL `cull = true` on the solid/cutout render pass) means a
single declared face is visible from exactly one direction. If you want a
thin plane (a pane, a bar, a leaf) visible from both sides, **declare both
opposing faces on the same plane** (e.g. both `north` and `south` at the same
`z`), each with the appropriate texture/uv for that side. This is exactly how
vanilla and Blockbench author thin/crossed geometry — see
`native/mods/simple_lantern/models/lantern.json` for a real example (four
crossed 1-unit-thick planes, each declaring only its two big faces, nothing
on the thin edges).

Do not "helpfully" add the other 4 faces to a thin plane element just because
`faces` looks incomplete — a full-cube-style closed box is a different (and
usually wrong) shape for what's normally meant to be a flat cross/billboard
element. Match the source Blockbench project's intent, not the count of keys.

## Face (`JsonModelFaceSpec`)

```json
{ "uv": [u1, v1, u2, v2], "texture": "#all", "rotation": 90, "tintindex": 0, "cullface": "north" }
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `uv` | `[u1,v1,u2,v2]` | auto (see below) | 0–16 pixel-space, **not** normalized 0–1. `u2>u1` and `v2>v1` are not enforced but should hold or the quad bakes flipped. |
| `texture` | string | *(required, effectively)* | See **Texture references**. Missing/unresolvable texture silently drops the face (no error surfaced). |
| `rotation` | int | `0` | Must be a multiple of 90 (parse error otherwise); normalized to `0..359`. Rotates which UV corner maps to which vertex, not the texture pixels themselves. |
| `tintindex` | int | `-1` | **Parsed and stored on the baked quad but not currently consumed anywhere in the render path.** Setting it has no visible effect yet — don't rely on it for foliage/grass-style tinting. |
| `cullface` | string | *(none)* | One of the 6 face names, or `"bottom"` as an alias for `"down"` (this alias only applies to `cullface`, not the `faces` object key). Only meaningful for blocks (not items/inventory rendering); see **cullface semantics** below. |

If `uv` is omitted, it's derived from `from`/`to` and the face direction
(`autoUv`) the same way vanilla does — i.e. the face is auto-mapped onto the
matching region of a 16×16 texture based on the element's extent on that
face's plane. For anything that isn't a full 0–16 axis-aligned slab, specify
`uv` explicitly; auto-UV on a rotated or offset element will not do what you
expect.

## `cullface` semantics — read this before adding any

`cullface` does **not** control GL back-face culling (that's fixed per
render-pass state, see `RenderType.cpp`). It controls a *different*,
additional check: whether this quad is skipped entirely when the neighboring
block on that side is opaque/solid (an optimization — don't draw a face
that's guaranteed hidden behind a neighbor).

- If a face's coordinate on that axis sits exactly on the block boundary (0
  or 16) **and no rotation is set on the element**, the engine auto-assigns
  `cullface` for you (`boundaryCullFace`) — you don't need to (and generally
  shouldn't) write it explicitly for ordinary full-face boundary quads.
- If a face is **interior** (not touching 0/16 on the relevant axis) or the
  element has a `rotation`, no cullface is auto-assigned, and none should be
  written by hand either — an interior/rotated face is not reliably hidden
  by a neighbor, and adding `cullface` there can make the face vanish
  incorrectly depending on placement (this was the exact bug fixed in
  `simple_lantern` — a previous edit added `cullface` to interior faces with
  no engine work backing it up, causing placement-dependent disappearance).
- `cullface` only takes effect for **blocks in world space** with a plain
  1:1 baked-model draw (`drawBakedModelQuads`, when not in inventory view and
  no per-coordinate scale/offset transform is active — `coordinate_bounds`
  disables it, see `ModModels.cpp` around line 1041). It never applies to
  items or inventory icons.

**Rule of thumb:** don't hand-write `cullface` at all unless you know the
face sits flush on a block boundary and the element has no rotation. Let
`boundaryCullFace` handle it automatically; it already does the right thing
for ordinary full-cube-aligned faces.

## Rotation

```json
"rotation": { "origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": false }
```

Two mutually exclusive forms, both accepted:

- **Vanilla single-axis**: `axis` is `"x"`, `"y"`, or `"z"`, `angle` is the
  rotation in degrees about `origin`. `rescale: true` additionally stretches
  the rotated geometry back out along the two perpendicular axes (vanilla's
  45°-diagonal "fit back to grid" behavior).
- **Blockbench free rotation**: omit `axis`; set any of `x`/`y`/`z` (degrees,
  applied in that order — x then y then z) about `origin`. `rescale` is
  parsed but only actually rescales in the single-axis path (it's read for
  the free-rotation branch too but has no effect there — don't rely on it).

`origin` defaults to `[8, 8, 8]` (block center) if omitted.

An element with rotation set never gets an auto-`cullface` — see above.

## Texture references

`texture` (on a face) and texture map values both resolve the same way,
recursively:

1. `#key` — look up `key` in this model's `textures` map (or the flattened
   parent's, after merge) and resolve *that* value, repeating until it's not
   a `#reference`. A cycle or missing key fails the face silently (dropped,
   not an error).
2. `ns:name` — resolves to `assets/<ns>/textures/<name>` (Blockbench/vanilla
   namespaced form).
3. Anything else that isn't already `assets/...` or `mods/...` is treated as
   relative to the *declaring* JSON file's directory (the base path of
   whichever file — child or parent — actually defined that texture key).
4. `.png` is appended automatically if the resolved path doesn't already end
   in `.png`.

## Parent chains

`parent` supports: sibling model names (`"base"`), explicit relative paths
(`"./shared/base"`), mod-root-relative paths (`"models/shared/base"`),
namespaced paths (`"mymod:base"`), and vanilla/Blockbench block paths
(`"block/cube_all"`). Only **one level** is flattened at bake time per the
merge function (`mergeParentModel`), but that function is applied while
walking the whole chain during load, so multi-level inheritance does resolve
— each level merges into the next as loading walks up. Self-referencing or
cyclic parent chains stop safely (whatever was resolved so far is kept; it
doesn't hang or crash).

Merge rule, applied per level: **child elements entirely replace parent
elements** if the child declares any (no per-element merging — it's all or
nothing); **texture keys merge**, with the child's value winning if both
define the same key.

## Degenerate (zero-thickness) elements

A "thin plane" element — `from`/`to` equal on one axis (e.g. a 1-unit-thick
pane, or an authored 0-thick plane) — is fully supported and is the intended
way to make crossed/billboard-style geometry (see `simple_lantern`). Both
faces on that axis are baked as ordinary quads with opposite winding; GL
culling naturally shows only the camera-facing one per angle. There is no
special-casing for this in the current baker — earlier code used to skip one
of the two faces here on a mistaken z-fighting concern; that skip was
removed because back-face culling makes the two quads mutually exclusive
per-view anyway, so they never contended for the same pixel in the first
place. If you ever see this kind of skip reappear, it's wrong — remove it.

## Render-layer interaction (why a model can look "double-sided" even without two faces)

Whether a single declared face is single- or double-visible ultimately also
depends on the **block's render layer**, which is a Lua `register_block`
concern, not part of the model JSON itself, but it interacts directly with
everything above:

- Solid/cutout layer (`translucent = false`, the effective default when
  `opaque = true`): GL `cull = true`. A single face is visible from one side
  only — you need the "declare both faces" technique above for see-through
  thin geometry.
- Translucent layer (`translucent = true`, the effective default when
  `opaque = false` and `translucent` isn't set explicitly —
  `LuaBlockBindings.cpp`: `translucent` defaults to `!opaque`): GL
  `cull = false`, no depth write. A single declared face is already visible
  from both sides — declaring the opposite face too would double-draw it
  (wasted fill, and can look wrong with blending/z-order on genuinely
  translucent textures).

If your block has `opaque = false` for lighting reasons (e.g. a lattice that
should let light pass through, via `BLOCKS_LIGHT_OPACITY`) but you still want
real single-sided culling with paired both-faces geometry, set
`translucent = false` explicitly in `register_block` to opt out of the
`!opaque` default — `opaque` and `translucent` are independent flags even
though one defaults off the other.
