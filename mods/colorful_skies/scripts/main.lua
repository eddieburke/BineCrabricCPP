local function sky_color_for_celestial(celestial)
  local t = (celestial or 0.0) * 6.2831853
  local r = 0.45 + 0.35 * math.max(0.0, math.cos(t))
  local g = 0.55 + 0.30 * math.max(0.0, math.sin(t + 1.0))
  local b = 0.85 + 0.10 * math.max(0.0, math.cos(t + 2.0))
  return r, g, b
end

local clamp = minecraft.util.clamp

local function luminance(r, g, b)
  return r * 0.30 + g * 0.59 + b * 0.11
end

local function apply_colorful_tint(event, strength)
  local tint_r, tint_g, tint_b = sky_color_for_celestial(event.celestial)
  local base_luma = luminance(event.r, event.g, event.b)
  local tint_luma = math.max(0.001, luminance(tint_r, tint_g, tint_b))
  local scale = base_luma / tint_luma
  local target_r = clamp(tint_r * scale, 0.0, 1.0)
  local target_g = clamp(tint_g * scale, 0.0, 1.0)
  local target_b = clamp(tint_b * scale, 0.0, 1.0)
  event.r = clamp(event.r * (1.0 - strength) + target_r * strength, 0.0, 1.0)
  event.g = clamp(event.g * (1.0 - strength) + target_g * strength, 0.0, 1.0)
  event.b = clamp(event.b * (1.0 - strength) + target_b * strength, 0.0, 1.0)
  return event
end

local COLOR_EVENT = {
  is_overworld = true,
  priority = 100,
}

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.sky,
  is_overworld = COLOR_EVENT.is_overworld,
  priority = COLOR_EVENT.priority,
}, function(event)
  return apply_colorful_tint(event, 0.75)
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,
  is_overworld = COLOR_EVENT.is_overworld,
  priority = COLOR_EVENT.priority,
}, function(event)
  return apply_colorful_tint(event, 0.5)
end)
