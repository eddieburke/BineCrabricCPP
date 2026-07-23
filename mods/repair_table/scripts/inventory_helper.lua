local M = {}

M.stack = {}

function M.stack.empty()
  return { id = 0, count = 0, damage = 0 }
end

function M.stack.is_empty(stack)
  return stack == nil or (stack.id or 0) == 0 or (stack.count or 0) <= 0
end

function M.stack.id(stack)
  return stack and stack.id or 0
end

function M.stack.combine_damage(left, right)
  if M.stack.is_empty(left) or M.stack.is_empty(right) then
    return left, right
  end
  if M.stack.id(left) ~= M.stack.id(right) then
    return left, right
  end
  local info = minecraft.items.describe(M.stack.id(left))
  if info == nil or not info.damageable then
    return left, right
  end
  local max_damage = info.max_damage or 0
  if max_damage <= 0 or (left.count or 0) ~= 1 or (right.count or 0) ~= 1 then
    return left, right
  end
  local damage_left = left.damage or 0
  local damage_right = right.damage or 0
  if damage_left <= 0 and damage_right <= 0 then
    return left, right
  end
  local remaining = (max_damage - damage_left) + (max_damage - damage_right)
  remaining = math.min(remaining, max_damage)
  left = {
    id = M.stack.id(left),
    count = 1,
    damage = max_damage - remaining,
  }
  return left, M.stack.empty()
end

function M.stack.copy(stack)
  if M.stack.is_empty(stack) then
    return M.stack.empty()
  end
  return {
    id = M.stack.id(stack),
    count = stack.count,
    damage = stack.damage or 0,
  }
end

function M.stack.describe(stack)
  local item_id = M.stack.id(stack)
  if item_id == 0 then
    return nil
  end
  return minecraft.items.describe(item_id)
end

function M.stack.mergeable(left, right)
  if M.stack.is_empty(left) or M.stack.is_empty(right) then
    return false
  end
  if M.stack.id(left) ~= M.stack.id(right) then
    return false
  end
  local info = M.stack.describe(left)
  if info == nil then
    return false
  end
  if info.has_subtypes and (left.damage or 0) ~= (right.damage or 0) then
    return false
  end
  return info.stackable ~= false
end

function M.stack.max_count(stack)
  local info = M.stack.describe(stack)
  if info == nil then
    return 64
  end
  return info.max_count or 64
end

function M.stack.click(slot_stack, cursor, button)
  slot_stack = M.stack.copy(slot_stack)
  cursor = M.stack.copy(cursor)
  if M.stack.is_empty(slot_stack) then
    if not M.stack.is_empty(cursor) then
      local amount = button == 0 and cursor.count or 1
      amount = math.min(amount, M.stack.max_count(cursor))
      slot_stack = M.stack.copy(cursor)
      slot_stack.count = amount
      cursor.count = cursor.count - amount
      if cursor.count <= 0 then
        cursor = M.stack.empty()
      end
    end
    return slot_stack, cursor
  end
  if M.stack.is_empty(cursor) then
    local amount = button == 0 and slot_stack.count or math.floor((slot_stack.count + 1) / 2)
    cursor = M.stack.copy(slot_stack)
    cursor.count = amount
    slot_stack.count = slot_stack.count - amount
    if slot_stack.count <= 0 then
      slot_stack = M.stack.empty()
    end
    return slot_stack, cursor
  end
  if M.stack.mergeable(slot_stack, cursor) then
    local room = M.stack.max_count(slot_stack) - slot_stack.count
    local moved = math.min(room, button == 0 and cursor.count or 1)
    if moved > 0 then
      slot_stack.count = slot_stack.count + moved
      cursor.count = cursor.count - moved
      if cursor.count <= 0 then
        cursor = M.stack.empty()
      end
    end
    return slot_stack, cursor
  end
  if cursor.count <= M.stack.max_count(slot_stack) then
    local swapped = M.stack.copy(slot_stack)
    slot_stack = M.stack.copy(cursor)
    cursor = swapped
  end
  return slot_stack, cursor
end

