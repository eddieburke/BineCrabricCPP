local config = require("config")
local search = require("scripts.search")

local SCREEN_ID = "seedfinder:main"

local KEY_ESCAPE = 1
local KEY_ENTER = 28
local KEY_UP = 200
local KEY_DOWN = 208

local DEFAULTS = {
  seed_start = "0",
  seed_end = "100000",
  samples = "1000000",
  keep = "24",
  radius = "128",
  step = "16",
  min_unique = "0",
  stop_after = "0",
  spawn_max = "",
}

S = {
  screen_id = SCREEN_ID,
  world_name = "",
  seed_text = "",
  saved = {},
  page = "settings",
  list = "origin",
  lists = { origin = {}, require = {}, forbid = {}, prefer = {}, spawn = {} },
  random = false,
  area_on = true,
  spawn_on = false,
  ground = "any",
  hits = {},
  selected_seed = nil,
  detail = nil,
  detail_pending = false,
  job = nil,
  detail_job = nil,
  running = false,
  progress = nil,
  error = nil,
  scroll = 0,
  map_tex = nil,
  map_radius = 512,
  hitboxes = {},
  mx = -1,
  my = -1,
  layout = nil,
  tick_count = 0,
  toast = nil,
  toast_until = 0,
}

for k, v in pairs(DEFAULTS) do
  S.saved[k] = v
end

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
local C_BEVEL_HI   = 0x18FFFFFF
local C_BEVEL_LO   = 0x26000000

local in_rect = minecraft.util.in_rect

local function tw(s)
  return minecraft.gui.text_width(s)
end

local function rect(x, y, w, h, color)
  minecraft.gui.fill_rect(x, y, w, h, color)
end

local function border(x, y, w, h, color)
  rect(x, y, w, 1, color)
  rect(x, y + h - 1, w, 1, color)
  rect(x, y, 1, h, color)
  rect(x + w - 1, y, 1, h, color)
end

local function text(x, y, s, color)
  minecraft.gui.draw_text(x, y, s, color)
end

local function text_right(x_right, y, s, color)
  minecraft.gui.draw_text(x_right - tw(s), y, s, color)
end

local function text_centered(x, y, w, s, color)
  minecraft.gui.draw_centered_text(x, y, w, s, color)
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

