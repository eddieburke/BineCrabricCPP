minecraft.log("info", "iron_bars loading")

minecraft.register_block({
  id = 101,
  texture = "mods/iron_bars/iron_bars.png",
  hardness = 5.0,
  resistance = 10.0,
  translation_key = "fenceIron",
  material = "metal",
  model = {
    type = "connected_bars",
    core = {
      min = { 0.4375, 0.0, 0.4375 },
      max = { 0.5625, 1.0, 0.5625 },
    },
    north = {
      min = { 0.4375, 0.0, 0.0 },
      max = { 0.5625, 1.0, 0.4375 },
    },
    south = {
      min = { 0.4375, 0.0, 0.5625 },
      max = { 0.5625, 1.0, 1.0 },
    },
    west = {
      min = { 0.0, 0.0, 0.4375 },
      max = { 0.4375, 1.0, 0.5625 },
    },
    east = {
      min = { 0.5625, 0.0, 0.4375 },
      max = { 1.0, 1.0, 0.5625 },
    },
    connect = { "same", "opaque", "glass", "fence" },
    collision_height = 1.5,
    opaque = false,
    full_cube = false,
    stack_on_same = true,
  },
})

minecraft.register_shaped_recipe({
  output_block_id = 101,
  output_count = 16,
  pattern = { "###", "###" },
  key = "#",
  item_id = 265,
})

minecraft.log("info", "iron_bars registered from Lua")
