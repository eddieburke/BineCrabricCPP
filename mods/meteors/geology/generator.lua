-- Meteors Geology: Procedural mesh generation and impact site creation
local config = require("meteors.config")
local geology = {}

local TAU = config.tau
local MAX_RENDER_VERTICES = config.max_render_vertices

local modelCache = {}
local modelErrors = {}

-- Hash function for deterministic randomness
local function hash01(a, b, c, d)
  local n = math.sin((a or 0) * 12.9898 + (b or 0) * 78.233 +
    (c or 0) * 37.719 + (d or 0) * 19.913) * 43758.5453123
  return n - math.floor(n)
end

local function clamp(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

local function smoothstep(a, b, x)
  if a == b then return x >= b and 1 or 0 end
  local t = clamp((x - a) / (b - a), 0, 1)
  return t * t * (3 - 2 * t)
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

local function cross(ax, ay, az, bx, by, bz)
  return ay * bz - az * by,
         az * bx - ax * bz,
         ax * by - ay * bx
end

local function randomUnit(seed, channel)
  local u = hash01(seed, channel, 1, 0)
  local v = hash01(seed, channel, 2, 0)
  local z = u * 2 - 1
  local a = v * TAU
  local r = math.sqrt(math.max(0, 1 - z * z))
  return r * math.cos(a), z, r * math.sin(a)
end

-- Build shape parameters for procedural geology
local function buildShapeParameters(kind, variant)
  local seed = variant * 101 + (kind == "comet" and 7001 or 1901)
  local params = {
    seed = seed,
    phase = hash01(seed, 1, 0, 0) * TAU,
    axisX = 0.84 + hash01(seed, 2, 0, 0) * 0.34,
    axisY = 0.82 + hash01(seed, 3, 0, 0) * 0.36,
    axisZ = 0.84 + hash01(seed, 4, 0, 0) * 0.34,
    craters = {},
    fractures = {},
  }

  if kind == "comet" then
    params.axisX = params.axisX * 1.15
    params.axisY = params.axisY * 0.88
    params.axisZ = params.axisZ * 0.97
  end

  local craterCount = kind == "comet" and 4 or (4 + math.floor(hash01(seed, 5, 0, 0) * 3))
  local i
  for i = 1, craterCount do
    local cx, cy, cz = randomUnit(seed, 20 + i)
    local radius = 0.22 + hash01(seed, i, 31, 0) * 0.30
    local depth = (kind == "comet" and 0.035 or 0.025) + hash01(seed, i, 32, 0) * 0.065
    params.craters[#params.craters + 1] = {
      x = cx, y = cy, z = cz,
      cosRadius = math.cos(radius),
      depth = depth,
      rim = depth * (0.22 + hash01(seed, i, 33, 0) * 0.28),
    }
  end

  for i = 1, 3 do
    local nx, ny, nz = randomUnit(seed, 60 + i)
    local gx, gy, gz = randomUnit(seed, 80 + i)
    params.fractures[#params.fractures + 1] = {
      x = nx, y = ny, z = nz,
      gateX = gx, gateY = gy, gateZ = gz,
      offset = (hash01(seed, i, 61, 0) - 0.5) * 0.34,
      width = 0.018 + hash01(seed, i, 62, 0) * 0.035,
      depth = 0.012 + hash01(seed, i, 63, 0) * 0.028,
      gate = -0.42 + hash01(seed, i, 64, 0) * 0.34,
    }
  end

  params.cutX, params.cutY, params.cutZ = randomUnit(seed, 99)
  params.cutThreshold = 0.64 + hash01(seed, 100, 0, 0) * 0.18
  params.cutDepth = 0.035 + hash01(seed, 101, 0, 0) * 0.065
  return params
end

local function cubeDirection(face, s, t)
  local x, y, z
  if face == 1 then x, y, z = 1, t, -s
  elseif face == 2 then x, y, z = -1, t, s
  elseif face == 3 then x, y, z = s, 1, -t
  elseif face == 4 then x, y, z = s, -1, t
  elseif face == 5 then x, y, z = s, t, 1
  else x, y, z = -s, t, -1 end
  return normalize3(x, y, z)
end

local function geologyPoint(params, dx, dy, dz)
  local phase = params.phase
  local n1 = math.sin(dx * 3.7 + dy * 4.9 - dz * 2.8 + phase)
  local n2 = math.sin((dx - dy) * 8.3 + dz * 6.7 + phase * 1.71)
  local n3 = math.sin(dx * 17.1 + dy * 13.7 + dz * 11.3 + phase * 2.39)
  local ridge = math.abs(math.sin(dx * 6.1 - dy * 5.4 + dz * 7.7 + phase * 0.73)) - 0.5
  local radius = 0.5 * (1 + n1 * 0.105 + n2 * 0.044 + n3 * 0.018 + ridge * 0.028)
  local depression = 0
  local rimHighlight = 0
  local i

  for i = 1, #params.craters do
    local c = params.craters[i]
    local d = dot(dx, dy, dz, c.x, c.y, c.z)
    if d > c.cosRadius then
      local u = clamp((d - c.cosRadius) / (1 - c.cosRadius), 0, 1)
      local bowl = smoothstep(0, 1, u)
      radius = radius - c.depth * bowl
      depression = depression + bowl
      local ring = math.exp(-((u - 0.16) * (u - 0.16)) / 0.010)
      radius = radius + c.rim * ring
      rimHighlight = rimHighlight + ring
    end
  end

  for i = 1, #params.fractures do
    local f = params.fractures[i]
    local gate = dot(dx, dy, dz, f.gateX, f.gateY, f.gateZ)
    if gate > f.gate then
      local distance = math.abs(dot(dx, dy, dz, f.x, f.y, f.z) - f.offset)
      if distance < f.width then
        local u = 1 - distance / f.width
        radius = radius - f.depth * u * u
        depression = depression + u * 0.65
      end
    end
  end

  local cutDot = dot(dx, dy, dz, params.cutX, params.cutY, params.cutZ)
  if cutDot > params.cutThreshold then
    local u = (cutDot - params.cutThreshold) / (1 - params.cutThreshold)
    radius = radius - params.cutDepth * math.pow(u, 1.35)
    depression = depression + u * 0.7
  end

  local x = dx * radius * params.axisX
  local y = dy * radius * params.axisY
  local z = dz * radius * params.axisZ
  return {
    x = x, y = y, z = z,
    dx = dx, dy = dy, dz = dz,
    depression = depression,
    rim = rimHighlight,
    grain = n2 * 0.55 + n3 * 0.30 + ridge * 0.35,
  }
end

local function paletteFor(kind, variant)
  if kind == "comet" then
    return 0.20, 0.23, 0.24, "dirty ice"
  end
  local composition = variant % 3
  if composition == 0 then
    return 0.17, 0.145, 0.12, "carbonaceous"
  elseif composition == 1 then
    return 0.34, 0.285, 0.225, "stony"
  end
  return 0.30, 0.29, 0.275, "iron"
end

local function meshSubdivisions(quality)
  quality = math.floor(clamp(quality, 1, 3))
  return 7 + quality * 2
end

function geology.buildModel(kind, variant, thermal)
  local setting = function(name, fallback)
    local value = minecraft.settings.get(config.settings[name].key)
    if value == nil then return fallback end
    return value
  end
  
  local quality = math.floor(setting("visual_quality", 3))
  local subdivisions = meshSubdivisions(quality)
  local key = table.concat({ config.model_version, kind, variant, subdivisions, thermal and "hot" or "body" }, "_")
  if modelCache[key] ~= nil then return modelCache[key] end

  local params = buildShapeParameters(kind, variant)
  local baseR, baseG, baseB = paletteFor(kind, variant)
  local quads = {}
  local lightX, lightY, lightZ = normalize3(0.42, 0.82, -0.38)
  local face

  for face = 1, 6 do
    local points = {}
    local iy, ix
    for iy = 0, subdivisions do
      points[iy] = {}
      local t = -1 + (iy / subdivisions) * 2
      for ix = 0, subdivisions do
        local s = -1 + (ix / subdivisions) * 2
        local dx, dy, dz = cubeDirection(face, s, t)
        points[iy][ix] = geologyPoint(params, dx, dy, dz)
      end
    end

    for iy = 0, subdivisions - 1 do
      for ix = 0, subdivisions - 1 do
        local p00 = points[iy][ix]
        local p10 = points[iy][ix + 1]
        local p11 = points[iy + 1][ix + 1]
        local p01 = points[iy + 1][ix]
        local nx = p00.dx + p10.dx + p11.dx + p01.dx
        local ny = p00.dy + p10.dy + p11.dy + p01.dy
        local nz = p00.dz + p10.dz + p11.dz + p01.dz
        nx, ny, nz = normalize3(nx, ny, nz)
        local sunlight = math.max(0, dot(nx, ny, nz, lightX, lightY, lightZ))
        local shade = 0.40 + sunlight * 0.60
        local depression = (p00.depression + p10.depression + p11.depression + p01.depression) * 0.25
        local rim = (p00.rim + p10.rim + p11.rim + p01.rim) * 0.25
        local grain = (p00.grain + p10.grain + p11.grain + p01.grain) * 0.25
        local r, g, b, alpha

        if thermal then
          local heatVariation = clamp(0.78 + sunlight * 0.25 + grain * 0.08 - depression * 0.06, 0.55, 1)
          r = 1.0
          g = 0.20 + heatVariation * 0.54
          b = 0.025 + heatVariation * 0.13
          shade = 1.0
          alpha = 1.0
        else
          local mineral = clamp(0.92 + grain * 0.10 - depression * 0.17 + rim * 0.10, 0.60, 1.18)
          r = clamp(baseR * mineral, 0, 1)
          g = clamp(baseG * mineral, 0, 1)
          b = clamp(baseB * mineral, 0, 1)
          alpha = 1.0
        end

        quads[#quads + 1] = {
          r = r, g = g, b = b, a = alpha, shade = shade,
          vertices = {
            { x = p00.x, y = p00.y, z = p00.z, u = 0, v = 0 },
            { x = p10.x, y = p10.y, z = p10.z, u = 1, v = 0 },
            { x = p11.x, y = p11.y, z = p11.z, u = 1, v = 1 },
            { x = p01.x, y = p01.y, z = p01.z, u = 0, v = 1 },
          },
        }
      end
    end
  end

  local ok, handleOrError, second = pcall(minecraft.model.build, { quads = quads, key = key })
  local handle, err
  if ok then
    handle = handleOrError
    err = second
  else
    err = handleOrError
  end

  if handle then
    modelCache[key] = handle
    return handle
  end

  if not modelErrors[key] then
    modelErrors[key] = true
    minecraft.log("error", "meteor model build failed for " .. key .. ": " .. tostring(err))
  end
  return nil
end

-- Impact site generation (ore placement, crater deformation)
function geology.createImpactSite(x, y, z, size, kind)
  local impactRadius = size * 3
  local craterDepth = size * 1.5
  local oreCount = math.floor(size * 8)
  
  -- This would interface with the world generation system
  -- For now, just log the impact
  minecraft.log("info", string.format("Meteor impact at %.1f, %.1f, %.1f (radius: %.1f)", x, y, z, impactRadius))
  
  return {
    x = x, y = y, z = z,
    radius = impactRadius,
    depth = craterDepth,
    oreCount = oreCount,
    kind = kind,
  }
end

return geology
