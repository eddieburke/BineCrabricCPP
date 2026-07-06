local last_camera_y = 64.0
local FOG_START_Y = 16.0
local clamp = minecraft.util.clamp

local function fog_darkness(camera_y)
  if camera_y >= FOG_START_Y then
    return 0.0
  end
  return clamp((FOG_START_Y - camera_y) / FOG_START_Y, 0.0, 1.0)
end

minecraft.on(minecraft.events.client_tick, {
  before = false,
  paused = false,
  has_world = true,
  is_overworld = true,
  priority = 100,
}, function(event)
  last_camera_y = event.camera_y or 64.0
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,
  is_overworld = true,
  priority = 100,
}, function(event)
  local darkness = fog_darkness(last_camera_y)
  if darkness <= 0.0 then
    return
  end
  event.r = event.r * (1.0 - darkness)
  event.g = event.g * (1.0 - darkness)
  event.b = event.b * (1.0 - darkness)
end)
