local earth_time_solar = minecraft.require("scripts.earth_time_solar")
local places = minecraft.require("scripts.places")
local globe_ui = minecraft.require("scripts.globe_ui")

local SCREEN_ID = "realtime_sky:globe"
local CONFIG_FILE = "realtime_sky.txt"
local SKY_PROVIDER_PRIORITY = 50

local SETTINGS_DEFAULTS = {
  time_zone_id = "GMT",
  latitude = 45.0,
  longitude = 0.0,
  use_dst = true,
  show_simulate_panel = false,
  override_enabled = false,
  simulate_date = false,
  simulate_time = false,
  sim_year = 2000,
  sim_month = 1,
  sim_day = 1,
  sim_hour = 0,
  sim_minute = 0,
}

local SETTINGS_KEYS = {
  "time_zone_id", "latitude", "longitude", "use_dst", "show_simulate_panel",
  "override_enabled", "simulate_date", "simulate_time",
  "sim_year", "sim_month", "sim_day", "sim_hour", "sim_minute",
}

local SETTINGS_NAMES = {
  time_zone_id = "timeZoneId",
  use_dst = "useDst",
  show_simulate_panel = "showSimulatePanel",
  override_enabled = "overrideEnabled",
  simulate_date = "simulateDate",
  simulate_time = "simulateTime",
  sim_year = "simYear",
  sim_month = "simMonth",
  sim_day = "simDay",
  sim_hour = "simHour",
  sim_minute = "simMinute",
}

local SETTINGS_ALIASES = {}
for internal_name, file_name in pairs(SETTINGS_NAMES) do
  SETTINGS_ALIASES[file_name] = internal_name
end

local settings = minecraft.util.copy(SETTINGS_DEFAULTS)
local ui = {
  search = "",
  list_scroll = 0,
  selected_index = 1,
  filtered = places.all(),
  globe_x = 10,
  globe_y = 32,
  globe_size = 220,
  globe_yaw = 0.0,
  globe_pitch = 0.0,
  globe_cam = 2.05,
  dragging = false,
  press_x = 0,
  press_y = 0,
  drag_last_x = 0,
  drag_last_y = 0,
  coast_loaded = false,
}

local clamp = minecraft.util.clamp

local function clamp_settings()
  settings.latitude = clamp(settings.latitude, -90.0, 90.0)
  settings.longitude = clamp(settings.longitude, -180.0, 180.0)
  settings.sim_year = math.floor(clamp(settings.sim_year, 1, 9999))
  settings.sim_month = math.floor(clamp(settings.sim_month, 1, 12))
  settings.sim_day = math.floor(clamp(settings.sim_day, 1, 31))
  settings.sim_hour = math.floor(clamp(settings.sim_hour, 0, 23))
  settings.sim_minute = math.floor(clamp(settings.sim_minute, 0, 59))
end

local function normalize_time_zone_id(value)
  if type(value) ~= "string" then
    return "GMT"
  end
  value = value:gsub("^%s+", ""):gsub("%s+$", "")
  if value == "" then
    return "GMT"
  end
  value = value:gsub("\\", "/")
  while value:sub(1, 1) == "/" do
    value = value:sub(2)
  end
  return value
end

local function save_settings()
  settings.time_zone_id = normalize_time_zone_id(settings.time_zone_id)
  if settings.simulate_date or settings.simulate_time then
    settings.override_enabled = true
  end
  minecraft.config.save(CONFIG_FILE, settings, {
    keys = SETTINGS_KEYS,
    names = SETTINGS_NAMES,
    separator = ":",
  })
end

local function load_settings()
  local found
  settings, found = minecraft.config.load(CONFIG_FILE, SETTINGS_DEFAULTS, {
    aliases = SETTINGS_ALIASES,
  })
  settings.time_zone_id = normalize_time_zone_id(settings.time_zone_id)
  if settings.simulate_date or settings.simulate_time then
    settings.override_enabled = true
  end
  clamp_settings()
  if not found then
    save_settings()
  end
end

local function utc_millis()
  return minecraft.time.utc_millis()
end

local function ensure_coastlines()
  if ui.coast_loaded then
    return
  end
  local coast_text = minecraft.read_asset("assets/globe_coasts.txt")
  if coast_text and coast_text ~= "" then
    globe_ui.load_coastlines(coast_text)
    ui.coast_loaded = true
  end
end

