local inventory_helper = minecraft.require("scripts.inventory_helper")
local SCREEN_ID = "repair_table:repair"

local screen = inventory_helper.slots({
  id = SCREEN_ID,
  title = "Repair Table",
  slots = 3,
  panel_width = 176,
  panel_height = 166,
  player_inventory = true,
  background = "mods/repair_table/repair_table_gui.png",
  positions = {
    { x = 44, y = 35 },
    { x = 62, y = 35 },
    { x = 120, y = 35 },
  },
  on_tick = function(ctx)
    if not inventory_helper.stack.is_empty(ctx.get(3)) then
      return
    end
    local left = ctx.get(1)
    local right = ctx.get(2)
    if inventory_helper.stack.is_empty(left) or inventory_helper.stack.is_empty(right) then
      return
    end
    local repaired = inventory_helper.stack.combine_damage(inventory_helper.stack.copy(left), inventory_helper.stack.copy(right))
    if inventory_helper.stack.is_empty(repaired) then
      return
    end
    ctx.set(3, repaired)
    ctx.set(1, inventory_helper.stack.empty())
    ctx.set(2, inventory_helper.stack.empty())
  end,
})

return {
  open = screen.open,
}
