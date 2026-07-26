-- ================================================================
-- MOD: stone_bricks
-- Standardized structure with separated config
-- ================================================================

local config = require("config")

-- Every block's shape comes from a JSON model. assert() the load: model.load
-- returns nil plus an error message on failure, and swallowing that leaves the
-- block with no model at all.
local model = assert(minecraft.model.load("models/stone_bricks.json"))

minecraft.register_block({
  id = 98,
  texture = "mods/stone_bricks/stone_bricks.png",
  hardness = 2.0,
  resistance = 10.0,
  translation_key = "stoneBrick",
  material = "stone",
  model = model,
})

minecraft.register_shaped_recipe({
  output_block_id = 98,
  output_count = 4,
  pattern = { "##", "##" },
  key = "#",
  item_id = 1,
})

minecraft.log("info", "stone_bricks registered from Lua")
