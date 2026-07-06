local function spawn_crit_particles(x, y, z)
  for _ = 1, 10 do
    minecraft.particles.spawn({
      x = x,
      y = y,
      z = z,
      vx = (math.random() - 0.5) * 0.4,
      vy = math.random() * 0.25 + 0.05,
      vz = (math.random() - 0.5) * 0.4,
      scale = 0.35,
      r = 1.0,
      g = 0.95,
      b = 0.4,
      max_age = 12,
      gravity = 0.04,
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
  event.damage = math.max(event.damage + 1, math.floor(event.damage * 1.5 + 0.5))
  event.critical = true
  spawn_crit_particles(event.target_x, event.target_y, event.target_z)
end)
