-- ================================================================
-- MOD: critical_hit
-- Standardized structure with separated config
-- ================================================================

local config = require("critical_hit.config")

minecraft.settings.register("Critical Hit", {
  { key = "damage_multiplier", label = "Damage Multiplier", kind = "slider", min = 1.0, max = 2.5, step = 0.05, default = 1.5 },
  { key = "particle_count", label = "Particle Count", kind = "slider", min = 5, max = 40, integer = true, default = 20 },
  { key = "particle_scale", label = "Particle Scale", kind = "slider", min = 0.2, max = 2.0, step = 0.05, default = 1.2 },
})

local function spawn_crit_particles(x, y, z)
  local count = minecraft.settings.get("particle_count") or 20
  local scale = minecraft.settings.get("particle_scale") or 1.2
  for _ = 1, count do
    minecraft.particles.spawn({
      x = x,
      y = y,
      z = z,
      vx = (math.random() - 0.5) * 0.6,
      vy = math.random() * 0.4 + 0.1,
      vz = (math.random() - 0.5) * 0.6,
      scale = scale,
      r = 1.0,
      g = 0.95,
      b = 0.4,
      max_age = 30,
      gravity = 0.03,
    })
  end
end

minecraft.on(minecraft.events.attack_damage, {
  has_player = true,
  has_target = true,
  priority = 100,
  when = function(event)
    return not event.on_ground and (event.fall_distance or 0.0) > 0.5 and event.critical
  end,
}, function(event)
  local multiplier = minecraft.settings.get("damage_multiplier") or 1.5
  event.damage = math.max(event.damage + 1, math.floor(event.damage * multiplier + 0.5))
  event.critical = true
  spawn_crit_particles(event.target_x, event.target_y, event.target_z)
end)
