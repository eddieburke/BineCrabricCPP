local meteors = {}
local meteorModels = {}
local spawnCooldown = 0
local cometTimer = 0
local nextId = 1
local SCREEN_ID = "meteors:controls"

minecraft.settings.register("Meteors", {
  { key = "gravity", label = "Gravity", kind = "slider", min = -0.12, max = -0.01, step = 0.005, decimals = 3, default = -0.04 },
  { key = "air_density", label = "Air Density", kind = "slider", min = 0.0002, max = 0.004, step = 0.0001, decimals = 4, default = 0.0012 },
  { key = "air_scale_height", label = "Air Scale Height", kind = "slider", min = 10, max = 50, step = 1, default = 25.0 },
  { key = "stone_strength", label = "Fragmentation Strength", kind = "slider", min = 2, max = 20, step = 0.5, default = 8.0 },
  { key = "fragment_min_size", label = "Minimum Fragment Size", kind = "slider", min = 0.05, max = 0.5, step = 0.01, decimals = 2, default = 0.15 },
  { key = "max_meteors", label = "Max Meteors", kind = "slider", min = 10, max = 150, integer = true, default = 50 },
})

minecraft.keybinds.register("spawn_meteor", { default = minecraft.key_code("m"), label = "Spawn Meteor" })
minecraft.keybinds.register("spawn_comet", { default = minecraft.key_code("n"), label = "Spawn Comet" })

local function clamp(v, min, max)
  if v < min then return min end
  if v > max then return max end
  return v
end

local function generateMeteorModel(size, seed, isComet)
  local cacheKey = "m_" .. math.floor(size * 10) .. "_" .. seed .. "_" .. (isComet and 1 or 0)
  if meteorModels[cacheKey] then
    return meteorModels[cacheKey]
  end

  local cells = {}
  local resolution = 12
  local halfRes = resolution * 0.5
  local radius = halfRes - 1
  local cx, cy, cz = halfRes, halfRes, halfRes

  for x = 0, resolution do
    for y = 0, resolution do
      for z = 0, resolution do
        local nx = (x - cx) / radius
        local ny = (y - cy) / radius
        local nz = (z - cz) / radius
        local dist = math.sqrt(nx * nx + ny * ny + nz * nz)

        local noise = 0
        noise = noise + math.sin(x * 4.7 + y * 6.3 + z * 8.1 + seed) * 0.35
        noise = noise + math.sin(x * 11.2 + y * 13.9 + seed * 1.7) * 0.2
        noise = noise + math.sin(z * 9.5 + y * 7.3 + seed * 2.3) * 0.15

        local threshold = 1.0
        if isComet then
          threshold = 0.7 + noise * 0.35
        end
        if dist + noise < threshold and dist > 0.08 then
          local brightness = clamp(1.0 - dist, 0, 1)
          local r, g, b
          if isComet then
            local ice = brightness * 0.8 + 0.2
            r = ice * (0.6 + math.sin(x + seed) * 0.1)
            g = ice * (0.7 + math.sin(y + seed * 2) * 0.1)
            b = 0.85 + math.sin(z + seed * 3) * 0.1
          else
            r = 0.25 + brightness * 0.35 + math.sin(x + seed) * 0.05
            g = 0.2 + brightness * 0.3 + math.sin(y + seed * 2) * 0.05
            b = 0.15 + brightness * 0.25 + math.sin(z + seed * 3) * 0.05
          end
          table.insert(cells, {
            x = x, y = y, z = z,
            r = clamp(r, 0, 1),
            g = clamp(g, 0, 1),
            b = clamp(b, 0, 1),
            a = 1.0
          })
        end
      end
    end
  end

  local handle, err = minecraft.model.voxels({
    cells = cells,
    resolution = resolution,
    origin_x = -0.5,
    origin_y = -0.5,
    origin_z = -0.5,
    key = cacheKey
  })

  if handle then
    meteorModels[cacheKey] = handle
  end

  return handle, err
end

local function spawnMeteorAt(x, y, z, vx, vy, vz, size, isComet)
  if #meteors >= (minecraft.settings.get("max_meteors") or 50) then return end

  local speed = math.sqrt(vx * vx + vy * vy + vz * vz)
  local tumblingSpeed = 2 + speed * 0.5

  local meteor = {
    id = nextId,
    x = x, y = y, z = z,
    vx = vx, vy = vy, vz = vz,
    yaw = math.random() * 360,
    pitch = math.random() * 360,
    roll = math.random() * 360,
    rotSpeed = {
      x = (math.random() - 0.5) * tumblingSpeed,
      y = (math.random() - 0.5) * tumblingSpeed,
      z = (math.random() - 0.5) * tumblingSpeed,
    },
    size = size,
    temperature = isComet and 0.05 or 0,
    age = 0,
    maxAge = 300 + math.random() * 300,
    seed = math.random(1, 99999),
    trail = {},
    isComet = isComet or false,
    alive = true,
  }

  nextId = nextId + 1
  table.insert(meteors, meteor)
  return meteor
