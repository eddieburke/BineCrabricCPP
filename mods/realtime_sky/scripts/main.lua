local earth_time_solar = minecraft.require("scripts.earth_time_solar")
local places = minecraft.require("scripts.cities")
local globe_ui = minecraft.require("scripts.globe_ui")
local settings_screen_mod = minecraft.require("scripts.settings_screen")

local SETTINGS_SCREEN_ID = "realtime_sky:settings"

local SCREEN_ID = "realtime_sky:globe"
local SEARCH_FIELD = "place_search"
local CONFIG_FILE = "realtime_sky.txt"
local SKY_PROVIDER_PRIORITY = 50

local SETTINGS_DEFAULTS = {
  enabled = false,
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
  "enabled", "time_zone_id", "latitude", "longitude", "use_dst", "show_simulate_panel",
  "override_enabled", "simulate_date", "simulate_time",
  "sim_year", "sim_month", "sim_day", "sim_hour", "sim_minute",
}

local SETTINGS_NAMES = {
  enabled = "enabled",
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

local function smoothstep(edge0, edge1, value)
  local t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

local solar_frame_cache
local fit_text

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
  solar_frame_cache = nil
  settings.time_zone_id = normalize_time_zone_id(settings.time_zone_id)
  minecraft.config.save(CONFIG_FILE, settings, {
    keys = SETTINGS_KEYS,
    names = SETTINGS_NAMES,
    separator = ":",
  })
end

local function load_settings()
  local loaded, found = minecraft.config.load(CONFIG_FILE, SETTINGS_DEFAULTS, {
    aliases = SETTINGS_ALIASES,
  })
  -- Keep the table identity stable. settings_screen.register() holds this exact
  -- table; replacing it made the UI edit an abandoned copy after the first save.
  for key in pairs(settings) do
    settings[key] = nil
  end
  for key, value in pairs(loaded) do
    settings[key] = value
  end
  settings.time_zone_id = normalize_time_zone_id(settings.time_zone_id)
  clamp_settings()
  if not found then
    save_settings()
  end
end

local function realtime_active()
  return settings.enabled == true
end

local function utc_millis()
  return minecraft.time.utc_millis()
end

local function current_solar_frame(partial_ticks)
  local now = utc_millis()
  if solar_frame_cache == nil or now < solar_frame_cache.sample_millis or
      now - solar_frame_cache.sample_millis >= 50.0 then
    solar_frame_cache = earth_time_solar.build_frame(settings, partial_ticks or 0.0, now)
    solar_frame_cache.sample_millis = now
  end
  return solar_frame_cache
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

local function place_label(place, max_width)
  if place.country and #place.country > 0 then
    return fit_text(place.name .. " (" .. place.country .. ")", max_width)
  end
  return fit_text(place.name, max_width)
end

local function frame_globe_to(lat, lon)
  -- Match the original globe's readable oblique framing instead of snapping
  -- the camera to the opposite hemisphere/pole.
  ui.globe_yaw = lon * 0.5
  ui.globe_pitch = clamp(lat * 0.35, -89.0, 89.0)
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
  return 76
end

local function list_bottom(height)
  return height - 32
end

local function visible_rows(height)
  return math.max(1, math.floor((list_bottom(height) - list_top()) / 14))
end

local function layout(width, height)
  local globe_left = 8
  local globe_top = 34
  local globe_right = list_column_left(width) - 8
  local globe_bottom = height - 32
  local available_width = math.max(1, globe_right - globe_left)
  local available_height = math.max(1, globe_bottom - globe_top)
  ui.globe_size = math.max(1, math.min(available_width, available_height))
  ui.globe_x = globe_left + math.floor((available_width - ui.globe_size) / 2)
  ui.globe_y = globe_top + math.floor((available_height - ui.globe_size) / 2)
  local left = list_column_left(width)
  local col_w = list_column_width(width)
  local control_w = math.floor((col_w - 12) / 4)
  ui.search_x = left
  ui.search_y = 36
  ui.search_w = col_w
  ui.search_h = 18
  ui.enabled_toggle_y = height - 24
  ui.enabled_toggle_x = left
  ui.enabled_toggle_w = control_w
  ui.enabled_toggle_h = 20
  ui.dst_toggle_y = height - 24
  ui.dst_toggle_x = left + control_w + 4
  ui.dst_toggle_w = control_w
  ui.dst_toggle_h = 20
  ui.settings_x = left + (control_w + 4) * 2
  ui.settings_w = control_w
  ui.done_x = left + (control_w + 4) * 3
  ui.done_w = col_w - (control_w + 4) * 3
end

local function format_lat_lon(lat, lon)
  local ns = lat >= 0 and "N" or "S"
  local ew = lon >= 0 and "E" or "W"
  return string.format("%.2f %s  %.2f %s", math.abs(lat), ns, math.abs(lon), ew)
end

fit_text = function(text, max_width)
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

  minecraft.gui.draw_text(left, 22, "LOCATIONS", 0xFF7EC8F2)
  local count_text = tostring(#ui.filtered) .. " places"
  minecraft.gui.draw_text(left, 59, count_text, 0xFF9BB0C2)
  local zone_text = fit_text("Zone  " .. settings.time_zone_id, col_w - minecraft.gui.text_width(count_text) - 10)
  minecraft.gui.draw_text(left + col_w - minecraft.gui.text_width(zone_text), 59, zone_text, 0xFFFFD36A)

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
    minecraft.gui.draw_text(left, y, place_label(place, col_w - 4), selected and 0xFFFFFFFF or 0xFFC0C0C0)
  end

  if #ui.filtered == 0 then
    minecraft.gui.draw_text(left, top + 4, "(no matches)", 0xFFFF8080)
  end
end

local function draw_screen_chrome(width, height)
  local left = list_column_left(width)
  local col_w = list_column_width(width)
  minecraft.gui.fill_rect(4, 4, width - 8, 24, 0xD9121A25)
  minecraft.gui.fill_rect(left - 5, 30, col_w + 10, height - 60, 0xB80D141D)
  minecraft.gui.fill_rect(left - 5, 30, 2, height - 60, 0xFF31536B)
  minecraft.gui.draw_text(10, 12, "REAL-TIME SKY", 0xFFFFFFFF)
  minecraft.gui.draw_text(ui.globe_x, 22, "GLOBE  drag to rotate  |  wheel to zoom", 0xFF9BB0C2)
end

local function draw_globe_overlay(width)
  local bar_h = 28
  local y0 = ui.globe_y + ui.globe_size - bar_h
  minecraft.gui.fill_rect(ui.globe_x, y0, ui.globe_size, bar_h, 0xC0202020)
  local coords = format_lat_lon(settings.latitude, settings.longitude)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 3, ui.globe_size, coords, 0xFFFFFFFF)
  local frame = current_solar_frame(0.0)
  local sun_line = string.format("Sun %03.0f deg   %+.1f deg", frame.sun_azimuth_deg, frame.sun_altitude_deg)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 15, ui.globe_size,
    fit_text(sun_line, ui.globe_size - 8), 0xFF88EE88)
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

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY + 1000,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.tick_delta)
  -- This runtime's sky renderer consumes celestial_angle as the physical
  -- rotation angle in radians. `celestial` remains the normalized 0..1
  -- Minecraft clock phase used by brightness/time-of-day code.
  event.celestial_angle = frame.sun_angle
  event.celestial = frame.celestial
  event.sky_yaw_deg = frame.skydome_yaw_deg
  event.astronomy_enabled = true
  event.astronomy_utc_millis = frame.utc_millis
  event.observer_latitude_deg = settings.latitude
  event.observer_longitude_deg = settings.longitude

  -- Shared authoritative values for shader/light providers. These are from
  -- the same frame that places the visible sun and computes the solar clock.
  event.sun_direction_x = frame.sun_direction_x
  event.sun_direction_y = frame.sun_direction_y
  event.sun_direction_z = frame.sun_direction_z
  event.sun_azimuth_deg = frame.sun_azimuth_deg
  event.sun_altitude_deg = frame.sun_altitude_deg
  event.solar_day_tick = frame.day_tick
  event.solar_time_hours = frame.solar_time_hours
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.sky,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.partial_ticks)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 12.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 14.0, alt))
  event.r = 0.015 + (0.45 - 0.015) * day + 0.28 * warm
  event.g = 0.025 + (0.65 - 0.025) * day + 0.08 * warm
  event.b = 0.070 + (1.00 - 0.070) * day + 0.02 * warm
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.partial_ticks)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 10.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 12.0, alt))
  event.r = 0.025 + (0.75 - 0.025) * day + 0.20 * warm
  event.g = 0.035 + (0.85 - 0.035) * day + 0.06 * warm
  event.b = 0.080 + (1.00 - 0.080) * day