local function refresh_filter()
  ui.filtered = places.filter(ui.search)
  if ui.selected_index > #ui.filtered then
    ui.selected_index = math.max(1, #ui.filtered)
  end
  if ui.selected_index < 1 then
    ui.selected_index = 1
  end
end

local function frame_globe_to(lat, lon)
  ui.globe_yaw = lon
  ui.globe_pitch = -lat
  ui.globe_cam = 2.05
end

local function apply_place(place)
  settings.latitude = place.lat
  settings.longitude = place.lon
  settings.time_zone_id = normalize_time_zone_id(place.time_zone_id)
  clamp_settings()
  frame_globe_to(place.lat, place.lon)
  save_settings()
end

local function list_column_left(width)
  return math.floor(width / 2) + 6
end

local function list_column_width(width)
  return width - list_column_left(width) - 10
end

local function list_top()
  return 68
end

local function list_bottom(height)
  return height - 28
end

local function visible_rows(height)
  return math.max(1, math.floor((list_bottom(height) - list_top()) / 14))
end

local function layout(width, height)
  ui.globe_size = math.min(math.floor(width / 2) - 26, height - 72)
  if ui.globe_size < 120 then
    ui.globe_size = 120
  end
  ui.globe_x = 10
  ui.globe_y = 32
  local left = list_column_left(width)
  local col_w = list_column_width(width)
  ui.dst_toggle_y = height - 24
  ui.dst_toggle_x = left
  ui.dst_toggle_w = math.floor(col_w / 2) - 4
  ui.dst_toggle_h = 20
end

local function format_lat_lon(lat, lon)
  local ns = lat >= 0 and "N" or "S"
  local ew = lon >= 0 and "E" or "W"
  return string.format("%.2f %s  %.2f %s", math.abs(lat), ns, math.abs(lon), ew)
end

local function fit_text(text, max_width)
  if minecraft.gui.text_width(text) <= max_width then
    return text
  end
  for length = #text, 1, -1 do
    local candidate = text:sub(1, length) .. "..."
    if minecraft.gui.text_width(candidate) <= max_width then
      return candidate
    end
  end
  return "..."
end

local function draw_list(width, height, mouse_x, mouse_y)
  refresh_filter()
  local left = list_column_left(width)
  local top = list_top()
  local bottom = list_bottom(height)
  local col_w = list_column_width(width)
  local rows = visible_rows(height)

  if ui.list_scroll > math.max(0, #ui.filtered - rows) then
    ui.list_scroll = math.max(0, #ui.filtered - rows)
  end
  if ui.list_scroll < 0 then
    ui.list_scroll = 0
  end

  minecraft.gui.draw_text(left, 40, "Places", 0xFF909090)
  minecraft.gui.draw_text(left, 52, fit_text("Zone: " .. settings.time_zone_id, col_w), 0xFFFFFFAA)

  for row = 0, rows - 1 do
    local index = ui.list_scroll + row + 1
    if index > #ui.filtered then
      break
    end
    local place = ui.filtered[index]
    local y = top + row * 14
    local hover = mouse_x >= left and mouse_x < left + col_w and mouse_y >= y and mouse_y < y + 14
    local selected = index == ui.selected_index
    if selected or hover then
      minecraft.gui.fill_rect(left - 1, y - 1, col_w + 2, 16, selected and 0xFF404070 or 0xFF303848)
    end
    minecraft.gui.draw_text(left, y, fit_text(place.name, col_w - 4), selected and 0xFFFFFFFF or 0xFFC0C0C0)
  end

  if #ui.filtered == 0 then
    minecraft.gui.draw_text(left, top + 4, "(no matches)", 0xFFFF8080)
  end
end

local function draw_globe_overlay(width)
  local bar_h = 28
  local y0 = ui.globe_y + ui.globe_size - bar_h
  minecraft.gui.fill_rect(ui.globe_x, y0, ui.globe_size, bar_h, 0xC0202020)
  local coords = format_lat_lon(settings.latitude, settings.longitude)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 3, ui.globe_size, coords, 0xFFFFFFFF)
  local place = ui.filtered[ui.selected_index]
  if place then
    minecraft.gui.draw_centered_text(ui.globe_x, y0 + 15, ui.globe_size, fit_text(place.name, ui.globe_size - 8), 0xFF88EE88)
  end
end

local function draw_globe_chrome()
  local border = 2
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y - border, ui.globe_size + border * 2, border, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y + ui.globe_size, ui.globe_size + border * 2, border, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y, border, ui.globe_size, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x + ui.globe_size, ui.globe_y, border, ui.globe_size, 0xFF505860)
end

local open_globe_screen

load_settings()
ensure_coastlines()

minecraft.on(minecraft.events.client_tick, {
  before = false,
  paused = false,
  has_world = true,
  priority = SKY_PROVIDER_PRIORITY,
}, function(event)
  local frame = earth_time_solar.build_frame(settings, 0.0, utc_millis())
  local current = event.world_time or 0
  local day_base = current - (current % 24000)
  minecraft.world.set_time(day_base + frame.day_tick)
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY,
}, function(event)
  local frame = earth_time_solar.build_frame(settings, event.tick_delta or 0.0, utc_millis())
  event.celestial_angle = frame.sun_angle
  event.sky_yaw_deg = frame.skydome_yaw_deg
  event.astronomy_enabled = true
  event.astronomy_utc_millis = frame.utc_millis
  event.observer_latitude_deg = settings.latitude
  event.observer_longitude_deg = settings.longitude
end)

