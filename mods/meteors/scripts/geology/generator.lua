local mathx = require("lib.mathx")

local generator = {}

local TAU = mathx.tau
local clamp, mix, smoothstep = mathx.clamp, mathx.mix, mathx.smoothstep
local dot, cross, normalize3 = mathx.dot, mathx.cross, mathx.normalize3
local hash01, randomUnit = mathx.hash01, mathx.randomUnit
local sin, cos, abs, exp, floor = math.sin, math.cos, math.abs, math.exp, math.floor

generator.heat_steps = 5

local EMBER = {
  { 0.58, 0.09, 0.02 },
  { 1.00, 0.30, 0.05 },
  { 1.00, 0.62, 0.17 },
  { 1.00, 0.88, 0.58 },
  { 1.00, 0.97, 0.90 },
}

local cache = {}
local cacheQuality = -1

local function shapeFor(kind, variant)
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

  local craterCount = kind == "comet" and 4 or (4 + floor(hash01(seed, 5, 0, 0) * 3))
  for i = 1, craterCount do
    local cx, cy, cz = randomUnit(seed, 20 + i)
    local radius = 0.22 + hash01(seed, i, 31, 0) * 0.30
    local depth = (kind == "comet" and 0.035 or 0.025) + hash01(seed, i, 32, 0) * 0.065
    params.craters[#params.craters + 1] = {
      x = cx, y = cy, z = cz,
      cosRadius = cos(radius),
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

local function surfacePoint(params, dx, dy, dz)
  local phase = params.phase
  local n1 = sin(dx * 3.7 + dy * 4.9 - dz * 2.8 + phase)
  local n2 = sin((dx - dy) * 8.3 + dz * 6.7 + phase * 1.71)
  local n3 = sin(dx * 17.1 + dy * 13.7 + dz * 11.3 + phase * 2.39)
  local ridge = abs(sin(dx * 6.1 - dy * 5.4 + dz * 7.7 + phase * 0.73)) - 0.5
  local radius = 0.5 * (1 + n1 * 0.105 + n2 * 0.044 + n3 * 0.018 + ridge * 0.028)
  local depression = 0
  local rimHighlight = 0

  for i = 1, #params.craters do
    local c = params.craters[i]
    local d = dot(dx, dy, dz, c.x, c.y, c.z)
    if d > c.cosRadius then
      local u = clamp((d - c.cosRadius) / (1 - c.cosRadius), 0, 1)
      local bowl = smoothstep(0, 1, u)
      radius = radius - c.depth * bowl
      depression = depression + bowl
      local ring = exp(-((u - 0.16) * (u - 0.16)) / 0.010)
      radius = radius + c.rim * ring
      rimHighlight = rimHighlight + ring
    end
  end

  for i = 1, #params.fractures do
    local f = params.fractures[i]
    if dot(dx, dy, dz, f.gateX, f.gateY, f.gateZ) > f.gate then
      local distance = abs(dot(dx, dy, dz, f.x, f.y, f.z) - f.offset)
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
    radius = radius - params.cutDepth * u ^ 1.35
    depression = depression + u * 0.7
  end

  return {
    x = dx * radius * params.axisX,
    y = dy * radius * params.axisY,
    z = dz * radius * params.axisZ,
    dx = dx, dy = dy, dz = dz,
    depression = depression,
    rim = rimHighlight,
    grain = n2 * 0.55 + n3 * 0.30 + ridge * 0.35,
  }
end

local function paletteFor(kind, variant)
  if kind == "comet" then
    return 0.42, 0.50, 0.56
  end
  local composition = variant % 3
  if composition == 0 then
    return 0.24, 0.20, 0.17
  elseif composition == 1 then
    return 0.44, 0.37, 0.29
  end
  return 0.40, 0.39, 0.37
end

function generator.ember(heat)
  local t = clamp(heat, 0, 1) * (#EMBER - 1)
  local i = floor(t)
  local f = t - i
  local a = EMBER[i + 1]
  local b = EMBER[i + 2] or a
  return mix(a[1], b[1], f), mix(a[2], b[2], f), mix(a[3], b[3], f)
end

local function push(packed, n, p, r, g, b)
  packed[n + 1] = p.x
  packed[n + 2] = p.y
  packed[n + 3] = p.z
  packed[n + 4] = 0
  packed[n + 5] = 0
  packed[n + 6] = r
  packed[n + 7] = g
  packed[n + 8] = b
  packed[n + 9] = 1
  return n + 9
end

function generator.body(kind, variant, quality, heatStep)
  if quality ~= cacheQuality then
    cache = {}
    cacheQuality = quality
  end
  local key = kind .. variant .. "h" .. heatStep
  local cached = cache[key]
  if cached then return cached end

  local subdivisions = 1 + quality * 2
  local params = shapeFor(kind, variant)
  local baseR, baseG, baseB = paletteFor(kind, variant)
  local heat = heatStep / (generator.heat_steps - 1)
  local emberR, emberG, emberB = generator.ember(heat)
  local glow = heat * heat
  local packed = {}
  local n = 0
  local grid = {}

  for face = 1, 6 do
    for iy = 0, subdivisions do
      local row = grid[iy]
      if row == nil then
        row = {}
        grid[iy] = row
      end
      local t = -1 + (iy / subdivisions) * 2
      for ix = 0, subdivisions do
        local s = -1 + (ix / subdivisions) * 2
        local dx, dy, dz = cubeDirection(face, s, t)
        row[ix] = surfacePoint(params, dx, dy, dz)
      end
    end

    for iy = 0, subdivisions - 1 do
      for ix = 0, subdivisions - 1 do
        local p00 = grid[iy][ix]
        local p10 = grid[iy][ix + 1]
        local p11 = grid[iy + 1][ix + 1]
        local p01 = grid[iy + 1][ix]
        local ox, oy, oz = normalize3(p00.dx + p10.dx + p11.dx + p01.dx,
                                      p00.dy + p10.dy + p11.dy + p01.dy,
                                      p00.dz + p10.dz + p11.dz + p01.dz)
        local nx, ny, nz = cross(p10.x - p00.x, p10.y - p00.y, p10.z - p00.z,
                                 p01.x - p00.x, p01.y - p00.y, p01.z - p00.z)

        local depression = (p00.depression + p10.depression + p11.depression + p01.depression) * 0.25
        local rim = (p00.rim + p10.rim + p11.rim + p01.rim) * 0.25
        local grain = (p00.grain + p10.grain + p11.grain + p01.grain) * 0.25

        local ambient = 0.34 + 0.46 * (oy * 0.5 + 0.5)
        local relief = clamp(1 + grain * 0.12 - depression * 0.22 + rim * 0.14, 0.62, 1.25)
        local r = clamp(baseR * relief * ambient, 0, 1)
        local g = clamp(baseG * relief * ambient, 0, 1)
        local b = clamp(baseB * relief * ambient, 0, 1)

        if glow > 0 then
          local flicker = clamp(0.84 + grain * 0.12 + rim * 0.12 - depression * 0.10, 0.68, 1.10)
          r = clamp(mix(r, emberR * flicker, glow), 0, 1)
          g = clamp(mix(g, emberG * flicker, glow), 0, 1)
          b = clamp(mix(b, emberB * flicker, glow), 0, 1)
        end

        if dot(nx, ny, nz, ox, oy, oz) < 0 then
          n = push(packed, n, p01, r, g, b)
          n = push(packed, n, p11, r, g, b)
          n = push(packed, n, p10, r, g, b)
          n = push(packed, n, p00, r, g, b)
        else
          n = push(packed, n, p00, r, g, b)
          n = push(packed, n, p10, r, g, b)
          n = push(packed, n, p11, r, g, b)
          n = push(packed, n, p01, r, g, b)
        end
      end
    end
  end

  cache[key] = packed
  return packed
end

return generator
