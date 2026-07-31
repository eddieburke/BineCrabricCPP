# 03 - Registries & Content Definition API

BineCrabricCPP allows Lua mods to register new blocks, items, crafting recipes, and custom entity types into the native C++ registries during the `init` phase.

---

## 1. Block Registration API

### `minecraft.register_block(spec)`

Registers a custom block into the native `Block` registry.

- **Signature**: `minecraft.register_block(spec: table) -> boolean`
- **Spec Fields**:

| Field | Type | Description | Default |
| :--- | :--- | :--- | :--- |
| `id` | `integer` | Unique numerical block ID (1..255). | *Required* |
| `name` | `string` | Human-readable display name (added to `I18n` translation table). | `"Block <id>"` |
| `translation_key` | `string` | Custom translation key token. | `"block<id>"` |
| `hardness` | `number` | Mining duration factor. | `1.5` |
| `resistance` | `number` | Explosion resistance factor. | `10.0` |
| `luminance` | `integer` | Light emission level (0..15). | `0` |
| `material` | `string` | Block material type (`"stone"`, `"metal"`, `"wood"`, `"glass"`). | `"stone"` |
| `texture` | `string` \| `integer` | Mod texture path (e.g. `"textures/blocks/lantern.png"`) or terrain atlas index. | None |
| `opaque` | `boolean` | Whether the block fully occludes adjacent faces. | `true` |
| `render_type` | `string` \| `integer` | Geometry render type (`"cube"`, `"cross"`, `"torch"`, `"custom"`). | `"cube"` |
| `bounds` | `table` | Bounding box `{ minX, minY, minZ, maxX, maxY, maxZ }` (values 0.0..1.0). | Unit Cube |
| `drop_item` | `integer` | Item ID dropped when broken. | `id` |
| `drop_count` | `integer` | Number of items dropped when broken. | `1` |
| `on_use` | `function` | Interaction callback `function(event)`. Automatically subscribes to `block_interact`. | `nil` |
| `behavior_priority` | `integer` | Interaction handler priority. | `0` |

#### Example: Registering a Luminant Block

```lua
minecraft.register_block({
  id = 155,
  name = "Simple Lantern",
  hardness = 0.5,
  luminance = 15,
  material = "glass",
  opaque = false,
  texture = "textures/blocks/lantern.png",
  on_use = function(event)
    minecraft.log("info", "Player right-clicked lantern at (" .. event.x .. "," .. event.y .. "," .. event.z .. ")")
  end
})
```

---

## 2. Item Registration API

### `minecraft.register_item(spec)`

Registers a custom item into the native `Item` registry.

- **Signature**: `minecraft.register_item(spec: table) -> boolean`
- **Spec Fields**:

| Field | Type | Description | Default |
| :--- | :--- | :--- | :--- |
| `id` | `integer` | Unique numerical item ID (256+). | *Required* |
| `name` | `string` | Display name for inventory tooltips. | `"Item <id>"` |
| `texture` | `string` \| `integer` | Path to item texture image or item atlas ID. | None |
| `max_stack` | `integer` | Maximum stack size per slot. | `64` |
| `max_damage` | `integer` | Maximum durability for damageable items. | `0` (indestructible) |

#### Example: Registering a Tool Item

```lua
minecraft.register_item({
  id = 500,
  name = "Obsidian Dagger",
  texture = "textures/items/obsidian_dagger.png",
  max_stack = 1,
  max_damage = 250
})
```

---

## 3. Crafting Recipe Registration API

### `minecraft.register_shaped_recipe(spec)`

Registers a 2x2 or 3x3 shaped crafting recipe into the native `RecipeRegistry`.

- **Signature**: `minecraft.register_shaped_recipe(spec: table) -> boolean`
- **Spec Fields**:
  - `pattern`: Array of 2 or 3 strings representing crafting grid rows (e.g. `{ "X", "X" }` or `{ "###", " # ", " # " }`).
  - `key`: Map of character tokens to ingredient item/block IDs.
  - `result`: Output spec `{ id = item_id, count = count }` or result item ID number.

#### Example: 2x2 Crafting Recipe

```lua
minecraft.register_shaped_recipe({
  pattern = {
    "XX",
    "XX"
  },
  key = {
    X = 4 -- Cobblestone ID
  },
  result = {
    id = 155, -- Custom Lantern Block ID
    count = 1
  }
})
```

---

## 4. Custom Lua Mod Entities (`LuaModEntity`)

Mods can define custom entities with physics bodies, custom tick logic, and Lua state persistence.

### Entity Registration & Callbacks

Entities instantiated via `minecraft.world.spawn_entity` with a custom Lua type delegate their tick and collision loops to registered Lua callbacks:

- **`on_spawn(entity)`**: Called when the entity enters the world.
- **`on_tick(entity, delta)`**: Called on every server/world tick.
- **`on_interact(entity, player)`**: Called when a player right-clicks the entity.
- **`on_remove(entity)`**: Called when the entity is removed or killed.
