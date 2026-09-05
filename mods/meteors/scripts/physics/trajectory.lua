local config = require("config")
local mathx = require("lib.mathx")

local physics = {}

local clamp, length3, normalize3 = mathx.clamp, mathx.length3, mathx.normalize3
local randomUnit, hash01 = mathx.randomUnit, mathx.hash01
local sin, cos, exp, floor, max, min, random = math.sin, math.cos, math.exp, math.floor, math.max, math.min, math.random
local DEG = mathx.deg_to_rad

local TRAIL_CAP = 96
local HEAT_REFERENCE = 5.0e-5
local FRAGMENT_PRESSURE = 1.2e-5
local FRAGMENT_COOLDOWN = 20
local MAX_AGE_TICKS = 1200
local COMET_HEAT_SCALE = 0.3

local function newMeteor(id, kind, variant, x, y, z, vx, vy, vz, size)
  return {
    id = id,
    kind = kind,
    gravityScale = kind == "comet" and 0.12 or 1.0,
    variant = variant,
    x = x, y = y, z = z,
    px = x, py = y, pz = z,
    vx = vx, vy = vy, vz = vz,
    size = size,
    mass = size * size * size * 2.5,
    alive = true,
    age = 0,
    heat = 0,
    prevHeat = 0,
    spinX = hash01(id, 7, 0, 0) * 360,
    spinY = hash01(id, 8, 0, 0) * 360,
    spinZ = hash01(id, 9, 0, 0) * 360,
    prevSpinX = 0, prevSpinY = 0, prevSpinZ = 0,
    spinRateX = (hash01(id, 11, 0, 0) - 0.5) * 7,
    spinRateY = (hash01(id, 12, 0, 0) - 0.5) * 7,
    spinRateZ = (hash01(id, 13, 0, 0) - 0.5) * 7,
    groundY = 0,
    fragCooldown = FRAGMENT_COOLDOWN,
    fragments = {},
    tx = {}, ty = {}, tz = {},
    trailCap = TRAIL_CAP,
    trailHead = 0,
    trailCount = 0,
  }
end

local function pushTrail(meteor)
  local head = meteor.trailHead + 1
  if head > TRAIL_CAP then head = 1 end
  meteor.trailHead = head
  meteor.tx[head] = meteor.x
  meteor.ty[head] = meteor.y
  meteor.tz[head] = meteor.z
  if meteor.trailCount < TRAIL_CAP then
    meteor.trailCount = meteor.trailCount + 1
  end
end

local function sampleGround(meteor)
  if meteor.y - meteor.groundY > 12 and (meteor.age + meteor.id) % 4 ~= 0 then
    return
  end
  local top = minecraft.world.get_top_y(floor(meteor.x), floor(meteor.z))
  if top >= 0 then
    meteor.groundY = top
  end
end