open_globe_screen = function()
  minecraft.screen.open(SCREEN_ID, { title = "Real-time sky" })
end

minecraft.screen.on_ui(minecraft.screen.ids.world_settings, minecraft.screen.regions.footer, function(event)
  event.ui.add_stacked_centered_button("Sky Globe...", open_globe_screen)
end, 100)

minecraft.screen.on_lua_screen(SCREEN_ID, {
  init = function(event)
    layout(event.width, event.height)
    refresh_filter()
    frame_globe_to(settings.latitude, settings.longitude)
    local left = list_column_left(event.width)
    local col_w = list_column_width(event.width)
    local button_y = event.height - 24
    minecraft.screen.add_button(left + math.floor(col_w / 2) + 4, button_y, math.floor(col_w / 2) - 4, 20, "Done", function()
      save_settings()
      minecraft.screen.close()
    end)
  end,
  render = function(event)
    ensure_coastlines()
    layout(event.width, event.height)
    if ui.dragging then
      local dx = event.mouse_x - ui.drag_last_x
      local dy = event.mouse_y - ui.drag_last_y
      if dx ~= 0 or dy ~= 0 then
        local sens = 0.58 * (3.15 / ui.globe_cam)
        ui.globe_yaw = ui.globe_yaw - dx * sens
        ui.globe_pitch = ui.globe_pitch - dy * sens
        ui.globe_pitch = clamp(ui.globe_pitch, -89.0, 89.0)
        ui.drag_last_x = event.mouse_x
        ui.drag_last_y = event.mouse_y
      end
    end
    globe_ui.draw(ui, event.width, event.height, settings.latitude, settings.longitude)
    draw_globe_chrome()
    draw_globe_overlay(event.width)
    draw_list(event.width, event.height, event.mouse_x, event.mouse_y)
    minecraft.gui.draw_toggle(ui.dst_toggle_x, ui.dst_toggle_y, ui.dst_toggle_w, ui.dst_toggle_h, "DST",
      settings.use_dst, event.mouse_x, event.mouse_y)
  end,
  mouse = function(event)
    if event.released then
      if ui.dragging and event.button == 0 then
        local dx = event.x - ui.press_x
        local dy = event.y - ui.press_y
        if dx * dx + dy * dy <= 9 then
          local picked = globe_ui.pick_lat_lon(ui, event.width, event.height, event.x, event.y)
          if picked then
            settings.latitude = picked.lat
            settings.longitude = picked.lon
            clamp_settings()
            save_settings()
          end
        end
      end
      ui.dragging = false
      return
    end
    if event.button ~= 0 then
      return
    end
    if minecraft.util.in_rect(event.x, event.y, ui.dst_toggle_x, ui.dst_toggle_y, ui.dst_toggle_w, ui.dst_toggle_h) then
      settings.use_dst = not settings.use_dst
      save_settings()
      event.handled = true
      return
    end
    if globe_ui.contains_point(event.x, event.y, ui.globe_x, ui.globe_y, ui.globe_size) then
      ui.dragging = true
      ui.press_x = event.x
      ui.press_y = event.y
      ui.drag_last_x = event.x
      ui.drag_last_y = event.y
      event.handled = true
      return
    end
    local left = list_column_left(event.width)
    local top = list_top()
    local bottom = list_bottom(event.height)
    local col_w = list_column_width(event.width)
    if event.x >= left and event.x < left + col_w and event.y >= top and event.y < bottom then
      local row = math.floor((event.y - top) / 14)
      local index = ui.list_scroll + row + 1
      if index >= 1 and index <= #ui.filtered then
        ui.selected_index = index
        apply_place(ui.filtered[index])
      end
      event.handled = true
    end
  end,
  scroll = function(event)
    if event.x >= list_column_left(event.width) then
      local rows = visible_rows(event.height)
      if event.delta > 0 then
        ui.list_scroll = ui.list_scroll - 3
      else
        ui.list_scroll = ui.list_scroll + 3
      end
      if ui.list_scroll < 0 then
        ui.list_scroll = 0
      end
      local max_scroll = math.max(0, #ui.filtered - rows)
      if ui.list_scroll > max_scroll then
        ui.list_scroll = max_scroll
      end
      event.handled = true
      return
    end
    if globe_ui.contains_point(event.x, event.y, ui.globe_x, ui.globe_y, ui.globe_size) then
      if event.delta > 0 then
        ui.globe_cam = ui.globe_cam - 0.24
      else
        ui.globe_cam = ui.globe_cam + 0.24
      end
      ui.globe_cam = clamp(ui.globe_cam, 1.5, 6.0)
      event.handled = true
    end
  end,
  key = function(event)
    if event.key == minecraft.keys.escape then
      save_settings()
      minecraft.screen.close()
      event.handled = true
    end
  end,
  close = function()
    save_settings()
  end,
}, 100)

minecraft.log("info", "realtime_sky loaded")
