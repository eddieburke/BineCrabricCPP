-- ================================================================
-- MOD: too_many_items
-- Standardized structure with separated config
-- ================================================================

local config = require("too_many_items.config")

local visible = false
local scroll = 0
local SLOT = 18
local MAX_COLS = 9
local MAX_ROWS = 8
local PADDING = 4
local GRID_Y_OFFSET = 16

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
  local cols = math.min(MAX_COLS, math.max(0, math.floor((w - PADDING * 2) / SLOT)))
  local rows = math.min(MAX_ROWS, math.max(0, math.floor((h - GRID_Y_OFFSET - PADDING) / SLOT)))
  return x, y, w, h, cols, rows
end

local function item_at(items, event, mx, my)
  local x, y, _, _, cols, rows = panel_layout(event)
  local grid_x = x + PADDING
  local grid_y = y + GRID_Y_OFFSET
  local col = math.floor((mx - grid_x) / SLOT)
  local row = math.floor((my - grid_y) / SLOT)
  if col < 0 or col >= cols or row < 0 or row >= rows then
    return nil
  end
  local index = scroll * cols + row * cols + col + 1
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
  local x, y, w, h, cols, rows = panel_layout(event)
  if cols == 0 or rows == 0 or event.mouse_x < x or event.mouse_x >= x + w or event.mouse_y < y or event.mouse_y >= y + h then
    return
  end
  local max_scroll = math.max(0, math.ceil(#items / cols) - rows)
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
  priority = REGION_FILTER.priority,
  when = REGION_FILTER.when,
}, function(event)
  if event.button ~= 0 and event.button ~= 1 then
    return
  end
  local items = minecraft.items.ids()
  local item_id = item_at(items, event, event.mouse_x, event.mouse_y)
  if item_id ~= nil then
    local count = 1
    if event.button == 0 then
      local info = minecraft.items.describe(item_id)
      count = info and info.max_count or 64
    end
    minecraft.inventory.give({id = item_id, count = count})
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
  local x, y, w, h, cols, rows = panel_layout(event)
  event.width = w
  event.height = h
  minecraft.gui.fill_rect(x, y, w, h, 0xC0101010)
  local title = "Items (O)"
  if minecraft.gui.text_width(title) + PADDING * 2 > w then
    title = "O"
  end
  minecraft.gui.draw_text(x + PADDING, y + PADDING, title, 0xFFFFFF)

  local max_scroll = cols > 0 and math.max(0, math.ceil(#items / cols) - rows) or 0
  scroll = math.min(scroll, max_scroll)
  for row = 0, rows - 1 do
    for col = 0, cols - 1 do
      local index = scroll * cols + row * cols + col + 1
      local item_id = items[index]
      if item_id ~= nil then
        local sx = x + PADDING + col * SLOT
        local sy = y + GRID_Y_OFFSET + row * SLOT
        minecraft.gui.fill_rect(sx, sy, SLOT, SLOT, 0x80202020)
        minecraft.gui.draw_item(sx + 1, sy + 1, item_id, 1)
      end
    end
  end

end)