function physics.fragment(parent, stressFactor, nextId)
  local minSize = config.get("fragment_min_size") or 0.15
  if parent.size < minSize * 2 then return nextId end

  local count = min(4, max(2, floor(stressFactor)))
  for i = 1, count do
    local size = parent.size * (0.28 + mathx.hash01(parent.id, i, 41, 0) * 0.34)
    if size < minSize then break end
    local sx, sy, sz = randomUnit(parent.id * 100 + i, i)
    local fragment = newMeteor(nextId, parent.kind, (parent.variant + i) % 6,
      parent.x, parent.y, parent.z,
      parent.vx + sx * 0.10,
      parent.vy + sy * 0.06,
      parent.vz + sz * 0.10,
      size)
    nextId = nextId + 1
    fragment.heat = parent.heat
    fragment.prevHeat = parent.heat
    fragment.groundY = parent.groundY
    parent.fragments[#parent.fragments + 1] = fragment
  end

  parent.size = parent.size * 0.62
  parent.mass = parent.size * parent.size * parent.size * 2.5
  parent.fragCooldown = FRAGMENT_COOLDOWN
  if parent.size < minSize then
    parent.alive = false
  end
  return nextId
end

function physics.update(meteor, nextId)
  meteor.px, meteor.py, meteor.pz = meteor.x, meteor.y, meteor.z
  meteor.prevSpinX, meteor.prevSpinY, meteor.prevSpinZ = meteor.spinX, meteor.spinY, meteor.spinZ
  meteor.prevHeat = meteor.heat

  local gravity = config.get("gravity") or -0.02
  local airDensity = config.get("air_density") or 0.0012
  local scaleHeight = config.get("air_scale_height") or 25.0
  local strength = config.get("stone_strength") or 8.0
  local minSize = config.get("fragment_min_size") or 0.15

  meteor.vy = meteor.vy + gravity * meteor.gravityScale

  local density = airDensity * exp(-max(0, meteor.y) / scaleHeight)
  local speed = length3(meteor.vx, meteor.vy, meteor.vz)
  local pressure = 0.5 * density * speed * speed

  local heatTarget = clamp(pressure / HEAT_REFERENCE, 0, 1)
  if meteor.kind == "comet" then
    heatTarget = heatTarget * COMET_HEAT_SCALE
  end
  local blend = heatTarget > meteor.heat and 0.30 or 0.05
  meteor.heat = meteor.heat + (heatTarget - meteor.heat) * blend

  if speed > 1e-4 then
    local accel = 0.47 * density * meteor.size * meteor.size * speed * speed / meteor.mass
    local shed = min(accel / speed, 0.4)
    meteor.vx = meteor.vx - meteor.vx * shed
    meteor.vy = meteor.vy - meteor.vy * shed
    meteor.vz = meteor.vz - meteor.vz * shed
  end

  meteor.fragCooldown = meteor.fragCooldown - 1
  local fragmentThreshold = strength * FRAGMENT_PRESSURE
  if pressure > fragmentThreshold and meteor.fragCooldown <= 0 and meteor.size > minSize * 2 then
    nextId = physics.fragment(meteor, pressure / fragmentThreshold, nextId)
  end

  meteor.x = meteor.x + meteor.vx
  meteor.y = meteor.y + meteor.vy
  meteor.z = meteor.z + meteor.vz
  meteor.spinX = meteor.spinX + meteor.spinRateX
  meteor.spinY = meteor.spinY + meteor.spinRateY
  meteor.spinZ = meteor.spinZ + meteor.spinRateZ
  meteor.age = meteor.age + 1

  pushTrail(meteor)
  sampleGround(meteor)

  if meteor.y <= meteor.groundY then
    meteor.y = meteor.groundY
    meteor.alive = false
    return "impact", nextId
  end
  if meteor.age > MAX_AGE_TICKS then
    meteor.alive = false
    return "expired", nextId
  end
  if not meteor.alive then
    return "burnout", nextId
  end
  return nil, nextId
end

function physics.spawn(kind, id, camX, camY, camZ, yawDeg, pitchDeg)
  local yaw = yawDeg * DEG
  local pitch = pitchDeg * DEG
  local pitchCos = cos(pitch)
  local forwardX = -sin(yaw) * pitchCos
  local forwardZ = cos(yaw) * pitchCos
  local rightX, rightZ = cos(yaw), sin(yaw)

  local comet = kind == "comet"
  local entryRange = comet and 190 or 160
  local exitRange = comet and 200 or 140
  local lateral = (random() * 2 - 1) * (comet and 95 or 80)
  local altitude = camY + (comet and 78 or 46) + random() * 26

  local x = camX + forwardX * entryRange + rightX * lateral
  local z = camZ + forwardZ * entryRange + rightZ * lateral
  local y = altitude

  local targetX = camX - forwardX * exitRange - rightX * lateral * 0.85
  local targetZ = camZ - forwardZ * exitRange - rightZ * lateral * 0.85
  local targetY = camY - (comet and 2 or 6)

  local dx, dy, dz = normalize3(targetX - x, targetY - y, targetZ - z)
  local speed = comet and (0.75 + random() * 0.35) or (2.0 + random() * 0.8)
  local size = comet and (1.6 + random() * 1.3) or (0.4 + random() * 0.8)

  return newMeteor(id, kind, random(0, 5), x, y, z,
    dx * speed, dy * speed, dz * speed, size)
end

return physics
