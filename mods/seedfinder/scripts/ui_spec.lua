local ui_spec = {}
local rules = minecraft.require("scripts.rules")

local MARGIN = 20
local TITLE_Y = 13
local CONTENT_TOP = 34
local FOOTER_H = 32
local CARD_PAD = 8
local HEADER_H = 22
local ROW_H = 20
local ROW_GAP = 3
local ROWADV = ROW_H + ROW_GAP
local CARD_GAP = 8
local ARROW_W = 14
local ICON_W = 16
local CHIP_H = 16

local SPEC_BG = 0xFF0C0F16
local CARD_BG = 0xFF202B3A
local HEADER_BG = 0xFF263449
local BTN_BG = 0xFF34445A
local VALUE_BG = 0xFF10151E
local CHIP_ON = 0xFF2C5C8F
local CHIP_OFF = 0xFF2A3340
local DELETE_BG = 0xFF7E3030
local ADD_BG = 0xFF2C5C8F
local DONE_BG = 0xFF2E7D4F
local LABEL_FG = 0xFF9AA6B6
local VALUE_FG = 0xFFE7ECF3
local MUTED_FG = 0xFF7F8FA5
local HOVER = 0x22FFFFFF
local C_ACCENT = 0xFF53B1FD
local C_ACCENT_DK = 0xFF2C5C8F
local C_BORDER = 0xFF46566D
local C_TEXT = 0xFFE7ECF3
local C_MUTE = 0xFF7F8FA5

local function spec_visible(y, h)
  return (y + h > S.view_top) and (y < S.view_bottom)
end