function M.slots(spec)
  local gui = minecraft.gui
  local inv = minecraft.inventory
  local slot_count = spec.slots or #(spec.positions or {})
  assert(slot_count > 0, "screen.slots requires slots > 0")
  local panel_w = spec.panel_width or 176
  local panel_h = spec.panel_height or 72
  local slot_y = spec.slot_y or 24
  local gap = spec.gap or 26
  local priority = spec.priority or 100
  local positions = spec.positions
  if positions == nil then
    positions = {}
    local slot_size = 16
    local span = slot_count * slot_size + math.max(0, slot_count - 1) * gap
    local start_x = math.floor((panel_w - span) / 2)
    for index = 1, slot_count do
      positions[index] = {
        x = start_x + (index - 1) * (slot_size + gap),
        y = slot_y,
      }
    end
  end
  local player_slots = {}
  if spec.player_inventory then
    for row = 0, 2 do
      for col = 0, 8 do
        table.insert(player_slots, {
          player_slot = 9 + row * 9 + col,
          x = 8 + col * 18,
          y = panel_h - 82 + row * 18,
        })
      end
    end
    for col = 0, 8 do
      table.insert(player_slots, {
        player_slot = col,
        x = 8 + col * 18,
        y = panel_h - 24,
      })
    end
  end
  local state = {
    origin_x = 0,
    origin_y = 0,
    stacks = {},
  }
  for index = 1, slot_count do
    state.stacks[index] = M.stack.empty()
  end
  local function panel_origin(width, height)
    state.origin_x = math.floor((width - panel_w) / 2)
    state.origin_y = math.floor((height - panel_h) / 2)
  end
  local function slot_at(mouse_x, mouse_y)
    local x = mouse_x - state.origin_x
    local y = mouse_y - state.origin_y
    for index, pos in ipairs(positions) do
      if x >= pos.x - 1 and x < pos.x + 17 and y >= pos.y - 1 and y < pos.y + 17 then
        return index
      end
    end
    return nil
  end
  local function player_slot_at(mouse_x, mouse_y)
    if not spec.player_inventory then
      return nil
    end
    local x = mouse_x - state.origin_x
    local y = mouse_y - state.origin_y
    for index, ps in ipairs(player_slots) do
      if x >= ps.x - 1 and x < ps.x + 17 and y >= ps.y - 1 and y < ps.y + 17 then
        return ps.player_slot
      end
    end
    return nil
  end
  local ctx = {}
  function ctx.count()
    return slot_count
  end
  function ctx.get(index)
    return M.stack.copy(state.stacks[index] or M.stack.empty())
  end
  function ctx.set(index, stack)
    state.stacks[index] = M.stack.copy(stack)
    if spec.on_slot_change then
      spec.on_slot_change(ctx, index)
    end
  end
  function ctx.slots()
    local copies = {}
    for index = 1, slot_count do
      copies[index] = ctx.get(index)
    end
    return copies
  end
  local function return_items()
    for index = 1, slot_count do
      local stack = state.stacks[index]
      if not M.stack.is_empty(stack) then
        inv.give(stack)
        state.stacks[index] = M.stack.empty()
      end
    end
  end
  local function reset_slots()
    for index = 1, slot_count do
      state.stacks[index] = M.stack.empty()
    end
  end
  local function draw_stack(stack, x, y)
    if M.stack.is_empty(stack) then
      return
    end
    gui.draw_item(x, y, M.stack.id(stack), stack.count, stack.damage or 0)
  end
  local function render_panel(mouse_x, mouse_y)
    if spec.background then
      local uv = spec.background_uv or { 0, 0, panel_w, panel_h }
      gui.draw_sprite(spec.background, state.origin_x, state.origin_y, uv[1], uv[2], uv[3], uv[4])
    else
      gui.fill_rect(state.origin_x, state.origin_y, panel_w, panel_h, spec.panel_color or 0xC0101010)
    end
    for index, pos in ipairs(positions) do
      local sx = state.origin_x + pos.x
      local sy = state.origin_y + pos.y
      if not spec.background then
        gui.fill_rect(sx - 1, sy - 1, 18, 18, 0xFF373737)
        gui.fill_rect(sx, sy, 16, 16, 0xFF8B8B8B)
      end
      if minecraft.util.in_rect(mouse_x, mouse_y, sx, sy, 16, 16) then
        gui.fill_rect(sx, sy, 16, 16, 0x80FFFFFF)
      end
      draw_stack(state.stacks[index], sx, sy)
    end
    if spec.player_inventory then
      for _, ps in ipairs(player_slots) do
        local sx = state.origin_x + ps.x
        local sy = state.origin_y + ps.y
        if not spec.background then
          gui.fill_rect(sx - 1, sy - 1, 18, 18, 0xFF373737)
          gui.fill_rect(sx, sy, 16, 16, 0xFF8B8B8B)
        end
        if minecraft.util.in_rect(mouse_x, mouse_y, sx, sy, 16, 16) then
          gui.fill_rect(sx, sy, 16, 16, 0x80FFFFFF)
        end
        local stack = inv.get(ps.player_slot) or M.stack.empty()
        draw_stack(stack, sx, sy)
      end
    end
    local cursor = inv.cursor_get() or M.stack.empty()
    if not M.stack.is_empty(cursor) then
      draw_stack(cursor, mouse_x - 8, mouse_y - 8)
    end
  end
  local function handle_click(mouse_x, mouse_y, button)
    local index = slot_at(mouse_x, mouse_y)
    local cursor = inv.cursor_get() or M.stack.empty()
    if index then
      local slot_stack, next_cursor = M.stack.click(ctx.get(index), cursor, button)
      ctx.set(index, slot_stack)
      inv.cursor_set(next_cursor)
      if spec.on_slot_change then
        spec.on_slot_change(ctx, index)
      end
      return
    end
    local player_slot = player_slot_at(mouse_x, mouse_y)
    if player_slot then
      local current_stack = inv.get(player_slot) or M.stack.empty()
      local slot_stack, next_cursor = M.stack.click(current_stack, cursor, button)
      inv.set(player_slot, slot_stack)
      inv.cursor_set(next_cursor)
      return
    end
    local outside = mouse_x < state.origin_x or mouse_y < state.origin_y
      or mouse_x >= state.origin_x + panel_w or mouse_y >= state.origin_y + panel_h
    if outside and not M.stack.is_empty(cursor) then
      if button == 0 then
        inv.give(cursor)
        inv.cursor_set(M.stack.empty())
      else
        local single = M.stack.copy(cursor)
        single.count = 1
        inv.give(single)
        cursor.count = cursor.count - 1
        if cursor.count <= 0 then
          inv.cursor_set(M.stack.empty())
        else
          inv.cursor_set(cursor)
        end
      end
    end
  end
  local function open()
    reset_slots()
    inv.cursor_set(M.stack.empty())
    if spec.on_open then
      spec.on_open(ctx)
    end
    minecraft.screen.open(spec.id, { title = spec.title or spec.label or spec.id })
  end
  minecraft.on(minecraft.events.screen_event, { screen_id = spec.id, priority = priority }, function(event)
    if event.phase == "init" then
      panel_origin(event.width, event.height)
    elseif event.phase == "render" then
      panel_origin(event.width, event.height)
      render_panel(event.mouse_x, event.mouse_y)
    elseif event.phase == "tick" then
      if spec.on_tick then
        spec.on_tick(ctx)
      end
    elseif event.phase == "mouse" then
      if event.released then
        return
      end
      if event.button == 0 or event.button == 1 then
        handle_click(event.x, event.y, event.button)
        event.handled = true
      end
    elseif event.phase == "key" then
      if event.key == minecraft.keys.escape or event.key == minecraft.key_code("inventory") then
        minecraft.screen.close()
        event.handled = true
      end
    elseif event.phase == "close" then
      if spec.on_close then
        spec.on_close(ctx)
      end
      return_items()
      local cursor = inv.cursor_get() or M.stack.empty()
      if not M.stack.is_empty(cursor) then
        inv.give(cursor)
        inv.cursor_set(M.stack.empty())
      end
    end
  end)
  return {
    open = open,
    ctx = ctx,
  }
end

return M
