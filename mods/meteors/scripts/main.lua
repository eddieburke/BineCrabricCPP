local meteors = {}
local meteorModels = {}
local spawnCooldown = 0
local cometTimer = 0
local nextId = 1

local GRAVITY = -0.04
local AIR_DENSITY_BASE = 0.0012
local AIR_SCALE_HEIGHT = 25.0
local STONE_STRENGTH = 8.0
local FRAGMENT_MIN_SIZE = 0.15
local MAX_METEORS = 50

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
  local radius = size * 3
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
    key = cacheKey
  })

  if handle then
    meteorModels[cacheKey] = handle
  end

  return handle, err
end

local function spawnMeteorAt(x, y, z, vx, vy, vz, size, isComet)
  if #meteors >= MAX_METEORS then return end

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

  spawnMeteorAt(sx, spawnY, sz, vx, vy, vz, size, false)
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
      local airDensity = AIR_DENSITY_BASE * math.exp(clamp(-(m.y - 64) / AIR_SCALE_HEIGHT, -10, 10))
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

      m.vy = m.vy + GRAVITY
      local dynPressure = 0.5 * airDensity * speedSq
      m.temperature = math.min(1, m.temperature + dynPressure * 0.000002)

      if not m.isComet and dynPressure > STONE_STRENGTH / m.size and m.size > FRAGMENT_MIN_SIZE * 2 then
        local numFrags = 2 + math.floor(math.random() * 2)
        local fragSize = m.size * math.pow(1 / numFrags, 1 / 2.5)
        local spreadSpeed = 0.1 + math.random() * 0.3

        for f = 1, numFrags do
          local a1 = math.random() * math.pi * 2
          local a2 = math.random() * math.pi
          local fs = fragSize * (0.7 + math.random() * 0.6)
          if fs >= FRAGMENT_MIN_SIZE then
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

-- minecraft.render.billboards is the sky-dome renderer: it places each
-- billboard at a fixed distance along a camera-relative direction and has no
-- concept of world position, so it cannot be used to draw effects attached to
-- a meteor's actual location (that used to render as a fixed glow glued to
-- the middle of the screen). World-positioned trail/glow comes from
-- spawnTrailParticles instead, which uses minecraft.particles.spawn.

local function renderMeteorBody(m)
  local handle, err = generateMeteorModel(m.size, m.seed, m.isComet)
  if handle then
    minecraft.model.draw(handle, {
      x = m.x, y = m.y, z = m.z,
      yaw = m.yaw, pitch = m.pitch, roll = m.roll,
      scale = 1,
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

minecraft.at_phase("ready", 0, function()
  minecraft.on("key_press", { key = minecraft.key_code("m"), pressed = true }, function(event)
    if spawnCooldown <= 0 then
      spawnMeteor()
      spawnCooldown = 5
    end
    return event
  end)

  minecraft.on("key_press", { key = minecraft.key_code("n"), pressed = true }, function(event)
    spawnComet()
    return event
  end)

  minecraft.on("client_tick", { after_world = true }, function(event)
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

  minecraft.on("world_render", { stage = "entities", moment = "after" }, function(event)
    renderAll()
    return event
  end)
end)
