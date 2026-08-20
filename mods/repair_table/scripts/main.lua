local config = require("config")

local repair_screen = require("scripts.repair_screen")
local repair_table_model = assert(minecraft.model.load("models/repair_table/repair_table.json"))

minecraft.register_block({
  id = 150,
  hardness = 2.5,
  resistance = 10.0,
  translation_key = "repairTable",
  material = "wood",
  opaque = false,
  full_cube = false,
  model = repair_table_model,
  behavior_priority = 100,
  on_use = function(event)
    if not event.right_click then
      return
    end
    repair_screen.open()
    event.handled = true
  end,
})

minecraft.register_shaped_recipe({
  output_block_id = 150,
  output_count = 1,
  pattern = { "###", "# #", "###" },
  key = "#",
  item_id = 5,
})
