local CHANCE_TO_SPAWN = 4000
local METEOR_KEY = minecraft.key_code("m")
local PI = 3.1415927

local function spawn_meteor_shower()
  local player = minecraft.world.player()
  if player == nil then
    return
  end
  local dist = 200.0 + math.random() * 100.0
  local angle = math.random() * PI * 2.0
  local start_x = player.x + math.cos(angle) * dist
  local start_z = player.z + math.sin(angle) * dist
  local start_y = 200.0 + math.random() * 100.0
  local speed = 3.0 + math.random() * 3.0
  local move_angle = angle + PI + (math.random() - 0.5) * 1.0
  local vel_x = math.cos(move_angle) * speed
  local vel_z = math.sin(move_angle) * speed
  local vel_y = -0.1 - math.random() * 0.3
  for i = 1, 8 do
    local t = i / 8.0
    minecraft.particles.spawn({
      x = start_x - vel_x * t * 6.0,
      y = start_y - vel_y * t * 6.0,
      z = start_z - vel_z * t * 6.0,
      vx = vel_x,
      vy = vel_y,
      vz = vel_z,
      scale = 0.8 + math.random() * 0.6,
      r = 1.0,
      g = 0.95,
      b = 0.8,
      max_age = 80,
      gravity = 0.0,
    })
  end
end

minecraft.on(minecraft.events.client_tick, {
  before = false,
  paused = false,
  has_world = true,
  priority = 100,
}, function(event)
  if event.is_night and minecraft.world.random(CHANCE_TO_SPAWN) == 0 then
    spawn_meteor_shower()
  end
end)

minecraft.on(minecraft.events.key_press, {
  key = METEOR_KEY,
  pressed = true,
  handled = false,
  priority = 100,
}, function(event)
  spawn_meteor_shower()
  event.handled = true
end)
