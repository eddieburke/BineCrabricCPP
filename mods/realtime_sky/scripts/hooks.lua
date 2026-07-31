local solar = require("scripts.solar")
local sky_render = require("scripts.sky_render")
local places = require("scripts.places")
local globe_ui = require("scripts.globe_ui")
local settings_ui = require("scripts.settings_ui")
local config = require("config")

local SCREEN_ID = "realtime_sky:globe"
local SEARCH_FIELD = "place_search"
-- world_color must run AFTER colorful_skies (priority 20): lower number = later.
local SKY_COLOR_PRIORITY = 10
local SKY_PROVIDER_PRIORITY = 50

local clamp = minecraft.util.clamp

local ui = {
  search = "",
  list_scroll = 0,
  selected_index = 1,
  filtered = nil,
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
}

local solar_frame_cache

local function normalize_time_zone_id(value)
  if type(value) ~= "string" then return "GMT+0" end
  value = value:gsub("%s+", "")
  if value:match("^GMT[+-]%d%d?$") or value:match("^GMT[+-]%d%d?:%d%d$") then
    return value
  end
  local sign, h, m = value:match("^GMT([%+%-])(%d%d?):?(%d%d?)$")
  if sign then
    h = h or "0"
    m = m or "0"
    if #m == 1 then m = m .. "0" end
    return "GMT" .. sign .. h .. ":" .. m
  end
  local sign2, h2, m2 = value:match("^UTC([%+%-])(%d%d?):?(%d%d?)$")
  if sign2 then
    h2 = h2 or "0"
    m2 = m2 or "0"
    if #m2 == 1 then m2 = m2 .. "0" end
    return "GMT" .. sign2 .. h2 .. ":" .. m2
  end
  minecraft.log("error", "realtime_sky: invalid time zone '" .. tostring(value) .. "', falling back to GMT+0")
  return "GMT+0"
end

local function save_settings()
  solar_frame_cache = nil
  config.time_zone_id = normalize_time_zone_id(config.time_zone_id)
end

local function realtime_active()
  return config.enabled == true
end

local function current_solar_frame(partial_ticks)
  local now = minecraft.time.utc_millis()
  if solar_frame_cache == nil or now < solar_frame_cache.sample_millis or
      now - solar_frame_cache.sample_millis >= 50.0 then
    solar_frame_cache = solar.build_frame(config, partial_ticks or 0.0, now)
    solar_frame_cache.sample_millis = now
  end
  return solar_frame_cache
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

local function ensure_filtered()
  if ui.filtered == nil then
    refresh_filter()
  end
end

local function frame_globe_to(lat, lon)
  ui.globe_yaw = -lon
  ui.globe_pitch = clamp(-lat, -89.0, 89.0)
  ui.globe_cam = 2.05
end

local function apply_place(place)
  config.latitude = place.lat
  config.longitude = place.lon
  config.time_zone_id = normalize_time_zone_id(place.time_zone_id)
  config.place_chosen = true
  frame_globe_to(place.lat, place.lon)
  save_settings()
end

local function apply_globe_pick(lat, lon)
  config.latitude = lat
  config.longitude = lon
  config.place_chosen = true
  local nearest = places.nearest(lat, lon)
  if nearest ~= nil then
    config.time_zone_id = normalize_time_zone_id(nearest.time_zone_id)
  end
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
  local control_w = math.floor((col_w - 8) / 3)
  ui.search_x = left
  ui.search_y = 36
  ui.search_w = col_w
  ui.search_h = 18
  ui.settings_x = left + control_w + 4
  ui.settings_w = control_w
  ui.done_x = left + (control_w + 4) * 2
  ui.done_w = col_w - (control_w + 4) * 2
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

local function place_label(place, max_width)
  if place.country and #place.country > 0 then
    return fit_text(place.name .. " (" .. place.country .. ")", max_width)
  end
  return fit_text(place.name, max_width)
end

local function format_civil(millis, offset_minutes)
  local local_ms = millis + (offset_minutes or 0) * 60000.0
  local day_ms = 86400000.0
  local day_index = math.floor(local_ms / day_ms)
  local millis_of_day = local_ms - day_index * day_ms
  if millis_of_day < 0 then
    millis_of_day = millis_of_day + day_ms
  end
  local hour = math.floor(millis_of_day / 3600000.0)
  local minute = math.floor((millis_of_day - hour * 3600000.0) / 60000.0)
  return string.format("%02d:%02d", hour, minute)
end

