-- Meteors Physics: Trajectory and collision calculations
local config = require("meteors.config")
local physics = {}

local TAU = config.tau

-- Utility functions (should eventually move to lib.math_util)
local function clamp(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

local function length3(x, y, z)
  return math.sqrt(x * x + y * y + z * z)
end

local function normalize3(x, y, z)
  local len = length3(x, y, z)
  if len < 0.000001 then return 0, 1, 0 end
  return x / len, y / len, z / len
end

local function dot(ax, ay, az, bx, by, bz)
  return ax * bx + ay * by + az * bz
end

local function randomUnit(seed, channel)
  local hash01 = function(a, b, c, d)
    local n = math.sin((a or 0) * 12.9898 + (b or 0) * 78.233 +
      (c or 0) * 37.719 + (d or 0) * 19.913) * 43758.5453123
    return n - math.floor(n)
  end
  local u = hash01(seed, channel, 1, 0)
  local v = hash01(seed, channel, 2, 0)
  local z = u * 2 - 1
  local a = v * TAU
  local r = math.sqrt(math.max(0, 1 - z * z))
  return r * math.cos(a), z, r * math.sin(a)
end

-- Meteor state
local function createMeteor(id, kind, variant, x, y, z, vx, vy, vz, size)
  return {
    id = id,
    kind = kind, -- "meteor" or "comet"
    variant = variant,
    x = x, y = y, z = z,
    vx = vx, vy = vy, vz = vz,
    size = size,
    mass = size * size * size * 2.5,
    alive = true,
    fragments = {},
    trail = {},
    thermal = false,
    age = 0,
  }
end

-- Physics update step
function physics.update(meteor, dt, cameraY)
  if not meteor.alive then return end
  
  local gravity = config.get("gravity") or -0.04
  local air_density = config.get("air_density") or 0.0012
  local air_scale_height = config.get("air_scale_height") or 25.0
  local stone_strength = config.get("stone_strength") or 8.0
  local fragment_min_size = config.get("fragment_min_size") or 0.15
  
  -- Apply gravity
  meteor.vy = meteor.vy + gravity * dt
  
  -- Apply atmospheric drag
  local altitude = math.max(0, meteor.y)
  local density = air_density * math.exp(-altitude / air_scale_height)
  local speed = length3(meteor.vx, meteor.vy, meteor.vz)
  
  if speed > 0.01 then
    local dragCoeff = 0.47 * density * meteor.size * meteor.size
    local dragForce = dragCoeff * speed * speed
    local dragX = -dragForce * (meteor.vx / speed) * dt / meteor.mass
    local dragY = -dragForce * (meteor.vy / speed) * dt / meteor.mass
    local dragZ = -dragForce * (meteor.vz / speed) * dt / meteor.mass
    
    meteor.vx = meteor.vx + dragX
    meteor.vy = meteor.vy + dragY
    meteor.vz = meteor.vz + dragZ
    
    -- Check for fragmentation due to aerodynamic stress
    local dynamicPressure = 0.5 * density * speed * speed
    if dynamicPressure > stone_strength and meteor.size > fragment_min_size then
      physics.fragmentate(meteor, dynamicPressure / stone_strength)
    end
    
    -- Check for thermal glow (re-entry heating)
    if speed > 0.8 and altitude < 40 then
      meteor.thermal = true
    end
  end
  
  -- Update position
  meteor.x = meteor.x + meteor.vx * dt
  meteor.y = meteor.y + meteor.vy * dt
  meteor.z = meteor.z + meteor.vz * dt
  
  -- Update trail
  if #meteor.trail > (config.get("trail_length") or 54) then
    table.remove(meteor.trail, 1)
  end
  meteor.trail[#meteor.trail + 1] = { x = meteor.x, y = meteor.y, z = meteor.z }
  
  -- Ground collision
  if meteor.y < 0 then
    meteor.alive = false
    return "impact"
  end
  
  meteor.age = meteor.age + dt
  return nil
end

-- Fragmentation logic
function physics.fragmentate(parent, stressFactor)
  local fragment_min_size = config.get("fragment_min_size") or 0.15
  
  if parent.size < fragment_min_size * 2 then return end
  
  local numFragments = math.min(5, math.floor(stressFactor))
  local i
  for i = 1, numFragments do
    local fragmentSize = parent.size * (0.3 + math.random() * 0.4)
    if fragmentSize < fragment_min_size then break end
    
    local spreadX, spreadY, spreadZ = randomUnit(parent.id * 100 + i, i)
    local fragment = createMeteor(
      parent.id * 1000 + i,
      parent.kind,
      parent.variant,
      parent.x, parent.y, parent.z,
      parent.vx + spreadX * 0.15,
      parent.vy + spreadY * 0.15,
      parent.vz + spreadZ * 0.15,
      fragmentSize
    )
    fragment.thermal = parent.thermal
    fragment.trail = {}
    parent.fragments[#parent.fragments + 1] = fragment
  end
  
  -- Reduce parent size
  parent.size = parent.size * 0.6
  if parent.size < fragment_min_size then
    parent.alive = false
  end
end

-- Spawn helper
function physics.spawn(kind, cameraX, cameraY, cameraZ, yaw, pitch)
  local max_meteors = config.get("max_meteors") or 50
  local spawnDistance = 80 + math.random() * 40
  
  -- Calculate spawn position in front of camera
  local horizontalDist = spawnDistance * math.cos(pitch)
  local x = cameraX + horizontalDist * math.sin(yaw)
  local y = cameraY + spawnDistance * math.sin(pitch) + 20
  local z = cameraZ + horizontalDist * math.cos(yaw)
  
  -- Initial velocity (downward and toward camera area)
  local speed = 0.6 + math.random() * 0.4
  local vx = -math.sin(yaw) * speed * 0.3
  local vy = -0.3 - math.random() * 0.2
  local vz = -math.cos(yaw) * speed * 0.3
  
  local size = 0.4 + math.random() * 0.8
  local variant = math.random(0, 5)
  
  return createMeteor(nextId or 1, kind, variant, x, y, z, vx, vy, vz, size)
end

return physics