local function add_hit(x, y, w, h, fn)
  S.hitboxes[#S.hitboxes + 1] = { x = x, y = y, w = w, h = h, fn = fn }
end

local function button(x, y, w, h, label, fn, opts)
  opts = opts or {}
  local hover = in_rect(S.mx, S.my, x, y, w, h)
  local base, hov, fg = C_BTN, C_BTN_HOV, C_TEXT
  if opts.variant == "accent" or opts.active then
    base, hov = C_BTN_ON, C_BTN_ONH
  elseif opts.variant == "danger" then
    base, hov = 0xFF7E3030, 0xFF9A3A3A
  elseif opts.variant == "ghost" then
    base, hov = 0xFF1B212B, C_BTN_HOV
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
    text(x + 5, ty, elide(label, w - 8), fg)
  else
    text_centered(x, ty, w, elide(label, w - 6), fg)
  end
  if fn ~= nil and not opts.disabled then
    add_hit(x, y, w, h, fn)
  end
end

local function set_page(page)
  S.page = page
  minecraft.screen.set_fields_visible(page == "settings")
end

local function layout(W, H)
  local L = { W = W, H = H, pad = 6, header_h = 22, footer_h = 24 }
  L.content_y = L.header_h + 4
  L.content_bottom = H - L.footer_h - 4
  L.lpx = L.pad
  L.lpw = math.max(196, math.min(232, math.floor(W * 0.44)))
  L.lpy = L.content_y
  L.lph = L.content_bottom - L.content_y
  L.rpx = L.lpx + L.lpw + L.pad
  L.rpw = W - L.rpx - L.pad

  L.inner_x = L.lpx + 8
  L.inner_w = L.lpw - 16
  L.tab_y = L.lpy + 6
  L.tab_h = 14
  L.page_top = L.tab_y + L.tab_h + 8
  L.search_h = 18
  L.search_y = L.lpy + L.lph - 6 - L.search_h
  L.page_bottom = L.search_y - 6

  local col_gap = 8
  L.col_w = math.floor((L.inner_w - col_gap) / 2)
  L.col0 = L.inner_x
  L.col1 = L.inner_x + L.col_w + col_gap

  L.fields = {
    { name = "seed_start", label = "Seed start", numeric = true, signed = true },
    { name = "seed_end", label = "Seed end", numeric = true, signed = true },
    { name = "samples", label = "Random samples", numeric = true },
    { name = "keep", label = "Keep top", numeric = true },
    { name = "radius", label = "Area radius", numeric = true },
    { name = "step", label = "Area step", numeric = true },
    { name = "min_unique", label = "Min biomes", numeric = true },
    { name = "stop_after", label = "Stop after hits", numeric = true },
    { name = "spawn_max", label = "Spawn max dist", numeric = true },
  }
  local rows = 5
  local stride = math.max(20, math.min(26, math.floor((L.page_bottom - L.page_top) / rows)))
  for i, f in ipairs(L.fields) do
    local r = math.floor((i - 1) / 2)
    local c = (i - 1) % 2
    f.x = (c == 0) and L.col0 or L.col1
    f.w = L.col_w
    f.h = 14
    f.label_y = L.page_top + r * stride
    f.y = f.label_y + 9
  end

  L.list_x = L.rpx + 6
  L.list_w = L.rpw - 12
  L.list_top = L.content_y + 18
  L.row_h = 12
  local map_size = math.min(L.rpw - 12, math.max(96, math.floor((L.content_bottom - L.content_y) * 0.46)))
  L.map_size = map_size
  L.map_panel_h = map_size + 28
  L.map_panel_y = L.content_bottom - L.map_panel_h
  L.list_bottom = L.map_panel_y - 6
  L.rows = math.max(1, math.floor((L.list_bottom - L.list_top) / L.row_h))
  L.footer_y = H - L.footer_h
  return L
end

local function draw_tabs(L)
  local tabs = { { "settings", "Settings" }, { "biomes", "Biomes" }, { "rules", "Rules" } }
  local gap = 3
  local w = math.floor((L.inner_w - gap * 2) / 3)
  for i, t in ipairs(tabs) do
    local x = L.inner_x + (i - 1) * (w + gap)
    button(x, L.tab_y, w, L.tab_h, t[2], function() set_page(t[1]) end, { active = (S.page == t[1]) })
  end
end

local function count_selected()
  local n = 0
  for _, which in ipairs(search.LISTS) do
    for _ in pairs(S.lists[which]) do
      n = n + 1
    end
  end
  return n
end

local function draw_settings_page(L)
  for _, f in ipairs(L.fields) do
    text(f.x, f.label_y, f.label, C_DIMTEXT)
  end
end

local function draw_biomes_page(L)
  local gap = 2
  local w = math.floor((L.inner_w - gap * 4) / 5)
  local y = L.page_top
  for i, which in ipairs(search.LISTS) do
    local x = L.inner_x + (i - 1) * (w + gap)
    local n = 0
    for _ in pairs(S.lists[which]) do
      n = n + 1
    end
    local label = search.LIST_LABELS[which] .. (n > 0 and (" " .. n) or "")
    button(x, y, w, 14, label, function() S.list = which end, { active = (S.list == which) })
  end
  y = y + 18
  text(L.inner_x, y, elide(search.LIST_HELP[S.list], L.inner_w), C_MUTE)
  y = y + 12
  local avail = L.page_bottom - y
  local rows = math.ceil(#search.BIOMES / 2)
  local rh = math.max(11, math.min(15, math.floor(avail / rows)))
  for i, name in ipairs(search.BIOMES) do
    local r = math.floor((i - 1) / 2)
    local c = (i - 1) % 2
    local x = (c == 0) and L.col0 or L.col1
    local on = S.lists[S.list][name] == true
    button(x, y + r * rh, L.col_w, rh - 1, (on and "+ " or "  ") .. pretty(name), function()
      S.lists[S.list][name] = (not on) or nil
    end, { active = on, align = "left" })
  end
end

local function draw_rules_page(L)
  local y = L.page_top
  local h = 16
  local stride = h + 4
  local function row(label, fn, opts)
    button(L.inner_x, y, L.inner_w, h, label, fn, opts)
    y = y + stride
  end
  row("Seeds: " .. (S.random and "random sample" or "sequential range"), function() S.random = not S.random end,
    { align = "left", active = S.random })
  row("Area scan: " .. (S.area_on and "on" or "off"), function() S.area_on = not S.area_on end,
    { align = "left", active = S.area_on })
  row("Exact spawn: " .. (S.spawn_on and "on (slow)" or "off"), function() S.spawn_on = not S.spawn_on end,
    { align = "left", active = S.spawn_on })
  row("Ground at 0,0: " .. S.ground, function()
    for i, g in ipairs(search.GROUND) do
      if g == S.ground then
        S.ground = search.GROUND[i % #search.GROUND + 1]
        break
      end
    end
  end, { align = "left", active = S.ground ~= "any" })
  row("Clear biome lists", search.clear_lists, { align = "left", variant = "ghost" })
  row("Preview start seed", search.preview_first, { align = "left", variant = "ghost" })
  y = y + 2
  text(L.inner_x, y, "Cheap rules run first; Exact spawn and", C_MUTE)
  text(L.inner_x, y + 10, "ground use real terrain generation.", C_MUTE)
end

local function status_line()
  if S.error ~= nil then
    return S.error, C_DANGER
  end
  if S.toast ~= nil and S.tick_count < S.toast_until then
    return S.toast, C_GOOD
  end
  local p = S.progress
  if p == nil then
    return "Ready - " .. count_selected() .. " biome filters", C_DIMTEXT
  end
  local pct = p.total > 0 and math.floor(100 * p.checked / p.total) or 0
  local body = fmt_count(p.checked) .. " / " .. fmt_count(p.total) .. " (" .. pct .. "%)  "
    .. fmt_count(p.rate) .. "/s  " .. fmt_count(p.matches) .. " hits"
  if p.state == "running" then
    return "Searching " .. body, C_ACCENT
  elseif p.state == "cancelled" then
    return "Stopped " .. body, C_DIMTEXT
  end
  return "Done " .. body, C_GOOD
end

local function draw_map(L)
  local x0 = L.rpx
  local y0 = L.map_panel_y
  rect(x0, y0, L.rpw, L.map_panel_h, C_PANEL2)
  border(x0, y0, L.rpw, L.map_panel_h, C_BORDER)
  text(x0 + 6, y0 + 4, "BIOME MAP", C_DIMTEXT)
  local h = search.selected_hit()
  if h ~= nil then
    local info
    if S.detail ~= nil and S.detail.spawn_x ~= nil then
      info = "spawn " .. S.detail.spawn_x .. ", " .. S.detail.spawn_z
    elseif S.detail_pending then
      info = "finding spawn..."
    else
      info = "no sand spawn"
    end
    text_right(x0 + L.rpw - 6, y0 + 4, info, C_MUTE)
  end
  local img = L.map_size
  local img_x = x0 + math.floor((L.rpw - img) / 2)
  local img_y = y0 + 14
  rect(img_x - 1, img_y - 1, img + 2, img + 2, C_BORDER)
  local tex = S.map_tex
  if tex ~= nil then
    minecraft.gui.draw_texture(tex.id, img_x, img_y, img, img)
    local function mark(wx, wz, color)
      local px = img_x + math.floor((wx - tex.origin_x) / tex.span * img)
      local pz = img_y + math.floor((wz - tex.origin_z) / tex.span * img)
      if px >= img_x and px < img_x + img and pz >= img_y and pz < img_y + img then
        rect(px - 3, pz - 1, 7, 3, color)
        rect(px - 1, pz - 3, 3, 7, color)
        rect(px, pz, 1, 1, 0xFFFFFFFF)
      end
    end
    mark(0, 0, 0xFF50A0FF)
    if S.detail ~= nil and S.detail.spawn_x ~= nil then
      mark(S.detail.spawn_x, S.detail.spawn_z, 0xFFFF5050)
    end
  else
    rect(img_x, img_y, img, img, C_TRACK)
    text_centered(img_x, img_y + math.floor(img / 2) - 4, img, "select a result", C_MUTE)
  end
  if h ~= nil then
    local line = elide(fmt_seed(h.seed) .. "  " .. pretty(h.origin_biome or "?") .. " at 0,0", L.rpw - 12)
    text(x0 + 6, img_y + img + 4, line, C_DIMTEXT)
  end
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

  rect(0, 0, L.W, L.H, C_BACKDROP)
  rect(0, 0, L.W, L.header_h, C_HEADER)
  rect(0, L.header_h - 1, L.W, 1, C_ACCENT_DK)
  rect(8, 6, 3, 10, C_ACCENT)
  text(16, 7, "SEED FINDER", C_TEXT)
  text_right(L.W - 8, 7, "Enter search   Up/Down select   Esc back", C_MUTE)

  rect(L.lpx, L.lpy, L.lpw, L.lph, C_PANEL)
  border(L.lpx, L.lpy, L.lpw, L.lph, C_BORDER)
  rect(L.rpx, L.content_y, L.rpw, L.content_bottom - L.content_y, C_PANEL)
  border(L.rpx, L.content_y, L.rpw, L.content_bottom - L.content_y, C_BORDER)

  draw_tabs(L)
  if S.page == "settings" then
    draw_settings_page(L)
  elseif S.page == "biomes" then
    draw_biomes_page(L)
  else
    draw_rules_page(L)
  end
  button(L.inner_x, L.search_y, L.inner_w, L.search_h, S.running and "Stop search" or "Search seeds",
    search.toggle_search, { variant = S.running and "danger" or "accent" })

  rect(0, L.footer_y, L.W, L.footer_h, C_HEADER)
  rect(0, L.footer_y, L.W, 1, C_BORDER)
  local fy = L.footer_y + math.floor((L.footer_h - 16) / 2)
  local back_w, copy_w, use_w = 56, 64, 80
  button(L.pad, fy, back_w, 16, "Back", search.go_back, { variant = "ghost" })
  local have = search.selected_hit() ~= nil
  button(L.W - L.pad - use_w, fy, use_w, 16, "Use Seed", search.use_selected,
    { variant = "accent", disabled = not have })
  button(L.W - L.pad * 2 - use_w - copy_w, fy, copy_w, 16, "Copy Seed", search.copy_selected, { disabled = not have })
  local line, color = status_line()
  local mid_x = L.pad * 2 + back_w
  text_centered(mid_x, fy + 4, L.W - mid_x - (L.pad * 3 + use_w + copy_w), elide(line, L.W - mid_x - (L.pad * 3 + use_w + copy_w)), color)

  text(L.rpx + 6, L.content_y + 5, "RESULTS", C_DIMTEXT)
  text_right(L.rpx + L.rpw - 6, L.content_y + 5, "score", C_MUTE)
  rect(L.rpx + 1, L.list_top - 2, L.rpw - 2, 1, C_BORDER)
  local max_scroll = math.max(0, #S.hits - L.rows)
  S.scroll = minecraft.util.clamp(S.scroll, 0, max_scroll)
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
      local label = key .. "  " .. pretty(h.origin_biome or "?")
      if h.spawn_x ~= nil then
        label = label .. "  @" .. h.spawn_x .. "," .. h.spawn_z
      end
      text(L.list_x + 5, y + 2, elide(label, L.list_w - 40), sel and C_TEXT or C_DIMTEXT)
      text_right(L.list_x + L.list_w - 5, y + 2, string.format("%.1f", h.score or 0), sel and C_ACCENT or C_MUTE)
      add_hit(L.list_x, y, L.list_w, L.row_h, function() search.select_index(i) end)
    end
  end
  if #S.hits > L.rows then
    local track_h = L.list_bottom - L.list_top
    local bar_x = L.list_x + L.list_w - 3
    local thumb = math.max(12, math.floor(track_h * L.rows / #S.hits))
    local pos = math.floor((track_h - thumb) * S.scroll / math.max(1, max_scroll))
    rect(bar_x, L.list_top + pos, 3, thumb, 0xFF3A4658)
  end
  draw_map(L)
end

minecraft.on("screen_ui", {
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
    search.open_finder()
  end)
  return event
end)

minecraft.on("screen_event", { screen_id = SCREEN_ID }, function(event)
  if event.phase == "init" then
    local L = layout(event.width, event.height)
    S.layout = L
    for _, f in ipairs(L.fields) do
      minecraft.screen.add_field(f.name, f.x, f.y, f.w, f.h, {
        text = S.saved[f.name] or "",
        numeric = f.numeric or false,
        signed = f.signed or false,
        max_len = 20,
      })
    end
    minecraft.screen.set_fields_visible(S.page == "settings")
    return event
  elseif event.phase == "render" then
    render_screen(event)
    return event
  elseif event.phase == "tick" then
    S.tick_count = S.tick_count + 1
    search.tick()
    return event
  elseif event.phase == "mouse" then
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
    S.scroll = math.max(0, S.scroll + ((event.delta or 0) > 0 and -1 or 1))
    event.handled = true
    return event
  elseif event.phase == "key" then
    if event.key == KEY_ESCAPE then
      search.go_back()
      event.handled = true
    elseif event.key == KEY_ENTER then
      search.toggle_search()
      event.handled = true
    elseif event.key == KEY_UP then
      search.move_selection(-1)
      event.handled = true
    elseif event.key == KEY_DOWN then
      search.move_selection(1)
      event.handled = true
    end
    return event
  elseif event.phase == "close" then
    search.save_fields()
    search.stop_job()
    search.stop_detail()
    search.free_map()
    return event
  end
end)