local function draw_list(width, height, mouse_x, mouse_y)
  ensure_filtered()
  local left = list_column_left(width)
  local top = list_top()
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
  local zone_text = fit_text("Zone  " .. config.time_zone_id, col_w - minecraft.gui.text_width(count_text) - 10)
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
  local zoom_label = string.format(
    "GLOBE  drag to rotate  |  wheel to zoom  |  %.1fx",
    globe_ui.zoom_factor(ui))
  minecraft.gui.draw_text(ui.globe_x, 22,
    fit_text(zoom_label, ui.globe_size), 0xFF9BB0C2)
end

local function draw_globe_overlay(width)
  local bar_h = 42
  local y0 = ui.globe_y + ui.globe_size - bar_h
  minecraft.gui.fill_rect(ui.globe_x, y0, ui.globe_size, bar_h, 0xC0202020)
  local coords = format_lat_lon(config.latitude, config.longitude)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 2, ui.globe_size, coords, 0xFFFFFFFF)
  local frame = current_solar_frame(0.0)
  local zone = solar.time_zone_info(config)
  local local_clock = format_civil(frame.utc_millis, zone.offset_minutes)
  local utc_clock = format_civil(frame.utc_millis, 0)
  local day_flag = frame.is_daylight and "DAY" or "NIGHT"
  local status = string.format("Local %s  UTC %s  %s", local_clock, utc_clock, day_flag)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 14, ui.globe_size,
    fit_text(status, ui.globe_size - 8), 0xFFFFD36A)
  local sun_line = string.format("Sun az %03.0f  alt %+.1f", frame.sun_azimuth_deg, frame.sun_altitude_deg)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 26, ui.globe_size,
    fit_text(sun_line, ui.globe_size - 8), 0xFF88EE88)
end

local function draw_globe_chrome()
  local border = 2
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y - border, ui.globe_size + border * 2, border, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y + ui.globe_size, ui.globe_size + border * 2, border, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y, border, ui.globe_size, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x + ui.globe_size, ui.globe_y, border, ui.globe_size, 0xFF505860)
end

local function open_globe_screen()
  minecraft.screen.open(SCREEN_ID, { title = "" })
end

local function nearest_world_time_with_phase(world_time, target_tick)
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

