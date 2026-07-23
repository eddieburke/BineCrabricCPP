local ui = {}

local function clamp(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

local function clamp_int(v, lo, hi)
  return math.floor(clamp(math.floor(v + 0.5), lo, hi))
end

local function read_int(field, default, lo, hi)
  local raw = minecraft.screen.field_text(field)
  if not raw or #raw == 0 then
    minecraft.log("debug", "realtime_sky: using default for empty int field '" .. field .. "'")
    return default
  end
  return clamp_int(tonumber(raw) or default, lo, hi)
end

local function read_num(field, default, lo, hi)
  local raw = minecraft.screen.field_text(field)
  if not raw or #raw == 0 then
    minecraft.log("debug", "realtime_sky: using default for empty num field '" .. field .. "'")
    return default
  end
  return clamp(tonumber(raw) or default, lo, hi)
end

local function read_str(field, default)
  local raw = minecraft.screen.field_text(field)
  if not raw or #raw == 0 then
    minecraft.log("debug", "realtime_sky: using default for empty str field '" .. field .. "'")
    return default
  end
  return raw
end

local function save_fields(settings, save_fn)
  settings.sim_year = read_int("sim_year", settings.sim_year, 1, 9999)
  settings.sim_month = read_int("sim_month", settings.sim_month, 1, 12)
  settings.sim_day = read_int("sim_day", settings.sim_day, 1, 31)
  settings.sim_hour = read_int("sim_hour", settings.sim_hour, 0, 23)
  settings.sim_minute = read_int("sim_minute", settings.sim_minute, 0, 59)
  settings.latitude = read_num("latitude", settings.latitude, -90, 90)
  settings.longitude = read_num("longitude", settings.longitude, -180, 180)
  settings.time_zone_id = read_str("timezone", settings.time_zone_id or "GMT+0")
  save_fn()
  ui.saved_ticks = 40
end

local function open_globe()
  minecraft.screen.open("realtime_sky:globe", { title = "" })
end

local function layout(width, height)
  ui.margin = 10
  ui.gap = 8
  ui.panel_y = 32
  ui.panel_h = height - 66
  ui.col_w = math.floor((width - ui.margin * 2 - ui.gap) / 2)
  ui.left_x = ui.margin
  ui.right_x = ui.left_x + ui.col_w + ui.gap
  ui.toggle_x = ui.right_x + 8
  ui.toggle_w = ui.col_w - 16
  ui.toggle_h = 22
  ui.toggle_ys = { 58, 86, 114, 142, 170 }
end

local function add_fields(settings)
  local x = ui.left_x + 8
  minecraft.screen.add_field("sim_year", x, 62, 50, 18, {
    numeric = true, max_len = 4, text = tostring(settings.sim_year) })
  minecraft.screen.add_field("sim_month", x + 58, 62, 38, 18, {
    numeric = true, max_len = 2, text = tostring(settings.sim_month) })
  minecraft.screen.add_field("sim_day", x + 104, 62, 38, 18, {
    numeric = true, max_len = 2, text = tostring(settings.sim_day) })
  minecraft.screen.add_field("sim_hour", x, 96, 42, 18, {
    numeric = true, max_len = 2, text = tostring(settings.sim_hour) })
  minecraft.screen.add_field("sim_minute", x + 50, 96, 42, 18, {
    numeric = true, max_len = 2, text = tostring(settings.sim_minute) })
  local half = math.floor((ui.col_w - 24) / 2)
  minecraft.screen.add_field("latitude", x, 144, half, 18, {
    numeric = true, signed = true, decimal = true, max_len = 10,
    text = string.format("%.4f", settings.latitude) })
  minecraft.screen.add_field("longitude", x + half + 8, 144, half, 18, {
    numeric = true, signed = true, decimal = true, max_len = 11,
    text = string.format("%.4f", settings.longitude) })
  minecraft.screen.add_field("timezone", x, 178, ui.col_w - 16, 18, {
    max_len = 32, text = settings.time_zone_id or "GMT+0" })
end

local function draw_labels()
  local x = ui.left_x + 8
  minecraft.gui.draw_text(x, 40, "DATE / TIME", 0xFF7EC8F2)
  minecraft.gui.draw_text(x, 52, "Year", 0xFF9BB0C2)
  minecraft.gui.draw_text(x + 58, 52, "Month", 0xFF9BB0C2)
  minecraft.gui.draw_text(x + 104, 52, "Day", 0xFF9BB0C2)
  minecraft.gui.draw_text(x, 86, "Hour", 0xFF9BB0C2)
  minecraft.gui.draw_text(x + 50, 86, "Minute", 0xFF9BB0C2)
  minecraft.gui.draw_text(x, 120, "OBSERVER", 0xFF7EC8F2)
  minecraft.gui.draw_text(x, 134, "Latitude", 0xFF9BB0C2)
  local half = math.floor((ui.col_w - 24) / 2)
  minecraft.gui.draw_text(x + half + 8, 134, "Longitude", 0xFF9BB0C2)
  minecraft.gui.draw_text(x, 166, "TIME ZONE", 0xFF7EC8F2)
end

local function draw_toggles(settings, mouse_x, mouse_y)
  local specs = {
    { "Date override", settings.simulate_date },
    { "Time override", settings.simulate_time },
    { "Simulation", settings.override_enabled },
    { "HUD panel", settings.show_simulate_panel },
    { "Realtime Sun", settings.drive_sun },
  }
  minecraft.gui.draw_text(ui.toggle_x, 44, "SIMULATION", 0xFF7EC8F2)
  for index, spec in ipairs(specs) do
    minecraft.gui.draw_toggle({
      x = ui.toggle_x,
      y = ui.toggle_ys[index],
      width = ui.toggle_w,
      height = ui.toggle_h,
      label = spec[1],
      value = spec[2],
      mouse_x = mouse_x,
      mouse_y = mouse_y,
    })
  end
  minecraft.gui.draw_text(ui.toggle_x, 202, "Use overrides to preview any date", 0xFF9BB0C2)
  minecraft.gui.draw_text(ui.toggle_x, 212, "or time without changing location.", 0xFF9BB0C2)
end

local function draw_screen(settings, width, height, mouse_x, mouse_y)
  minecraft.gui.fill_rect(4, 4, width - 8, 24, 0xD9121A25)
  minecraft.gui.fill_rect(ui.left_x, ui.panel_y, ui.col_w, ui.panel_h, 0xB80D141D)
  minecraft.gui.fill_rect(ui.right_x, ui.panel_y, ui.col_w, ui.panel_h, 0xB80D141D)
  minecraft.gui.fill_rect(ui.left_x, ui.panel_y, ui.col_w, 2, 0xFF31536B)
  minecraft.gui.fill_rect(ui.right_x, ui.panel_y, ui.col_w, 2, 0xFF31536B)
  minecraft.gui.draw_text(10, 12, "SKY SETTINGS", 0xFFFFFFFF)
  draw_labels()
  draw_toggles(settings, mouse_x, mouse_y)
  if ui.saved_ticks and ui.saved_ticks > 0 then
    minecraft.gui.draw_text(10, height - 20, "Saved", 0xFF75E6A4)
  end
end

local function toggle_at(settings, x, y)
  if not minecraft.util.in_rect(x, y, ui.toggle_x, ui.toggle_ys[1], ui.toggle_w, ui.toggle_h) and
      not minecraft.util.in_rect(x, y, ui.toggle_x, ui.toggle_ys[2], ui.toggle_w, ui.toggle_h) and
      not minecraft.util.in_rect(x, y, ui.toggle_x, ui.toggle_ys[3], ui.toggle_w, ui.toggle_h) and
      not minecraft.util.in_rect(x, y, ui.toggle_x, ui.toggle_ys[4], ui.toggle_w, ui.toggle_h) and
      not minecraft.util.in_rect(x, y, ui.toggle_x, ui.toggle_ys[5], ui.toggle_w, ui.toggle_h) then
    return false
  end
  for index, toggle_y in ipairs(ui.toggle_ys) do
    if minecraft.util.in_rect(x, y, ui.toggle_x, toggle_y, ui.toggle_w, ui.toggle_h) then
      if index == 1 then settings.simulate_date = not settings.simulate_date end
      if index == 2 then settings.simulate_time = not settings.simulate_time end
      if index == 3 then settings.override_enabled = not settings.override_enabled end
      if index == 4 then settings.show_simulate_panel = not settings.show_simulate_panel end
      if index == 5 then settings.drive_sun = not settings.drive_sun end
      return true
    end
  end
  return false
end

local SCREEN_ID = "realtime_sky:settings"

local function register(settings, save_fn)
  minecraft.on(minecraft.events.screen_event, {
    screen_id = SCREEN_ID,
    priority = 100,
  }, function(event)
    if event.phase == "init" then
      layout(event.width, event.height)
      ui.saved_ticks = 0
      add_fields(settings)
      minecraft.screen.add_button(event.width - 136, event.height - 26, 62, 20, "Save", function()
        save_fields(settings, save_fn)
      end)
      minecraft.screen.add_button(event.width - 70, event.height - 26, 62, 20, "Back", function()
        save_fields(settings, save_fn)
        open_globe()
      end)
    elseif event.phase == "render" then
      layout(event.width, event.height)
      draw_screen(settings, event.width, event.height, event.mouse_x, event.mouse_y)
    elseif event.phase == "tick" then
      if ui.saved_ticks and ui.saved_ticks > 0 then ui.saved_ticks = ui.saved_ticks - 1 end
    elseif event.phase == "mouse" then
      if not event.released and event.button == 0 and toggle_at(settings, event.x, event.y) then
        save_fields(settings, save_fn)
        event.handled = true
      end
    elseif event.phase == "key" then
      if event.key == minecraft.keys.escape then
        save_fields(settings, save_fn)
        open_globe()
        event.handled = true
      end
    end
  end)
end

return { register = register }
