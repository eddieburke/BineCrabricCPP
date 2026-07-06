minecraft.register_block({
  id = 150,
  texture = "mods/repair_table/repair_table.png",
  hardness = 2.5,
  resistance = 10.0,
  translation_key = "repairTable",
  material = "wood",
  model = {
    type = "box_list",
    boxes = {
      {
        min = { 0.0, 0.0, 0.0 },
        max = { 1.0, 0.8125, 1.0 },
      },
      {
        min = { 0.0, 0.8125, 0.0 },
        max = { 1.0, 1.0, 1.0 },
      },
    },
  },
  behavior_priority = 100,
  on_use = function(event)
    if event.item_damageable and event.item_damage > 0 then
      event.item_damage = 0
      event.handled = true
    end
  end,
})

minecraft.register_shaped_recipe({
  output_block_id = 150,
  output_count = 1,
  pattern = { "###", "# #", "###" },
  key = "#",
  item_id = 5,
})