local function register()
  minecraft.on("screen_ui", {
    screen_id = minecraft.screen.ids.world_settings,
    region = minecraft.screen.regions.screen,
    priority = 100,
  }, function(event)
    if event.ui == nil then return event end
    local function realtime_label()
      return config.enabled and "Realtime: ON" or "Realtime: OFF"
    end
    event.ui:add_centered_button(143,
      realtime_label(),
      function()
        config.enabled = not config.enabled
        save_settings()
      end,
      realtime_label)
    return event
  end)

  minecraft.on("world_render", {
    stage = "sky",
    moment = "before",
    is_overworld = true,
    priority = SKY_PROVIDER_PRIORITY + 1000,
    when = function()
      return realtime_active()
    end,
  }, function(event)
    if event.time_mode and event.time_mode ~= 0 then return event end
    local frame = current_solar_frame(event.tick_delta)
    event.astronomy_enabled = true
    event.astronomy_utc_millis = frame.utc_millis
    event.observer_latitude_deg = config.latitude
    event.observer_longitude_deg = config.longitude

    if config.drive_sun then
      event.celestial_angle = frame.celestial
      event.sky_yaw_deg = frame.skydome_yaw_deg
      event.sun_direction_x = frame.sun_direction_x
      event.sun_direction_y = frame.sun_direction_y
      event.sun_direction_z = frame.sun_direction_z
      event.sun_azimuth_deg = frame.sun_azimuth_deg
      event.sun_altitude_deg = frame.sun_altitude_deg
      event.cancel_vanilla = true
      sky_render.draw_dome(event, frame)
      sky_render.draw_stars(event, frame)
    end
  end)

  minecraft.on("world_color", {
    kind = "sky",
    is_overworld = true,
    priority = SKY_COLOR_PRIORITY,
    when = function()
      return realtime_active()
    end,
  }, function(event)
    if event.time_mode and event.time_mode ~= 0 then return event end
    local frame = current_solar_frame(event.partial_ticks)
    event.r, event.g, event.b = sky_render.sky_rgb(frame)
  end)

  minecraft.on("world_color", {
    kind = "fog",
    is_overworld = true,
    priority = SKY_COLOR_PRIORITY,
    when = function()
      return realtime_active()
    end,
  }, function(event)
    if event.time_mode and event.time_mode ~= 0 then return event end
    local frame = current_solar_frame(event.partial_ticks)
    event.r, event.g, event.b = sky_render.fog_rgb(frame)
  end)

  minecraft.on("screen_ui", {
    screen_id = minecraft.screen.ids.mod_settings,
    region = minecraft.screen.regions.footer,
    priority = 90,
  }, function(event)
    if event.ui ~= nil then
      event.ui:add_stacked_centered_button("Sky Globe...", open_globe_screen)
    end
    return event
  end)

  minecraft.on("screen_event", { screen_id = SCREEN_ID, priority = 100 }, function(event)
    if event.phase == "init" then
      layout(event.width, event.height)
      refresh_filter()
      frame_globe_to(config.latitude, config.longitude)
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
        minecraft.screen.open("realtime_sky:settings", { title = "" })
      end)
      globe_ui.load_data()
    elseif event.phase == "tick" then
      local text = minecraft.screen.field_text(SEARCH_FIELD)
      if text ~= nil and text ~= ui.search then
        ui.search = text
        refresh_filter()
      end
    elseif event.phase == "render" then
      layout(event.width, event.height)
      if ui.dragging then
        local degrees = globe_ui.drag_degrees_per_pixel(ui)
        ui.globe_yaw = ui.globe_yaw + (event.mouse_x - ui.drag_last_x) * degrees
        ui.globe_pitch = clamp(ui.globe_pitch + (event.mouse_y - ui.drag_last_y) * degrees, -89.0, 89.0)
        ui.drag_last_x = event.mouse_x
        ui.drag_last_y = event.mouse_y
      end
      draw_screen_chrome(event.width, event.height)
      globe_ui.draw(ui, event.width, event.height, config.latitude, config.longitude)
      draw_globe_chrome()
      draw_globe_overlay(event.width)
      draw_list(event.width, event.height, event.mouse_x, event.mouse_y)
    elseif event.phase == "mouse" then
      if event.released then
        if ui.dragging and event.button == 0 then
          local dx = event.x - ui.press_x
          local dy = event.y - ui.press_y
          if dx * dx + dy * dy <= 25 then
            local picked = globe_ui.pick_lat_lon(ui, event.width, event.height, event.x, event.y)
            if picked then
              apply_globe_pick(picked.lat, picked.lon)
            end
          end
        end
        ui.dragging = false
        return
      end
      if event.button ~= 0 then
        return
      end
      if globe_ui.contains_point(ui, event.x, event.y) then
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
        ensure_filtered()
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
        ensure_filtered()
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
      if globe_ui.contains_point(ui, event.x, event.y) then
        globe_ui.zoom(ui, event.delta)
        event.handled = true
      end
    elseif event.phase == "key" then
      if event.key == minecraft.key_code("escape") then
        save_settings()
        minecraft.screen.close()
        event.handled = true
      end
    elseif event.phase == "close" then
      save_settings()
      globe_ui.cleanup()
    end
  end)

  -- Stars stage is skipped when cancel_vanilla is set on sky; draw_stars handles that.
  -- Keep a stars hook for builds that still publish the stage (e.g. northern_stars consumers).
  minecraft.on("world_render", {
    stage = "stars",
    moment = "before",
    is_overworld = true,
    priority = 30,
    when = function()
      return realtime_active()
    end,
  }, function(event)
    local frame = current_solar_frame(event.tick_delta)
    event.star_brightness = sky_render.star_brightness(frame)
  end)

  minecraft.on("world_render", {
    stage = "terrain_opaque",
    moment = "before",
    is_overworld = true,
    priority = -1000,
    when = function()
      return realtime_active()
    end,
  }, function(event)
    local frame = current_solar_frame(event.tick_delta)
    local rain_alpha = 1.0 - clamp(event.rain_strength or 0.0, 0.0, 1.0) * 0.80

    local sun_altitude = frame.sun_altitude_deg or -90.0
    local sun_alpha = (function()
      local t = clamp((sun_altitude - (-1.15)) / (0.35 - (-1.15)), 0.0, 1.0)
      return t * t * (3.0 - 2.0 * t)
    end)() * rain_alpha
    sky_render.draw_sun(event, frame, sun_alpha)

    local moon_altitude = frame.moon_altitude_deg or -90.0
    local moon_t = clamp((moon_altitude - (-0.35)) / (1.25 - (-0.35)), 0.0, 1.0)
    local moon_alpha = moon_t * moon_t * (3.0 - 2.0 * moon_t) * rain_alpha
    if moon_alpha > 0.001 then
      local distance_scale = clamp(60.27 /
        math.max(frame.moon_distance_earth_radii or 60.27, 1.0), 0.90, 1.12)
      sky_render.draw_moon(event, frame, moon_alpha,
        sky_render.MOON_MEAN_HALF_SIZE * distance_scale)
    end
  end)

  -- time_mode is not pushed on world_tick; rely on render/color hooks for Day/Night yield.
  minecraft.on("world_tick", { before = false }, function(event)
    if not realtime_active() then
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

  settings_ui.register(config, function()
    save_settings()
  end)
end

return {
  register = register,
  current_solar_frame = current_solar_frame,
}