end

local function spawnMeteor()
  local player = minecraft.world.player()
  if not player then return end

  local angle = math.random() * math.pi * 2
  local elev = -(20 + math.random() * 35)
  local speed = 4 + math.random() * 6
  local size = 0.3 + math.random() * 1.2

  local radElev = elev * math.pi / 180
  local vh = math.cos(radElev) * speed
  local vy = math.sin(radElev) * speed
  local vx = math.cos(angle) * vh
  local vz = math.sin(angle) * vh

  local spawnY = 150 + math.random() * 50
  local t = (spawnY - player.y) / (-vy)
  local sx = player.x - vx * t + (math.random() - 0.5) * 40
  local sz = player.z - vz * t + (math.random() - 0.5) * 40

  return spawnMeteorAt(sx, spawnY, sz, vx, vy, vz, size, false)
end

local function spawnMeteorFromKeybind()
  if spawnCooldown > 0 then return end
  if spawnMeteor() then
    spawnCooldown = 5
  end
end

local function spawnComet()
  local player = minecraft.world.player()
  if not player then return end

  local angle = math.random() * math.pi * 2
  local elev = -(5 + math.random() * 15)
  local speed = 0.5 + math.random() * 1.5
  local size = 4 + math.random() * 8

  local radElev = elev * math.pi / 180
  local vh = math.cos(radElev) * speed
  local vy = math.sin(radElev) * speed
  local vx = math.cos(angle) * vh
  local vz = math.sin(angle) * vh

  local dist = 400 + math.random() * 600
  local sx = player.x + math.cos(angle) * dist
  local sz = player.z + math.sin(angle) * dist
  local sy = 200 + math.random() * 100

  spawnMeteorAt(sx, sy, sz, vx, vy, vz, size, true)
end

local function updatePhysics()
  local i = 1
  while i <= #meteors do
    local m = meteors[i]
    m.age = m.age + 1

    if not m.alive or m.age > m.maxAge or m.y < -10 or m.y > 300 then
      m.alive = false
      table.remove(meteors, i)
    else
      local airDensity = (minecraft.settings.get("air_density") or 0.0012) * math.exp(clamp(-(m.y - 64) / (minecraft.settings.get("air_scale_height") or 25.0), -10, 10))
      if m.y > 100 then
        airDensity = airDensity * math.exp(clamp(-(m.y - 100) / 30, -10, 0))
      end

      local speedSq = m.vx * m.vx + m.vy * m.vy + m.vz * m.vz
      local speed = math.sqrt(speedSq)

      if speed > 0.001 then
        local crossSection = math.pi * m.size * m.size
        local volume = m.size * m.size * m.size + 0.1
        local dragForce = 0.5 * airDensity * speedSq * crossSection * 0.05 / volume
        local drag = dragForce / speed
        drag = math.min(drag, speed * 0.5)

        local invSpeed = 1 / speed
        m.vx = m.vx - (m.vx * invSpeed) * drag
        m.vy = m.vy - (m.vy * invSpeed) * drag
        m.vz = m.vz - (m.vz * invSpeed) * drag
      end

      m.vy = m.vy + (minecraft.settings.get("gravity") or -0.04)
      local dynPressure = 0.5 * airDensity * speedSq
      m.temperature = math.min(1, m.temperature + dynPressure * 0.000002)

      if not m.isComet and dynPressure > (minecraft.settings.get("stone_strength") or 8.0) / m.size and m.size > (minecraft.settings.get("fragment_min_size") or 0.15) * 2 then
        local numFrags = 2 + math.floor(math.random() * 2)
        local fragSize = m.size * math.pow(1 / numFrags, 1 / 2.5)
        local spreadSpeed = 0.1 + math.random() * 0.3

        for f = 1, numFrags do
          local a1 = math.random() * math.pi * 2
          local a2 = math.random() * math.pi
          local fs = fragSize * (0.7 + math.random() * 0.6)
          if fs >= (minecraft.settings.get("fragment_min_size") or 0.15) then
            spawnMeteorAt(
              m.x, m.y, m.z,
              m.vx + math.cos(a1) * math.sin(a2) * spreadSpeed,
              m.vy + math.cos(a2) * spreadSpeed,
              m.vz + math.sin(a1) * math.sin(a2) * spreadSpeed,
              fs, false
            )
          end
        end

        m.alive = false
        table.remove(meteors, i)
      else
        m.x = m.x + m.vx
        m.y = m.y + m.vy
        m.z = m.z + m.vz

        m.yaw = m.yaw + m.rotSpeed.x
        m.pitch = m.pitch + m.rotSpeed.y
        m.roll = m.roll + m.rotSpeed.z

        table.insert(m.trail, { x = m.x, y = m.y, z = m.z })
        if #m.trail > 24 then
          table.remove(m.trail, 1)
        end

        local bx = math.floor(m.x + 0.5)
        local by = math.floor(m.y + 1)
        local bz = math.floor(m.z + 0.5)
        if by >= 0 and by < 128 then
          local block = minecraft.world.get_block(bx, by, bz)
          if block ~= 0 then
            m.alive = false
            table.remove(meteors, i)
          end
        end

        i = i + 1
      end
    end
  end
