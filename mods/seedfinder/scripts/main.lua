-- ================================================================
-- MOD: seedfinder
-- Standardized structure with separated config
-- ================================================================

local config = require("seedfinder.config")

local rules = minecraft.require("scripts.rules")
local search_mod = minecraft.require("scripts.search")
local ui_spec = minecraft.require("scripts.ui_spec")

-- Seedfinder: rules, cooperative seed search, and biome preview — all Lua on generic APIs.

local SCREEN_ID = "seedfinder:main"

-- LWJGL Beta 1.7.3 keyboard scancodes.
local KEY_ESCAPE = 1
local KEY_ENTER = 28
local KEY_UP = 200
local KEY_DOWN = 208

local DEFAULTS = {
  seed_start = "0",
  seed_end = "10000",
  radius = "4",
  top_k = "12",
  min_score = "0",
  unique_min = "",
}

local SPAWN_PICKER_BIOMES = {
  "plains", "forest", "desert", "taiga", "swampland", "rainforest", "savanna",
  "shrubland", "seasonal_forest", "ice_desert", "tundra", "hell", "sky",
}

S = {
  mode = "finder",
  world_name = "",
  seed_text = "",
  saved = {},
  spawn_biomes = {},
  all_biomes = {},
  spawn_index = 0,
  hits = {},
  seen = {},
  selected_seed = nil,
  checked = 0,
  searching = false,
  search_cur = 0,
  search_end = 0,
  search_cfg = nil,
  scroll = 0,
  preview_seed = nil,
  map = nil,
  map_tex = nil,
  rules = {},
  rules_backup = nil,
  spec_scroll = 0,
  spec_content_height = 0,
  spec_boxes = {},
  spec_footer = {},
  view_top = 0,
  view_bottom = 0,
  dry = false,
  hitboxes = {},
  mx = -1,
  my = -1,
  layout = nil,
}

for k, v in pairs(DEFAULTS) do
  S.saved[k] = v
end

S.spawn_biomes = SPAWN_PICKER_BIOMES
S.all_biomes = minecraft.registry.list("biome")

----------------------------------------------------------------------
-- theme
----------------------------------------------------------------------

local C_BACKDROP   = 0xFF101722
local C_PANEL      = 0xFF202B3A
local C_PANEL2     = 0xFF263449
local C_HEADER     = 0xFF182231
local C_BORDER     = 0xFF46566D
local C_BORDER_HI  = 0xFF7186A5
local C_ACCENT     = 0xFF53B1FD
local C_ACCENT_DK  = 0xFF2C5C8F
local C_TEXT       = 0xFFE7ECF3
local C_DIMTEXT    = 0xFF9AA6B6
local C_MUTE       = 0xFF7F8FA5
local C_GOOD       = 0xFF6FCF82
local C_DANGER     = 0xFFD46A6A
local C_BTN        = 0xFF34445A
local C_BTN_HOV    = 0xFF415570
local C_BTN_ON     = 0xFF2C5C8F
local C_BTN_ONH    = 0xFF3A74B0
local C_ROW        = 0xFF1A2431
local C_ROW_ALT    = 0xFF1E2A39
local C_ROW_SEL    = 0xFF234461
local C_TRACK      = 0xFF0E1219
local C_THUMB      = 0xFF3A4658
local C_BEVEL_HI   = 0x18FFFFFF
local C_BEVEL_LO   = 0x26000000
local C_HOVER      = 0x22FFFFFF

----------------------------------------------------------------------
-- generic helpers
----------------------------------------------------------------------

function fmt_seed(n)
  n = n or 0
  if math.type(n) == "integer" then
    return tostring(n)
  end
  return string.format("%.0f", n)
end

local function tw(s)
  return minecraft.gui.text_width(s)
end

local in_rect = minecraft.util.in_rect

local function rect(x, y, w, h, color)
  minecraft.gui.fill_rect(x, y, w, h, color)
end

local function border(x, y, w, h, color)
  rect(x, y, w, 1, color)
  rect(x, y + h - 1, w, 1, color)
  rect(x, y, 1, h, color)
  rect(x + w - 1, y, 1, h, color)
end

local function panel(x, y, w, h, fill, brd)
  rect(x, y, w, h, fill)
  border(x, y, w, h, brd or C_BORDER)
end

local function text(x, y, s, color)
  minecraft.gui.draw_text(x, y, s, color)
end

local function draw_text_centered(x, y, w, s, color)
  minecraft.gui.draw_centered_text(x, y, w, s, color)