local function spec_body_btn(x, y, w, h, label, bg, fg, center, fn)
  if not spec_visible(y, h) then
    return
  end
  if bg ~= nil and bg ~= 0 then
    minecraft.gui.fill_rect(x, y, w, h, bg)
    if fn ~= nil and minecraft.util.in_rect(S.mx, S.my, x, y, w, h) then
      minecraft.gui.fill_rect(x, y, w, h, HOVER)
    end
  end
  if label ~= nil and label ~= "" then
    local ty = y + math.floor((h - 8) / 2)
    if center then
      minecraft.gui.draw_centered_text(x, ty, w, label, fg)
    else
      minecraft.gui.draw_text(x, y, label, fg)
    end
  end
  if fn ~= nil then
    S.spec_boxes[#S.spec_boxes + 1] = { x = x, y = y, w = w, h = h, fn = fn }
  end
end

local function spec_body_label(x, y, label, fg)
  if (y + 10 > S.view_top) and (y < S.view_bottom) then
    minecraft.gui.draw_text(x, y, label, fg)
  end
end

local function spec_rect_culled(x, y, w, h, color)
  if spec_visible(y, h) then
    minecraft.gui.fill_rect(x, y, w, h, color)
  end
end

local function spec_border_culled(x, y, w, h, color)
  if spec_visible(y, h) then
    minecraft.gui.fill_rect(x, y, w, 1, color)
    minecraft.gui.fill_rect(x, y + h - 1, w, 1, color)
    minecraft.gui.fill_rect(x, y, 1, h, color)
    minecraft.gui.fill_rect(x + w - 1, y, 1, h, color)
  end
end

local function L_label(x, y, label, fg)
  if not S.dry then
    spec_body_label(x, y, label, fg)
  end
end

local function L_btn(x, y, w, h, label, bg, fg, center, fn)
  if not S.dry then
    spec_body_btn(x, y, w, h, label, bg, fg, center, fn)
  end
end

local function L_cycle(x, y, label, fg, on_left, on_right)
  local value_w = math.max(58, minecraft.gui.text_width(label) + 12)
  L_btn(x, y, ARROW_W, ROW_H - 2, "<", BTN_BG, VALUE_FG, true, on_left)
  L_btn(x + ARROW_W + 1, y, value_w, ROW_H - 2, label, VALUE_BG, fg, true, nil)
  L_btn(x + ARROW_W + 1 + value_w + 1, y, ARROW_W, ROW_H - 2, ">", BTN_BG, VALUE_FG, true, on_right)
  return ARROW_W + 1 + value_w + 1 + ARROW_W
end

local function L_stepper(x, y, label, on_minus, on_plus)
  local value_w = math.max(50, minecraft.gui.text_width(label) + 12)
  L_btn(x, y, ARROW_W, ROW_H - 2, "-", BTN_BG, VALUE_FG, true, on_minus)
  L_btn(x + ARROW_W + 1, y, value_w, ROW_H - 2, label, VALUE_BG, VALUE_FG, true, nil)
  L_btn(x + ARROW_W + 1 + value_w + 1, y, ARROW_W, ROW_H - 2, "+", BTN_BG, VALUE_FG, true, on_plus)
end

local function chip_section(node, inner_left, inner_right, by, single)
  local chip_x = inner_left
  for _, name in ipairs(S.all_biomes) do
    local label = pretty(name)
    local w = minecraft.gui.text_width(label) + 12
    if chip_x + w > inner_right then
      chip_x = inner_left
      by = by + CHIP_H + 4
    end
    local on
    if single then
      on = (#node.values == 1 and node.values[1] == name)
    else
      on = contains(node.values, name)
    end
    L_btn(chip_x, by, w, CHIP_H, label, on and CHIP_ON or CHIP_OFF, VALUE_FG, true, function()
      if single then
        node.values = { name }
      else
        toggle_value(node, name)
      end
    end)
    chip_x = chip_x + w + 4
  end
  return by + CHIP_H + 4
end

local function body_layout(node, inner_left, inner_right, value_col, by)
  if node.kind == "biome" then
    L_label(inner_left, by + 5, "Match", LABEL_FG)
    L_cycle(value_col, by, node.op == "none_of" and "none of" or "any of", VALUE_FG,
      function() node.op = (node.op == "none_of") and "any_of" or "none_of" end,
      function() node.op = (node.op == "none_of") and "any_of" or "none_of" end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Min cover %", LABEL_FG)
    L_stepper(value_col, by, rules.stepper_text(node.min_value),
      function() node.min_value = (node.min_value <= 0) and -1 or (node.min_value - 5) end,
      function()
        local v = (node.min_value < 0) and 0 or node.min_value
        v = v + 5
        if v > 100 then v = 100 end
        node.min_value = v
      end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Min radius", LABEL_FG)
    L_stepper(value_col, by, rules.stepper_text(node.value),
      function() node.value = (node.value <= 0) and -1 or (node.value - 1) end,
      function() node.value = ((node.value < 0) and 0 or node.value) + 1 end)
    L_label(value_col + 120, by + 5, "chunks (contiguous blob)", MUTED_FG)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Max cover %", LABEL_FG)
    L_stepper(value_col, by, rules.stepper_text(node.max_value),
      function() node.max_value = (node.max_value <= 0) and -1 or (node.max_value - 5) end,
      function()
        local v = (node.max_value < 0) and 0 or node.max_value
        v = v + 5
        if v > 100 then v = 100 end
        node.max_value = v
      end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Biomes", LABEL_FG)
    by = by + CARD_PAD + 6
    by = chip_section(node, inner_left, inner_right, by, false)
    return by
  elseif node.kind == "block" then
    local histogram = node.metric == "block_histogram"
    L_label(inner_left, by + 5, "Check", LABEL_FG)
    L_cycle(value_col, by, histogram and "block count in area" or "spawn surface block", VALUE_FG,
      function() node.metric = (node.metric == "block_histogram") and "spawn_surface_block_id" or "block_histogram" end,
      function() node.metric = (node.metric == "block_histogram") and "spawn_surface_block_id" or "block_histogram" end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Block", LABEL_FG)
    L_cycle(value_col, by, rules.block_name(node.block_id), VALUE_FG,
      function() node.block_id = rules.cycle_block(node.block_id, -1) end,
      function() node.block_id = rules.cycle_block(node.block_id, 1) end)
    by = by + ROWADV
    if histogram then
      L_label(inner_left, by + 5, "Min count", LABEL_FG)
      L_stepper(value_col, by, rules.stepper_text(node.min_value),
        function() node.min_value = (node.min_value <= 0) and -1 or (node.min_value - 1) end,
        function() node.min_value = ((node.min_value < 0) and 0 or node.min_value) + 1 end)
      by = by + ROWADV
      L_label(inner_left, by + 5, "Max count", LABEL_FG)
      L_stepper(value_col, by, rules.stepper_text(node.max_value),
        function() node.max_value = (node.max_value <= 0) and -1 or (node.max_value - 1) end,
        function() node.max_value = ((node.max_value < 0) and 0 or node.max_value) + 1 end)
      by = by + ROWADV
    else
      L_label(inner_left, by + 5, "Must be", LABEL_FG)
      L_cycle(value_col, by, node.op == "ne" and "not this block" or "this block", VALUE_FG,
        function() node.op = (node.op == "ne") and "eq" or "ne" end,
        function() node.op = (node.op == "ne") and "eq" or "ne" end)
      by = by + ROWADV
    end
    return by
  elseif node.kind == "block_on_block" then
    L_label(inner_left, by + 5, "Top block", LABEL_FG)
    L_cycle(value_col, by, rules.block_name(node.block_id), VALUE_FG,
      function() node.block_id = rules.cycle_block(node.block_id, -1) end,
      function() node.block_id = rules.cycle_block(node.block_id, 1) end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "On top of", LABEL_FG)
    L_cycle(value_col, by, rules.block_name(node.block_below_id), VALUE_FG,
      function() node.block_below_id = rules.cycle_block(node.block_below_id, -1) end,
      function() node.block_below_id = rules.cycle_block(node.block_below_id, 1) end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Min count", LABEL_FG)
    L_stepper(value_col, by, rules.stepper_text(node.min_value),
      function() node.min_value = (node.min_value <= 0) and -1 or (node.min_value - 1) end,
      function() node.min_value = ((node.min_value < 0) and 0 or node.min_value) + 1 end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Counts sampled surface columns", MUTED_FG)
    by = by + ROWADV
    return by
  else
    L_label(inner_left, by + 5, "Prefer", LABEL_FG)
    L_cycle(value_col, by, node.direction == "minimize" and "less" or "more", VALUE_FG,
      function() node.direction = (node.direction == "minimize") and "maximize" or "minimize" end,
      function() node.direction = (node.direction == "minimize") and "maximize" or "minimize" end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Measure", LABEL_FG)
    L_cycle(value_col, by, rules.metric_label(node.metric), VALUE_FG,
      function()
        node.metric = rules.cycle_metric(node.metric, -1)
        if not rules.needs_biome(node.metric) then
          node.values = {}
        elseif #node.values == 0 then
          node.values = { "desert" }
        end
      end,
      function()
        node.metric = rules.cycle_metric(node.metric, 1)
        if not rules.needs_biome(node.metric) then
          node.values = {}
        elseif #node.values == 0 then
          node.values = { "desert" }
        end
      end)
    by = by + ROWADV
    if rules.needs_biome(node.metric) then
      L_label(inner_left, by + 5, "Target biome", LABEL_FG)
      by = by + CARD_PAD + 6
      by = chip_section(node, inner_left, inner_right, by, true)
    end
    L_label(inner_left, by + 5, "Weight", LABEL_FG)
    local weight_text = tostring(math.floor((node.weight and node.weight > 0) and node.weight or 1))
    L_stepper(value_col, by, weight_text,
      function() node.weight = math.max(1, (node.weight or 1) - 1) end,
      function() node.weight = math.min(20, ((node.weight and node.weight > 0) and node.weight or 1) + 1) end)
    by = by + ROWADV
    return by
  end
end

function ui_spec.enter_spec()
  S.rules_backup = rules.clone_rules(S.rules)
  S.mode = "spec"
  S.spec_scroll = 0
  minecraft.screen.set_fields_visible(false)
end

function ui_spec.exit_spec(keep)
  if not keep then
    S.rules = rules.clone_rules(S.rules_backup or {})
  end
  S.rules_backup = nil
  S.mode = "finder"
  minecraft.screen.set_fields_visible(true)
end

local function footer_add(x, y, w, h, label, bg, fg, fn)
  S.spec_footer[#S.spec_footer + 1] = { x = x, y = y, w = w, h = h, text = label, bg = bg, fg = fg, fn = fn }
end

function ui_spec.spec_render(event)
  local W, H = event.width, event.height
  S.mx = event.mouse_x
  S.my = event.mouse_y
  S.spec_boxes = {}
  S.spec_footer = {}
  S.view_top = CONTENT_TOP
  S.view_bottom = H - FOOTER_H

  local viewport = math.max(0, (H - FOOTER_H) - CONTENT_TOP)
  local max_scroll = math.max(0, (S.spec_content_height or 0) - viewport)
  if S.spec_scroll > max_scroll then
    S.spec_scroll = max_scroll
  end
  if S.spec_scroll < 0 then
    S.spec_scroll = 0
  end

  minecraft.gui.fill_rect(0, 0, W, H, SPEC_BG)

  local left = MARGIN
  local card_w = W - 2 * MARGIN
  local inner_left = left + CARD_PAD
  local inner_right = left + card_w - CARD_PAD
  local value_col = inner_left + 96
  local top0 = CONTENT_TOP - S.spec_scroll
  local cy = top0

  for i, node in ipairs(S.rules) do
    local card_top = cy
    local body_start = card_top + HEADER_H + CARD_PAD
    S.dry = true
    local body_end = body_layout(node, inner_left, inner_right, value_col, body_start)
    S.dry = false
    local body_h = body_end - body_start
    local card_h = HEADER_H + body_h + CARD_PAD

    spec_rect_culled(left, card_top, card_w, card_h, CARD_BG)
    spec_rect_culled(left, card_top, card_w, HEADER_H, HEADER_BG)
    spec_rect_culled(left, card_top, 3, card_h, C_ACCENT_DK)
    spec_border_culled(left, card_top, card_w, card_h, C_BORDER)

    local hx = inner_left
    local hy = card_top + 3
    if i > 1 then
      L_btn(hx, hy, ICON_W, ICON_W, "^", BTN_BG, VALUE_FG, true, function()
        S.rules[i], S.rules[i - 1] = S.rules[i - 1], S.rules[i]
      end)
    end
    hx = hx + ICON_W + 2
    if i < #S.rules then
      L_btn(hx, hy, ICON_W, ICON_W, "v", BTN_BG, VALUE_FG, true, function()
        S.rules[i], S.rules[i + 1] = S.rules[i + 1], S.rules[i]
      end)
    end
    hx = hx + ICON_W + 6
    spec_body_label(hx, card_top + 7, "RULE " .. tostring(i), C_ACCENT)

    spec_body_btn(inner_right - ICON_W, hy, ICON_W, ICON_W, "X", DELETE_BG, VALUE_FG, true, function()
      table.remove(S.rules, i)
    end)
    local kind_text = rules.kind_label(node.kind)
    local cycle_w = ARROW_W + 1 + math.max(58, minecraft.gui.text_width(kind_text) + 12) + 1 + ARROW_W
    local cxk = inner_right - ICON_W - 6 - cycle_w
    L_cycle(cxk, card_top + 3, kind_text, VALUE_FG,
      function() S.rules[i] = rules.make_default(rules.cycle_kind(node.kind, -1)) end,
      function() S.rules[i] = rules.make_default(rules.cycle_kind(node.kind, 1)) end)

    body_layout(node, inner_left, inner_right, value_col, body_start)
    cy = card_top + card_h + CARD_GAP
  end

  spec_body_btn(left, cy, card_w, ROW_H + 4, "+ Add Rule", ADD_BG, VALUE_FG, true, function()
    S.rules[#S.rules + 1] = rules.make_default("biome")
  end)
  cy = cy + ROW_H + 4
  S.spec_content_height = cy - top0

  local footer_y = H - FOOTER_H + 6
  local btn_w = 110
  local gap = 10
  local total_w = btn_w * 2 + gap
  local fx = math.floor((W - total_w) / 2)
  footer_add(fx, footer_y, btn_w, ROW_H, "Cancel", BTN_BG, VALUE_FG, function() ui_spec.exit_spec(false) end)
  footer_add(fx + btn_w + gap, footer_y, btn_w, ROW_H, "Done", DONE_BG, VALUE_FG, function() ui_spec.exit_spec(true) end)
  footer_add(W - MARGIN + 1, CONTENT_TOP, ICON_W, ICON_W, "^", BTN_BG, VALUE_FG, function()
    S.spec_scroll = S.spec_scroll - 40
  end)
  footer_add(W - MARGIN + 1, H - FOOTER_H - ICON_W, ICON_W, ICON_W, "v", BTN_BG, VALUE_FG, function()
    S.spec_scroll = S.spec_scroll + 40
  end)

  minecraft.gui.fill_rect(0, 0, W, S.view_top, SPEC_BG)
  minecraft.gui.fill_rect(0, S.view_top - 1, W, 1, C_BORDER)
  minecraft.gui.draw_centered_text(0, TITLE_Y, W, "Seed Specification", C_TEXT)
  minecraft.gui.draw_centered_text(0, TITLE_Y + 11, W, "build rules, no typing", C_MUTE)
  if #S.rules == 0 then
    minecraft.gui.draw_centered_text(0, CONTENT_TOP + 24, W, "No rules yet. Press  + Add Rule  to start.", MUTED_FG)
  end
  minecraft.gui.fill_rect(0, S.view_bottom, W, H - S.view_bottom, SPEC_BG)
  minecraft.gui.fill_rect(0, S.view_bottom, W, 1, C_BORDER)
  for _, c in ipairs(S.spec_footer) do
    minecraft.gui.fill_rect(c.x, c.y, c.w, c.h, c.bg)
    if minecraft.util.in_rect(S.mx, S.my, c.x, c.y, c.w, c.h) then
      minecraft.gui.fill_rect(c.x, c.y, c.w, c.h, HOVER)
    end
    minecraft.gui.fill_rect(c.x, c.y, c.w, 1, C_BORDER)
    minecraft.gui.fill_rect(c.x, c.y + c.h - 1, c.w, 1, C_BORDER)
    minecraft.gui.fill_rect(c.x, c.y, 1, c.h, C_BORDER)
    minecraft.gui.fill_rect(c.x + c.w - 1, c.y, 1, c.h, C_BORDER)
    minecraft.gui.draw_centered_text(c.x, c.y + math.floor((c.h - 8) / 2), c.w, c.text, c.fg)
  end
end

function ui_spec.spec_mouse(event)
  if event.button ~= 0 then
    return
  end
  for i = #S.spec_footer, 1, -1 do
    local b = S.spec_footer[i]
    if minecraft.util.in_rect(event.x, event.y, b.x, b.y, b.w, b.h) then
      b.fn()
      event.handled = true
      return
    end
  end
  for i = #S.spec_boxes, 1, -1 do
    local b = S.spec_boxes[i]
    if minecraft.util.in_rect(event.x, event.y, b.x, b.y, b.w, b.h) then
      b.fn()
      event.handled = true
      return
    end
  end
end

return ui_spec
