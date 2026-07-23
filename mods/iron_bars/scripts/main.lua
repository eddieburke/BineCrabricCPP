-- ================================================================
-- MOD: iron_bars
-- Standardized structure with separated config
-- ================================================================

local config = require("iron_bars.config")

minecraft.log("info", "iron_bars loading")
local iron_bars_model = assert(minecraft.model.load("models/iron_bars.json"))

minecraft.register_block({
  id = 101,
  texture = "mods/iron_bars/iron_bars.png",
  hardness = 5.0,
  resistance = 10.0,
  translation_key = "fenceIron",
  material = "metal",
  collision_height = 1.5,
  opaque = false,
  full_cube = false,
  stack_on_same = true,
  model = iron_bars_model,
  item = {
    texture = "mods/iron_bars/iron_bars.png",
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