end

local function text_right(x_right, y, s, color)
  minecraft.gui.draw_text(x_right - tw(s), y, s, color)
end

local function elide(s, max_w)
  if tw(s) <= max_w then
    return s
  end
  while #s > 1 and tw(s .. "..") > max_w do
    s = s:sub(1, #s - 1)
  end
  return s .. ".."
end

function pretty(name)
  return (tostring(name):gsub("_", " "))
end

function contains(list, name)
  for _, v in ipairs(list) do
    if v == name then
      return true
    end
  end
  return false
end

function toggle_value(node, name)
  for i, v in ipairs(node.values) do
    if v == name then
      table.remove(node.values, i)
      return
    end
  end
  node.values[#node.values + 1] = name
end

function map_texture_from_grid(grid)
  if grid == nil then
    return nil
  end
  local tex = minecraft.render.create_texture(grid.side, grid.side, grid.values)
  if tex == nil or tex.id == nil then
    return nil
  end
  local spawn_px, spawn_pz = minecraft.world.marker_px(grid, grid.center_x or 0, grid.center_z or 0)
  return {id = tex.id, side = grid.side, spawn_px = spawn_px, spawn_pz = spawn_pz}
end

----------------------------------------------------------------------
-- finder layout + drawing
----------------------------------------------------------------------

local function add_hit(x, y, w, h, fn)
  S.hitboxes[#S.hitboxes + 1] = { x = x, y = y, w = w, h = h, fn = fn }
end

-- Unified themed button. opts: { variant = "accent"|"danger"|"ghost",
-- active = bool, disabled = bool, align = "left" }.
local function ui_button(x, y, w, h, label, fn, opts)
  opts = opts or {}
  local hover = in_rect(S.mx, S.my, x, y, w, h)
  local base, hov, fg = C_BTN, C_BTN_HOV, C_TEXT
  if opts.variant == "accent" then
    base, hov = C_BTN_ON, C_BTN_ONH
  elseif opts.variant == "danger" then
    base, hov = 0xFF7E3030, 0xFF9A3A3A
  elseif opts.variant == "ghost" then
    base, hov = 0xFF1B212B, C_BTN_HOV
  end
  if opts.active then
    base, hov = C_BTN_ON, C_BTN_ONH
  end
  if opts.disabled then
    base, hov, fg = 0xFF1A1F27, 0xFF1A1F27, C_MUTE
    hover = false
  end
  rect(x, y, w, h, hover and hov or base)
  rect(x, y, w, 1, C_BEVEL_HI)
  rect(x, y + h - 1, w, 1, C_BEVEL_LO)
  border(x, y, w, h, (hover or opts.active) and C_BORDER_HI or C_BORDER)
  local ty = y + math.floor((h - 8) / 2)
  if opts.align == "left" then
    text(x + 6, ty, elide(label, w - 10), fg)
  else
    draw_text_centered(x, ty, w, elide(label, w - 8), fg)
  end
  if fn ~= nil and not opts.disabled then
    add_hit(x, y, w, h, fn)
  end
end

local function layout(W, H)
  local L = { W = W, H = H }
  L.pad = 6
  L.header_h = 26
  L.footer_h = 24
  L.content_y = L.header_h + 4
  L.content_bottom = H - L.footer_h - 4

  -- left configuration panel
  L.lpx = L.pad
  L.lpw = math.max(188, math.min(210, math.floor(W * 0.42)))
  L.lpy = L.content_y
  L.lph = L.content_bottom - L.content_y

  -- right results / map panel
  L.rpx = L.lpx + L.lpw + L.pad
  L.rpw = W - L.rpx - L.pad

  -- two-column field grid (label above narrow field)
  local inner_x = L.lpx + 8
  local inner_w = L.lpw - 16
  local col_gap = 8
  local col_w = math.floor((inner_w - col_gap) / 2)
  local col0 = inner_x
  local col1 = inner_x + col_w + col_gap

  L.fields = {
    { name = "seed_start", label = "Seed start", numeric = true, signed = true },
    { name = "seed_end", label = "Seed end", numeric = true, signed = true },
    { name = "radius", label = "Radius (ch)", numeric = true },
    { name = "top_k", label = "Keep top", numeric = true },
    { name = "min_score", label = "Min score", numeric = true, signed = true, decimal = true },
    { name = "unique_min", label = "Min biomes", numeric = true },
  }

  local body_top = L.lpy + 8
  local body_bottom = L.lpy + L.lph - 8
  local total = body_bottom - body_top
  local field_rows = 3
  local ctrl_rows = 2
  local search_h = 18
  local block = total - search_h - 16
  local field_stride = math.max(22, math.min(28, math.floor((block * 0.58) / field_rows)))
  local ctrl_stride = math.max(18, math.min(24, math.floor((block * 0.42) / ctrl_rows)))

  for i, f in ipairs(L.fields) do
    local r = math.floor((i - 1) / 2)
    local c = (i - 1) % 2
    f.x = (c == 0) and col0 or col1
    f.w = col_w
    f.h = 14
    f.label_y = body_top + r * field_stride
    f.y = f.label_y + 9
  end

  L.col0 = col0
  L.col1 = col1
  L.col_w = col_w
  L.inner_x = inner_x
  L.inner_w = inner_w
  L.field_stride = field_stride
  L.ctrl_stride = ctrl_stride

  local ctrl_top = body_top + field_rows * field_stride + 6
  L.ctrl_top = ctrl_top
  L.ctrl_h = 16
  L.search_y = ctrl_top + ctrl_rows * ctrl_stride + 6
  L.search_h = search_h

  L.footer_y = H - L.footer_h
  L.use_w = 92
  L.back_w = 70

  L.list_x = L.rpx + 6
  L.list_w = L.rpw - 12
  L.list_top = L.content_y + 18
  L.row_h = 12
  local map_size = math.min(L.rpw - 12, math.max(96, math.floor((L.content_bottom - L.content_y) * 0.42)))
  L.map_size = map_size
  L.map_panel_h = map_size + 16
  L.map_panel_y = L.content_bottom - L.map_panel_h
  L.list_bottom = L.map_panel_y - 6
  L.rows = math.max(1, math.floor((L.list_bottom - L.list_top) / L.row_h))
  return L
end

local function render_screen(event)
  local L = S.layout
  if L == nil then
    L = layout(event.width, event.height)
    S.layout = L
  end
  S.mx = event.mouse_x
  S.my = event.mouse_y
  S.hitboxes = {}

  -- backdrop + header
  rect(0, 0, L.W, L.H, C_BACKDROP)
  rect(0, 0, L.W, L.header_h, C_HEADER)
  rect(0, L.header_h - 1, L.W, 1, C_ACCENT_DK)
  rect(8, 8, 3, 10, C_ACCENT)
  text(16, 9, "SEED FINDER", C_TEXT)
  local hint = "Enter search   Up/Down select   Esc back"
  text_right(L.W - 8, 9, hint, C_MUTE)

  -- panels
  panel(L.lpx, L.lpy, L.lpw, L.lph, C_PANEL, C_BORDER)
  panel(L.rpx, L.content_y, L.rpw, L.content_bottom - L.content_y, C_PANEL, C_BORDER)

  -- config fields (native widgets draw themselves; we draw the labels above)
  for _, f in ipairs(L.fields) do
    text(f.x, f.label_y, f.label, C_DIMTEXT)
  end

  -- control buttons (2x2): spawn / preview / rules / import
  local ct = L.ctrl_top
  ui_button(L.col0, ct, L.col_w, L.ctrl_h, "Spawn: " .. search_mod.spawn_label(), function()
    local total = #S.spawn_biomes
    S.spawn_index = S.spawn_index + 1
    if S.spawn_index > total then
      S.spawn_index = 0
    end
  end, { align = "left" })
  ui_button(L.col1, ct, L.col_w, L.ctrl_h, "Preview start", search_mod.preview_seed, { align = "left" })
  local ct2 = ct + L.ctrl_stride
  ui_button(L.col0, ct2, L.col_w, L.ctrl_h, "Rules (" .. tostring(#S.rules) .. ")", ui_spec.enter_spec,
    { align = "left", active = (#S.rules > 0) })
  ui_button(L.col1, ct2, L.col_w, L.ctrl_h, "Import JSON", search_mod.do_import, { align = "left" })

  ui_button(L.inner_x, L.search_y, L.inner_w, L.search_h,
    S.searching and "Stop search" or "Search seeds", search_mod.toggle_search,
    { variant = S.searching and "danger" or "accent" })

  rect(0, L.footer_y, L.W, L.footer_h, C_HEADER)
  rect(0, L.footer_y, L.W, 1, C_BORDER)
  local fb_y = L.footer_y + math.floor((L.footer_h - 16) / 2)
  ui_button(L.pad, fb_y, L.back_w, 16, "Back", search_mod.go_back, { variant = "ghost" })
  ui_button(L.W - L.pad - L.use_w, fb_y, L.use_w, 16, "Use Seed", search_mod.use_selected,
    { variant = "accent", disabled = (S.selected_seed == nil) })
  local mid = S.searching and ("Searching... " .. fmt_seed(S.checked) .. " checked")
    or (#S.hits .. " result" .. (#S.hits == 1 and "" or "s") .. "  -  " .. fmt_seed(S.checked) .. " checked")
  draw_text_centered(L.back_w + L.pad * 2, fb_y, L.W - (L.back_w + L.use_w + L.pad * 4), mid,
    S.searching and C_ACCENT or C_DIMTEXT)

  text(L.rpx + 6, L.content_y + 5, "RESULTS", C_DIMTEXT)
  text_right(L.rpx + L.rpw - 6, L.content_y + 5, "score", C_MUTE)
  rect(L.rpx + 1, L.list_top - 2, L.rpw - 2, 1, C_BORDER)

  if S.scroll > math.max(0, #S.hits - L.rows) then
    S.scroll = math.max(0, #S.hits - L.rows)
  end
  if S.scroll < 0 then
    S.scroll = 0
  end
  rect(L.list_x - 2, L.list_top, L.list_w + 4, L.list_bottom - L.list_top, C_TRACK)
  for row = 1, L.rows do
    local i = S.scroll + row
    local h = S.hits[i]
    if h ~= nil then
      local y = L.list_top + (row - 1) * L.row_h
      local key = fmt_seed(h.seed)
      local sel = (key == S.selected_seed)
      local hover = in_rect(S.mx, S.my, L.list_x, y, L.list_w, L.row_h)
      local bg = (row % 2 == 0) and C_ROW_ALT or C_ROW
      if sel then
        bg = C_ROW_SEL
      elseif hover then
        bg = C_BTN_HOV
      end
      rect(L.list_x, y, L.list_w, L.row_h - 1, bg)
      if sel then
        rect(L.list_x, y, 2, L.row_h - 1, C_ACCENT)
      end
      local biome = pretty(minecraft.registry.name("biome", h.spawn_biome_id or 0))
      local left_text = elide(key .. "  " .. biome, L.list_w - 40)
      text(L.list_x + 5, y + 2, left_text, sel and C_TEXT or C_DIMTEXT)
      text_right(L.list_x + L.list_w - 5, y + 2, string.format("%.1f", h.score or 0),
        sel and C_ACCENT or C_MUTE)
      add_hit(L.list_x, y, L.list_w, L.row_h, function()
        search_mod.select_index(i)
      end)
    end
  end

  if #S.hits > L.rows then
    local track_h = L.list_bottom - L.list_top
    local bar_x = L.list_x + L.list_w - 3
    rect(bar_x, L.list_top, 3, track_h, C_TRACK)
    local thumb = math.max(12, math.floor(track_h * L.rows / #S.hits))
    local maxs = math.max(1, #S.hits - L.rows)
    local pos = math.floor((track_h - thumb) * S.scroll / maxs)
    rect(bar_x, L.list_top + pos, 3, thumb, C_THUMB)
  end

  panel(L.rpx, L.map_panel_y, L.rpw, L.map_panel_h, C_PANEL2, C_BORDER)
  text(L.rpx + 6, L.map_panel_y + 4, "BIOME MAP", C_DIMTEXT)
  local si = search_mod.selected_index()
  if si ~= nil then
    local h = S.hits[si]
    text_right(L.rpx + L.rpw - 6, L.map_panel_y + 4,
      string.format("%d, %d", h.spawn_x or 0, h.spawn_z or 0), C_MUTE)
  end
  local img = L.map_size
  local img_x = L.rpx + math.floor((L.rpw - img) / 2)
  local img_y = L.map_panel_y + 14
  rect(img_x - 1, img_y - 1, img + 2, img + 2, C_BORDER)
  local drew = false
  if S.map_tex ~= nil and S.map_tex.id ~= nil and S.map_tex.id > 0 then
    minecraft.gui.draw_texture(S.map_tex.id, img_x, img_y, img, img)
    local side = S.map_tex.side or 1
    if side < 1 then side = 1 end
    local mxp = img_x + math.floor((S.map_tex.spawn_px or 0) / side * img)
    local myp = img_y + math.floor((S.map_tex.spawn_pz or 0) / side * img)
    rect(mxp - 3, myp - 1, 7, 3, 0xFFFF5050)
    rect(mxp - 1, myp - 3, 3, 7, 0xFFFF5050)
    rect(mxp, myp, 1, 1, 0xFFFFFFFF)
    drew = true
  elseif S.map ~= nil and S.map.values ~= nil and S.map.side ~= nil and S.map.side > 0 then
    local side = S.map.side
    local cell = math.max(1, math.floor(img / side))
    local mw = cell * side
    local ox = img_x + math.floor((img - mw) / 2)
    local oy = img_y + math.floor((img - mw) / 2)
    rect(img_x, img_y, img, img, 0xFF0E1219)
    for r = 0, side - 1 do
      for c = 0, side - 1 do
        local color = S.map.values[r * side + c + 1]
        if color ~= nil then
          rect(ox + c * cell, oy + r * cell, cell, cell, color)
        end
      end
    end
    local cx = ox + math.floor(mw / 2)
    local cyy = oy + math.floor(mw / 2)
    rect(cx - 1, cyy - 1, 3, 3, 0xFFFF5050)
    drew = true
  end
  if not drew then
    rect(img_x, img_y, img, img, 0xFF0E1219)
    draw_text_centered(img_x, img_y + math.floor(img / 2) - 4, img, "select a result", C_MUTE)
  end
end

----------------------------------------------------------------------
-- events
----------------------------------------------------------------------

minecraft.on(minecraft.events.screen_ui, {
  screen_id = minecraft.screen.ids.create_world,
  region = minecraft.screen.regions.footer,
  priority = 90,
}, function(event)
  if event.ui == nil then
    return event
  end
  local host = event.host_fields or {}
  S.world_name = host.world_name or minecraft.screen.host_field("world_name")
  S.seed_text = host.seed or S.seed_text or ""
  event.ui:add_stacked_centered_button("Seed tools...", function()
    S.world_name = minecraft.screen.host_field("world_name")
    S.seed_text = minecraft.screen.host_field("seed")
    search_mod.open_finder()
  end)
  return event
end)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID }, function(event)
  if event.phase == "init" then
    local L = layout(event.width, event.height)
    S.layout = L
    for _, f in ipairs(L.fields) do
      minecraft.screen.add_field(f.name, f.x, f.y, f.w, f.h, {
        text = S.saved[f.name] or "",
        numeric = f.numeric or false,
        signed = f.signed or false,
        decimal = f.decimal or false,
        max_len = 20,
      })
    end
    if S.mode == "spec" then
      minecraft.screen.set_fields_visible(false)
    end
    return event
  elseif event.phase == "render" then
    if S.mode == "spec" then
      ui_spec.spec_render(event)
    else
      render_screen(event)
    end
    return event
  elseif event.phase == "tick" then
    search_mod.search_tick()
    return event
  elseif event.phase == "mouse" then
    if S.mode == "spec" then
      ui_spec.spec_mouse(event)
      return event
    end
    if event.button ~= 0 then
      return event
    end
    for i = #S.hitboxes, 1, -1 do
      local box = S.hitboxes[i]
      if in_rect(event.x, event.y, box.x, box.y, box.w, box.h) then
        box.fn()
        event.handled = true
        return event
      end
    end
    return event
  elseif event.phase == "scroll" then
    if S.mode == "spec" then
      S.spec_scroll = S.spec_scroll - ((event.delta or 0) > 0 and 24 or -24)
      event.handled = true
    else
      local step = (event.delta or 0) > 0 and -1 or 1
      S.scroll = S.scroll + step
      if S.scroll < 0 then
        S.scroll = 0
      end
      event.handled = true
    end
    return event
  elseif event.phase == "key" then
    if S.mode == "spec" then
      if event.key == KEY_ESCAPE then
        ui_spec.exit_spec(false)
        event.handled = true
      end
      return event
    end
    if event.key == KEY_ESCAPE then
      search_mod.go_back()
      event.handled = true
    elseif event.key == KEY_ENTER then
      search_mod.toggle_search()
      event.handled = true
    elseif event.key == KEY_UP then
      search_mod.move_selection(-1)
      event.handled = true
    elseif event.key == KEY_DOWN then
      search_mod.move_selection(1)
      event.handled = true
    end
    return event
  elseif event.phase == "close" then
    S.searching = false
    S.search_cfg = nil
    search_mod.free_map()
    return event
  end
end)
