local clamp = minecraft.util.clamp
local PI = 3.14159265

minecraft.settings.register("Colorful Skies", {
  { key = "enabled", label = "Colorful Sky", kind = "toggle", default = true },
  { key = "dynamic_colors", label = "Daily Twilight Colors", kind = "toggle", default = false },
  { key = "day_sky_r", label = "Day Sky Red", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.36 },
  { key = "day_sky_g", label = "Day Sky Green", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.62 },
  { key = "day_sky_b", label = "Day Sky Blue", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 1.0 },
  { key = "twilight_strength", label = "Twilight Strength", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.35 },
  { key = "night_brightness", label = "Night Brightness", kind = "slider", min = 0.05, max = 0.6, step = 0.01, decimals = 2, default = 0.18 },
  { key = "night_sky_r", label = "Night Sky Red", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.02 },
  { key = "night_sky_g", label = "Night Sky Green", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.02 },
  { key = "night_sky_b", label = "Night Sky Blue", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.08 },
  { key = "fog_blend", label = "Fog Color Blend", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.35 },
})

local function value(key, fallback)
  local current = minecraft.settings.get(key)
  if current == nil then
    minecraft.log("warn", "colorful_skies: using fallback for setting '" .. key .. "'")
    return fallback
  end
  return current
end

local function lerp(a, b, t)
  return a + (b - a) * clamp(t, 0.0, 1.0)
end

local function hsb_to_rgb(hue, saturation, brightness)
  if saturation == 0 then return brightness, brightness, brightness end
  local h = (hue - math.floor(hue)) * 6.0
  local i = math.floor(h)
  local f = h - i
  local p = brightness * (1.0 - saturation)
  local q = brightness * (1.0 - saturation * f)
  local t = brightness * (1.0 - saturation * (1.0 - f))
  i = i % 6
  if i == 0 then return brightness, t, p end
  if i == 1 then return q, brightness, p end
  if i == 2 then return p, brightness, t end
  if i == 3 then return p, q, brightness end
  if i == 4 then return t, p, brightness end
  return brightness, p, q
end

local function hash_for_day(seed, day, offset)
  local x = seed + day * 3 + offset
  x = ((x * 1103515245) + 12345) % 2147483647
  x = ((x * 1103515245) + 12345) % 2147483647
  return (x % 10000) / 10000.0
end

local session_seed = 0
local last_day = -1
local sunset_r, sunset_g, sunset_b = 1.0, 0.52, 0.20
local midnight_r, midnight_g, midnight_b = 0.08, 0.12, 0.35
local sunrise_r, sunrise_g, sunrise_b = 1.0, 0.42, 0.28

local function refresh_daily_colors(day)
  local function color(offset)
    return hsb_to_rgb(
      hash_for_day(session_seed, day, offset),
      0.55 + hash_for_day(session_seed, day, offset + 1) * 0.25,
      0.72 + hash_for_day(session_seed, day, offset + 2) * 0.20)
  end
  sunset_r, sunset_g, sunset_b = color(0)
  midnight_r, midnight_g, midnight_b = color(3)
  sunrise_r, sunrise_g, sunrise_b = color(6)
end

local function update_daily_colors()
  local world_time = minecraft.world.get_time()
  local day = math.floor(world_time / 24000)
  if day == last_day then return end
  last_day = day
  if day == 0 then session_seed = world_time end
  refresh_daily_colors(day)
end

local function celestial_angle(world_time)
  local raw = world_time / 24000.0 - 0.25
  raw = raw - math.floor(raw)
  local curved = 1.0 - (math.cos(raw * PI) + 1.0) / 2.0
  return raw + (curved - raw) / 3.0
end

local function sky_color(world_time, celestial)
  local day_r = value("day_sky_r", 0.36)
  local day_g = value("day_sky_g", 0.62)
  local day_b = value("day_sky_b", 1.0)
  local angle = celestial
  if type(angle) ~= "number" then angle = celestial_angle(world_time) end
  local transition = value("twilight_strength", 0.35)
  local night_r = value("night_sky_r", 0.02)
  local night_g = value("night_sky_g", 0.02)
  local night_b = value("night_sky_b", 0.08)
  local target_r, target_g, target_b

  if value("dynamic_colors", false) then
    update_daily_colors()
    if angle < 0.25 then
      local t = angle / 0.25
      target_r = lerp(day_r, sunset_r, t)
      target_g = lerp(day_g, sunset_g, t)
      target_b = lerp(day_b, sunset_b, t)
    elseif angle < 0.5 then
      local t = (angle - 0.25) / 0.25
      target_r = lerp(sunset_r, midnight_r, t)
      target_g = lerp(sunset_g, midnight_g, t)
      target_b = lerp(sunset_b, midnight_b, t)
    elseif angle < 0.75 then
      local t = (angle - 0.5) / 0.25
      target_r = lerp(midnight_r, sunrise_r, t)
      target_g = lerp(midnight_g, sunrise_g, t)
      target_b = lerp(midnight_b, sunrise_b, t)
    else
      local t = (angle - 0.75) / 0.25
      target_r = lerp(sunrise_r, day_r, t)
      target_g = lerp(sunrise_g, day_g, t)
      target_b = lerp(sunrise_b, day_b, t)
    end
    local twilight = 1.0 - math.abs(math.cos(angle * PI * 2.0))
    twilight = twilight * twilight * transition
    target_r = lerp(day_r, target_r, twilight)
    target_g = lerp(day_g, target_g, twilight)
    target_b = lerp(day_b, target_b, twilight)
  else
    local sun = math.cos(angle * PI * 2.0)
    local blend = 1.0 - clamp(sun * 2.0 + 0.5, 0.0, 1.0)
    target_r = lerp(day_r, night_r, blend)
    target_g = lerp(day_g, night_g, blend)
    target_b = lerp(day_b, night_b, blend)
  end

  local brightness = clamp(math.cos(angle * PI * 2.0) * 2.0 + 0.5, 0.0, 1.0)
  local night = value("night_brightness", 0.18)
  local light = night + brightness * (1.0 - night)
  return clamp(target_r * light, 0.0, 1.0), clamp(target_g * light, 0.0, 1.0), clamp(target_b * light, 0.0, 1.0)
end

local function active()
  return value("enabled", true) == true
end

local function handle_sky_color(event)
  if not active() then return event end
  event.r, event.g, event.b = sky_color(event.world_time or 0, event.celestial)
  return event
end

local function handle_fog_color(event)
  if not active() then return event end
  local r, g, b = sky_color(event.world_time or 0, event.celestial)
  local blend = value("fog_blend", 0.35)
  event.r = lerp(event.r, r, blend)
  event.g = lerp(event.g, g, blend)
  event.b = lerp(event.b, b, blend)
  return event
end

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.sky,
  is_overworld = true,
  priority = 20,
}, handle_sky_color)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,
  is_overworld = true,
  priority = 20,
}, handle_fog_color)
