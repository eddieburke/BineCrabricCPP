local visible = false
local scroll = 0
local SLOT = 18
local COLS = 9
local ROWS = 8

local function panel_layout(event)
  local x = event.x or 0
  local y = event.y or 0
  local w = event.width or 180
  local h = event.height or 220
  if w > 180 then
    w = 180
  end
  if h > 220 then
    h = 220
  end
  return x, y, w, h
end

local function item_at(items, event, mx, my)
  local x, y = panel_layout(event)
  local grid_y = y + 14
  local col = math.floor((mx - x) / SLOT)
  local row = math.floor((my - grid_y) / SLOT)
  if col < 0 or col >= COLS or row < 0 or row >= ROWS then
    return nil
  end
  local index = scroll * COLS + row * COLS + col + 1
  return items[index]
end

local TOGGLE_KEY = minecraft.key_code("o")

minecraft.on(minecraft.events.key_press, {
  key = TOGGLE_KEY,
  pressed = true,
  priority = 100,
}, function(event)
  visible = not visible
  event.handled = true
end)

local REGION_FILTER = {
  screen_id = minecraft.screen.ids.inventory,
  region = minecraft.screen.regions.side_panel,
  priority = 100,
  when = function()
    return visible
  end,
}

minecraft.on(minecraft.events.screen_region, {
  screen_id = REGION_FILTER.screen_id,
  region = REGION_FILTER.region,
  phase_name = "mouse_scroll",
  scroll_delta = function(delta)
    return delta ~= 0
  end,
  priority = REGION_FILTER.priority,
  when = REGION_FILTER.when,
}, function(event)
  local items = minecraft.items.ids()
  local max_scroll = math.max(0, math.ceil(#items / COLS) - ROWS)
  if event.scroll_delta > 0 then
    scroll = math.max(0, scroll - 1)
  else
    scroll = math.min(max_scroll, scroll + 1)
  end
  event.handled = true
end)

minecraft.on(minecraft.events.screen_region, {
  screen_id = REGION_FILTER.screen_id,
  region = REGION_FILTER.region,
  phase_name = "mouse_click",
  button = 0,
  priority = REGION_FILTER.priority,
  when = REGION_FILTER.when,
}, function(event)
  local items = minecraft.items.ids()
  local item_id = item_at(items, event, event.mouse_x, event.mouse_y)
  if item_id ~= nil then
    minecraft.world.set_cursor(item_id, 64)
    event.handled = true
  end
end)

minecraft.on(minecraft.events.screen_region, {
  screen_id = REGION_FILTER.screen_id,
  region = REGION_FILTER.region,
  phase_name = "render",
  priority = REGION_FILTER.priority,
  when = REGION_FILTER.when,
}, function(event)
  local items = minecraft.items.ids()
  local x, y, w, h = panel_layout(event)
  event.width = w
  event.height = h
  minecraft.gui.fill_rect(x, y, w, h, 0xC0101010)
  minecraft.gui.draw_text(x + 4, y + 4, "Items (O)", 0xFFFFFF)

  for row = 0, ROWS - 1 do
    for col = 0, COLS - 1 do
      local index = scroll * COLS + row * COLS + col + 1
      local item_id = items[index]
      if item_id ~= nil then
        local sx = x + col * SLOT
        local sy = y + 14 + row * SLOT
        minecraft.gui.fill_rect(sx, sy, SLOT, SLOT, 0x80202020)
        minecraft.gui.draw_item(sx + 1, sy + 1, item_id, 1)
      end
    end
  end

end)
