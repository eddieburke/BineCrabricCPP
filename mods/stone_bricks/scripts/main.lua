minecraft.register_block({
  id = 98,
  texture_id = 7,
  hardness = 2.0,
  resistance = 10.0,
  translation_key = "stoneBrick",
  material = "stone",
})

minecraft.register_shaped_recipe({
  output_block_id = 98,
  output_count = 4,
  pattern = { "##", "##" },
  key = "#",
  item_id = 1,
})

minecraft.log("info", "stone_bricks registered from Lua")