end)

open_globe_screen = function()
  minecraft.screen.open(SCREEN_ID, { title = "" })
end

minecraft.screen.on_ui(minecraft.screen.ids.world_settings, minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil then
    event.ui:add_stacked_centered_button("Sky Globe...", open_globe_screen)
  end
  return event
end, 90)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID, priority = 100 }, function(event)
  if event.phase == "init" then
    layout(event.width, event.height)
    refresh_filter()
    frame_globe_to(settings.latitude, settings.longitude)
    local button_y = event.height - 24
    minecraft.screen.add_field(SEARCH_FIELD, ui.search_x, ui.search_y, ui.search_w, ui.search_h, {
      text = ui.search,
      max_len = 32,
    })
    minecraft.screen.add_button(ui.done_x, button_y, ui.done_w, 20, "Done", function()
      save_settings()
      minecraft.screen.close()
    end)
    minecraft.screen.add_button(ui.settings_x, button_y, ui.settings_w, 20, "Settings", function()
      save_settings()
      minecraft.screen.open(SETTINGS_SCREEN_ID, { title = "" })
    end)
  elseif event.phase == "render" then
    ensure_coastlines()
    layout(event.width, event.height)
    local query = minecraft.screen.field_text(SEARCH_FIELD) or ""
    if query ~= ui.search then
      ui.search = query
      ui.list_scroll = 0
      ui.selected_index = 1
      refresh_filter()
    end
    if ui.dragging then
      local dx = event.mouse_x - ui.drag_last_x
      local dy = event.mouse_y - ui.drag_last_y
      if dx ~= 0 or dy ~= 0 then
        local sens = 0.58 * (3.15 / ui.globe_cam)
        ui.globe_yaw = ui.globe_yaw + dx * sens
        ui.globe_pitch = ui.globe_pitch - dy * sens
        ui.globe_pitch = clamp(ui.globe_pitch, -89.0, 89.0)
        ui.drag_last_x = event.mouse_x
        ui.drag_last_y = event.mouse_y
      end
    end
    draw_screen_chrome(event.width, event.height)
    globe_ui.draw(ui, event.width, event.height, settings.latitude, settings.longitude)
    draw_globe_chrome()
    draw_globe_overlay(event.width)
    draw_list(event.width, event.height, event.mouse_x, event.mouse_y)
    minecraft.gui.draw_toggle({ x = ui.enabled_toggle_x, y = ui.enabled_toggle_y, width = ui.enabled_toggle_w,
      height = ui.enabled_toggle_h, label = "Sky", value = settings.enabled, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
    minecraft.gui.draw_toggle({ x = ui.dst_toggle_x, y = ui.dst_toggle_y, width = ui.dst_toggle_w,
      height = ui.dst_toggle_h, label = "DST", value = settings.use_dst, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
  elseif event.phase == "mouse" then
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
    if minecraft.util.in_rect(event.x, event.y, ui.enabled_toggle_x, ui.enabled_toggle_y,
        ui.enabled_toggle_w, ui.enabled_toggle_h) then
      settings.enabled = not settings.enabled
      save_settings()
      event.handled = true
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
  elseif event.phase == "scroll" then
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
  elseif event.phase == "key" then
    if event.key == minecraft.keys.escape then
      save_settings()
      minecraft.screen.close()
      event.handled = true
    end
  elseif event.phase == "close" then
    save_settings()
    globe_ui.cleanup()
  end
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.stars,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = 30,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.tick_delta)
  local darkness = 1.0 - smoothstep(-18.0, -4.0, frame.sun_altitude_deg)
  event.star_brightness = darkness * darkness * 0.5
end)


local function nearest_world_time_with_phase(world_time, target_tick)
  -- Preserve the current Minecraft day number while choosing the occurrence of
  -- the requested phase nearest to the current absolute world time. This makes
  -- the 23999 -> 0 wrap advance one day instead of jumping backward.
  local day_start = math.floor(world_time / 24000.0) * 24000.0
  local candidate = day_start + target_tick
  local delta = candidate - world_time
  if delta >= 12000.0 then
    candidate = candidate - 24000.0
  elseif delta < -12000.0 then
    candidate = candidate + 24000.0
  end
  return math.floor(candidate + 0.5)
end

-- Keep the actual world phase locked to the same apparent-solar tick used by
-- the visible sun. Minecraft normally advances 24000 ticks in 20 minutes, so a
-- real-time sky must correct that automatic increment. Only the time-of-day
-- phase is replaced; the nearest absolute day is retained across midnight.
minecraft.on(minecraft.events.world_tick, { before = false }, function(event)
  if not realtime_active() or event.remote then
    return
  end

  local frame = current_solar_frame(0.0)
  local target_tick = math.floor(frame.day_tick + 0.5) % 24000
  local world_time = minecraft.world.get_time()
  local synchronized_time = nearest_world_time_with_phase(world_time, target_tick)

  if synchronized_time ~= world_time then
    minecraft.world.set_time(synchronized_time)
  end
end)

settings_screen_mod.register(settings, function()
  save_settings()
end)

minecraft.log("info", "realtime_sky loaded")
