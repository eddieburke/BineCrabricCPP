local SCREEN_ID = "repair_table:repair"

local screen = minecraft.screen.slots({
  id = SCREEN_ID,
  title = "Repair Table",
  label = "Repair Table",
  slots = 3,
  panel_width = 176,
  panel_height = 72,
  background = "/gui/inventory.png",
  background_uv = { 0, 0, 176, 72 },
  on_tick = function(ctx)
    if not minecraft.stack.is_empty(ctx.get(3)) then
      return
    end
    local left = ctx.get(1)
    local right = ctx.get(2)
    if minecraft.stack.is_empty(left) or minecraft.stack.is_empty(right) then
      return
    end
    local repaired = minecraft.stack.combine_damage(minecraft.stack.copy(left), minecraft.stack.copy(right))
    if minecraft.stack.is_empty(repaired) then
      return
    end
    ctx.set(3, repaired)
    ctx.set(1, minecraft.stack.empty())
    ctx.set(2, minecraft.stack.empty())
  end,
})

return {
  open = screen.open,
}