end

local function spawnTrailParticles()
  for _, m in ipairs(meteors) do
    if m.alive and m.temperature > 0.05 then
      local speed = math.sqrt(m.vx * m.vx + m.vy * m.vy + m.vz * m.vz)
      if speed > 0.5 and m.y < 130 then
        local count = 1 + math.floor(m.temperature * 2)
        for p = 1, count do
          minecraft.particles.spawn({
            x = m.x + (math.random() - 0.5) * m.size,
            y = m.y + (math.random() - 0.5) * m.size,
            z = m.z + (math.random() - 0.5) * m.size,
            vx = (math.random() - 0.5) * 0.3,
            vy = math.random() * 0.3,
            vz = (math.random() - 0.5) * 0.3,
            scale = 0.5 + m.temperature * 1.5,
            r = 1, g = 0.5 + m.temperature * 0.3, b = 0.1,
            max_age = 10 + math.floor(m.temperature * 15),
            gravity = -0.01,
          })
        end
      end
    end
  end
end

local function renderMeteorBody(m)
  local handle, err = generateMeteorModel(m.size, m.seed, m.isComet)
  if handle then
    minecraft.model.draw(handle, {
      x = m.x, y = m.y, z = m.z,
      yaw = m.yaw, pitch = m.pitch, roll = m.roll,
      scale = m.size * 2,
      brightness = 1,
      cull = false,
      blend = true,
      depth_test = true,
      depth_write = true,
    })
  end
end

local function renderAll()
  for _, m in ipairs(meteors) do
    if m.alive then
      renderMeteorBody(m)
    end
  end
end

minecraft.on(minecraft.events.key_press, { pressed = true, handled = false, priority = 100 }, function(event)
  if event["repeat"] then return event end
  if event.key == minecraft.keybinds.get_code("meteors.spawn_meteor") then
    spawnMeteorFromKeybind()
    event.handled = true
  elseif event.key == minecraft.keybinds.get_code("meteors.spawn_comet") then
    spawnComet()
    event.handled = true
  end
  return event
end)

minecraft.screen.on_ui(minecraft.screen.ids.mod_settings, minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil then
    event.ui:add_stacked_centered_button("Meteor Controls...", function()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)
  end
  return event
end, 80)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID, priority = 100 }, function(event)
  if event.phase == "init" then
    local x = math.floor(event.width / 2 - 100)
    minecraft.screen.add_button(x, 52, 200, 20, "Spawn Meteor", function()
      minecraft.screen.close()
      spawnMeteorFromKeybind()
    end)
    minecraft.screen.add_button(x, 80, 200, 20, "Spawn Comet", function()
      minecraft.screen.close()
      spawnComet()
    end)
    minecraft.screen.add_button(x, 116, 200, 20, "Done", function()
      minecraft.screen.close()
    end)
  elseif event.phase == "render" then
    minecraft.gui.fill_rect(4, 4, event.width - 8, event.height - 8, 0xD9121A25)
    minecraft.gui.draw_text(math.floor(event.width / 2 - 48), 22, "METEOR CONTROLS", 0xFFFFFFFF)
  elseif event.phase == "key" and event.key == minecraft.keys.escape then
    minecraft.screen.close()
    event.handled = true
  end
  return event
end)

minecraft.on(minecraft.events.client_tick, { after_world = true }, function(event)
  if spawnCooldown > 0 then spawnCooldown = spawnCooldown - 1 end
  cometTimer = cometTimer + 1
  if cometTimer > 1800 + math.random(600) then
    spawnComet()
    cometTimer = 0
  end
  updatePhysics()
  spawnTrailParticles()
  return event
end)

minecraft.on(minecraft.events.world_render, { stage = "entities", moment = "after" }, function(event)
  renderAll()
  return event
end)