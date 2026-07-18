-- Meteors: Stunning procedural meteor and comet renderer
-- Lua 5.1 / 5.4 compatible. Uses cached faceted geology meshes, smooth
-- interpolation, layered atmospheric ribbons, comet coma/tails, fragmentation,
-- and impact flashes without external textures.

local meteors = {}
local effects = {}
local modelCache = {}
local modelErrors = {}
local nextId = 1
local spawnCooldown = 0
local cometTimer = 0
local nextCometTick = 3600
local renderClock = 0
local lastCamera = { x = 0, y = 70, z = 0, yaw = 0, pitch = 0, valid = false }
local SCREEN_ID = "meteors:controls"
local MODEL_VERSION = "geo_stunning_4"
local MAX_RENDER_VERTICES = 48000
local TAU = math.pi * 2

minecraft.settings.register("Meteors", {
  { key = "gravity", label = "Gravity", kind = "slider", min = -0.12, max = -0.01, step = 0.005, decimals = 3, default = -0.04 },
  { key = "air_density", label = "Air Density", kind = "slider", min = 0.0002, max = 0.004, step = 0.0001, decimals = 4, default = 0.0012 },
  { key = "air_scale_height", label = "Air Scale Height", kind = "slider", min = 10, max = 50, step = 1, default = 25.0 },
  { key = "stone_strength", label = "Fragmentation Strength", kind = "slider", min = 2, max = 20, step = 0.5, default = 8.0 },
  { key = "fragment_min_size", label = "Minimum Fragment Size", kind = "slider", min = 0.05, max = 0.5, step = 0.01, decimals = 2, default = 0.15 },
  { key = "trail_length", label = "Trail Length", kind = "slider", min = 12, max = 96, integer = true, default = 54 },
  { key = "visual_quality", label = "Visual Quality", kind = "slider", min = 1, max = 3, integer = true, default = 3 },
  { key = "effect_density", label = "Effect Density", kind = "slider", min = 1, max = 3, integer = true, default = 2 },
  { key = "max_meteors", label = "Max Meteors", kind = "slider", min = 10, max = 150, integer = true, default = 50 },
})

minecraft.keybinds.register("spawn_meteor", { default = minecraft.key_code("m"), label = "Spawn Meteor" })
minecraft.keybinds.register("spawn_comet", { default = minecraft.key_code("n"), label = "Spawn Comet" })

local function setting(name, fallback)
  local value = minecraft.settings.get(name)
  if value == nil then return fallback end
  return value
end

local function clamp(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

local function lerp(a, b, t)
  return a + (b - a) * t
end

local function smoothstep(a, b, x)
  if a == b then return x >= b and 1 or 0 end
  local t = clamp((x - a) / (b - a), 0, 1)
  return t * t * (3 - 2 * t)
end

local function hash01(a, b, c, d)
  local n = math.sin((a or 0) * 12.9898 + (b or 0) * 78.233 +
    (c or 0) * 37.719 + (d or 0) * 19.913) * 43758.5453123
  return n - math.floor(n)
end

local function length3(x, y, z)
  return math.sqrt(x * x + y * y + z * z)
end

local function normalize3(x, y, z)
  local len = length3(x, y, z)
  if len < 0.000001 then return 0, 1, 0 end
  return x / len, y / len, z / len
end

local function cross(ax, ay, az, bx, by, bz)
  return ay * bz - az * by,
         az * bx - ax * bz,
         ax * by - ay * bx
end

local function dot(ax, ay, az, bx, by, bz)
  return ax * bx + ay * by + az * bz
end

local function randomUnit(seed, channel)
  local u = hash01(seed, channel, 1, 0)
  local v = hash01(seed, channel, 2, 0)
  local z = u * 2 - 1
  local a = v * TAU
  local r = math.sqrt(math.max(0, 1 - z * z))
  return r * math.cos(a), z, r * math.sin(a)
end

local function colorVertex(x, y, z, r, g, b, a)
  return { x = x, y = y, z = z, r = r, g = g, b = b, a = a }
end

local function canAppend(vertices, count)
  return #vertices + count <= MAX_RENDER_VERTICES
end

local function appendQuad(vertices, a, b, c, d)
  if not canAppend(vertices, 4) then return false end
  vertices[#vertices + 1] = a
  vertices[#vertices + 1] = b
  vertices[#vertices + 1] = c
  vertices[#vertices + 1] = d
  return true
end

-- ---------------------------------------------------------------------------
-- Procedural geology mesh
-- ---------------------------------------------------------------------------

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

local function meshSubdivisions(lod, quality)
  quality = math.floor(clamp(quality, 1, 3))
  if lod == 1 then return 7 + quality * 2 end
  if lod == 2 then return 4 + quality end
  return 3
end

local function buildMeteorModel(kind, variant, lod, thermal)
  local quality = math.floor(setting("visual_quality", 3))
  local subdivisions = meshSubdivisions(lod, quality)
  local key = table.concat({ MODEL_VERSION, kind, variant, lod, subdivisions, thermal and "hot" or "body" }, "_")
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

local function modelLOD(distance)
  local quality = math.floor(setting("visual_quality", 3))
  if distance < 56 + quality * 18 then return 1 end
  if distance < 190 + quality * 50 then return 2 end
  return 3
end

-- ---------------------------------------------------------------------------
-- Rendering helpers
-- ---------------------------------------------------------------------------

local function cameraBasis(cameraX, cameraY, cameraZ, px, py, pz)
  local vx, vy, vz = normalize3(cameraX - px, cameraY - py, cameraZ - pz)
  local rx, ry, rz = cross(0, 1, 0, vx, vy, vz)
  if length3(rx, ry, rz) < 0.001 then
    rx, ry, rz = cross(0, 0, 1, vx, vy, vz)
  end
  rx, ry, rz = normalize3(rx, ry, rz)
  local ux, uy, uz = cross(vx, vy, vz, rx, ry, rz)
  ux, uy, uz = normalize3(ux, uy, uz)
  return rx, ry, rz, ux, uy, uz
end

local function appendGlowFan(vertices, camera, px, py, pz, radius, r, g, b, alpha, sectors)
  if alpha <= 0.001 or radius <= 0.001 then return end
  local rx, ry, rz, ux, uy, uz = cameraBasis(camera.x, camera.y, camera.z, px, py, pz)
  local i
  for i = 0, sectors - 1 do
    if not canAppend(vertices, 4) then return end
    local a0 = TAU * i / sectors
    local a1 = TAU * (i + 1) / sectors
    local c0, s0 = math.cos(a0), math.sin(a0)
    local c1, s1 = math.cos(a1), math.sin(a1)
    local x0 = px + (rx * c0 + ux * s0) * radius
    local y0 = py + (ry * c0 + uy * s0) * radius
    local z0 = pz + (rz * c0 + uz * s0) * radius
    local x1 = px + (rx * c1 + ux * s1) * radius
    local y1 = py + (ry * c1 + uy * s1) * radius
    local z1 = pz + (rz * c1 + uz * s1) * radius
    appendQuad(vertices,
      colorVertex(px, py, pz, r, g, b, alpha),
      colorVertex(x0, y0, z0, r, g, b, 0),
      colorVertex(x1, y1, z1, r, g, b, 0),
      colorVertex(px, py, pz, r, g, b, alpha))
  end
end

local function appendRing(vertices, camera, px, py, pz, innerRadius, outerRadius, r, g, b, alpha, sectors)
  if alpha <= 0.001 then return end
  local rx, ry, rz, ux, uy, uz = cameraBasis(camera.x, camera.y, camera.z, px, py, pz)
  local i
  for i = 0, sectors - 1 do
    if not canAppend(vertices, 4) then return end
    local a0 = TAU * i / sectors
    local a1 = TAU * (i + 1) / sectors
    local c0, s0 = math.cos(a0), math.sin(a0)
    local c1, s1 = math.cos(a1), math.sin(a1)
    local ix0 = px + (rx * c0 + ux * s0) * innerRadius
    local iy0 = py + (ry * c0 + uy * s0) * innerRadius
    local iz0 = pz + (rz * c0 + uz * s0) * innerRadius
    local ix1 = px + (rx * c1 + ux * s1) * innerRadius
    local iy1 = py + (ry * c1 + uy * s1) * innerRadius
    local iz1 = pz + (rz * c1 + uz * s1) * innerRadius
    local ox0 = px + (rx * c0 + ux * s0) * outerRadius
    local oy0 = py + (ry * c0 + uy * s0) * outerRadius
    local oz0 = pz + (rz * c0 + uz * s0) * outerRadius
    local ox1 = px + (rx * c1 + ux * s1) * outerRadius
    local oy1 = py + (ry * c1 + uy * s1) * outerRadius
    local oz1 = pz + (rz * c1 + uz * s1) * outerRadius
    appendQuad(vertices,
      colorVertex(ix0, iy0, iz0, r, g, b, alpha),
      colorVertex(ix1, iy1, iz1, r, g, b, alpha),
      colorVertex(ox1, oy1, oz1, r, g, b, 0),
      colorVertex(ox0, oy0, oz0, r, g, b, 0))
  end
end

local function ribbonSide(camera, ax, ay, az, bx, by, bz)
  local dx, dy, dz = normalize3(bx - ax, by - ay, bz - az)
  local mx, my, mz = (ax + bx) * 0.5, (ay + by) * 0.5, (az + bz) * 0.5
  local vx, vy, vz = normalize3(camera.x - mx, camera.y - my, camera.z - mz)
  local sx, sy, sz = cross(dx, dy, dz, vx, vy, vz)
  if length3(sx, sy, sz) < 0.001 then
    sx, sy, sz = cross(dx, dy, dz, 0, 1, 0)
  end
  return normalize3(sx, sy, sz)
end

local function appendRibbonSegment(vertices, camera, p1, p2, w1, w2,
    r1, g1, b1, a1, r2, g2, b2, a2)
  if not canAppend(vertices, 4) then return false end
  local sx, sy, sz = ribbonSide(camera, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z)
  return appendQuad(vertices,
    colorVertex(p1.x - sx * w1, p1.y - sy * w1, p1.z - sz * w1, r1, g1, b1, a1),
    colorVertex(p1.x + sx * w1, p1.y + sy * w1, p1.z + sz * w1, r1, g1, b1, a1),
    colorVertex(p2.x + sx * w2, p2.y + sy * w2, p2.z + sz * w2, r2, g2, b2, a2),
    colorVertex(p2.x - sx * w2, p2.y - sy * w2, p2.z - sz * w2, r2, g2, b2, a2))
end

local function renderPosition(m, tickDelta)
  local t = clamp(tickDelta or 0, 0, 1)
  return lerp(m.px, m.x, t), lerp(m.py, m.y, t), lerp(m.pz, m.z, t)
end

local function appendMeteorTrail(vertices, camera, m, hx, hy, hz, distance)
  local count = #m.trail
  if count < 2 or m.temperature < 0.08 then return end
  local quality = math.floor(setting("visual_quality", 3))
  local step = distance > 420 and 3 or (distance > 220 and 2 or 1)
  if quality == 1 and step < 2 then step = 2 end
  local maxVisible = math.min(count, math.floor(setting("trail_length", 54)))
  local start = math.max(1, count - maxVisible + 1)
  local i = start

  while i < count do
    local j = math.min(count, i + step)
    local p1 = m.trail[i]
    local p2
    if j >= count then p2 = { x = hx, y = hy, z = hz } else p2 = m.trail[j] end
    local t1 = (i - start) / math.max(1, count - start)
    local t2 = (j - start) / math.max(1, count - start)
    local heat = clamp(m.temperature, 0, 1)
    local outerW1 = m.size * (0.08 + 1.50 * t1) * (0.6 + heat * 0.8)
    local outerW2 = m.size * (0.08 + 1.50 * t2) * (0.6 + heat * 0.8)
    local coreW1 = outerW1 * 0.24
    local coreW2 = outerW2 * 0.24
    local whiteW1 = outerW1 * 0.075
    local whiteW2 = outerW2 * 0.075

    appendRibbonSegment(vertices, camera, p1, p2, outerW1, outerW2,
      1.0, 0.12, 0.015, 0.01 + 0.10 * t1 * heat,
      1.0, 0.22, 0.025, 0.03 + 0.24 * t2 * heat)
    appendRibbonSegment(vertices, camera, p1, p2, coreW1, coreW2,
      1.0, 0.42, 0.05, 0.01 + 0.18 * t1 * heat,
      1.0, 0.70, 0.18, 0.04 + 0.55 * t2 * heat)
    if distance < 320 or quality >= 3 then
      appendRibbonSegment(vertices, camera, p1, p2, whiteW1, whiteW2,
        1.0, 0.84, 0.48, 0.01 + 0.22 * t1 * heat,
        1.0, 0.98, 0.84, 0.06 + 0.80 * t2 * heat)
    end
    i = j
  end
end

local function makeCometTailPoints(m, hx, hy, hz, curved, segments)
  local vx, vy, vz = normalize3(m.vx, m.vy, m.vz)
  local bx, by, bz = -vx, -vy, -vz
  local sx, sy, sz = cross(vx, vy, vz, 0, 1, 0)
  if length3(sx, sy, sz) < 0.01 then sx, sy, sz = 1, 0, 0 end
  sx, sy, sz = normalize3(sx, sy, sz)
  local points = {}
  local spacing = 1.6 + m.size * 1.15
  local i
  for i = 0, segments do
    local d = i * spacing
    local curve = curved and (i * i * 0.018 * m.size) or 0
    local wave = math.sin(renderClock * 0.035 + i * 0.43 + m.seed) * (curved and 0.10 or 0.035) * i
    points[#points + 1] = {
      x = hx + bx * d + sx * (curve + wave),
      y = hy + by * d - curve * 0.18,
      z = hz + bz * d + sz * (curve + wave),
    }
  end
  return points
end

local function appendCometTail(vertices, camera, m, hx, hy, hz, distance)
  local quality = math.floor(setting("visual_quality", 3))
  local segments = quality == 1 and 14 or (quality == 2 and 20 or 28)
  if distance > 500 then segments = math.min(segments, 16) end
  local ion = makeCometTailPoints(m, hx, hy, hz, false, segments)
  local dust = makeCometTailPoints(m, hx, hy, hz, true, math.max(10, segments - 4))
  local i

  for i = #dust - 1, 1, -1 do
    local p1, p2 = dust[i + 1], dust[i]
    local t1 = 1 - i / #dust
    local t2 = 1 - (i - 1) / #dust
    local w1 = m.size * (0.35 + t1 * 1.55)
    local w2 = m.size * (0.35 + t2 * 1.55)
    appendRibbonSegment(vertices, camera, p1, p2, w1, w2,
      0.92, 0.66, 0.34, 0.012 + 0.055 * t1,
      1.0, 0.88, 0.62, 0.018 + 0.105 * t2)
  end

  for i = #ion - 1, 1, -1 do
    local p1, p2 = ion[i + 1], ion[i]
    local t1 = 1 - i / #ion
    local t2 = 1 - (i - 1) / #ion
    local w1 = m.size * (0.10 + t1 * 0.58)
    local w2 = m.size * (0.10 + t2 * 0.58)
    appendRibbonSegment(vertices, camera, p1, p2, w1, w2,
      0.20, 0.52, 1.0, 0.018 + 0.10 * t1,
      0.68, 0.88, 1.0, 0.035 + 0.26 * t2)
    if quality >= 2 then
      appendRibbonSegment(vertices, camera, p1, p2, w1 * 0.22, w2 * 0.22,
        0.75, 0.91, 1.0, 0.025 + 0.17 * t1,
        1.0, 1.0, 1.0, 0.05 + 0.50 * t2)
    end
  end

  local sectors = quality == 1 and 10 or (quality == 2 and 14 or 18)
  appendGlowFan(vertices, camera, hx, hy, hz, m.size * 5.5, 0.22, 0.58, 1.0, 0.075, sectors)
  appendGlowFan(vertices, camera, hx, hy, hz, m.size * 3.2, 0.54, 0.82, 1.0, 0.14, sectors)
  appendGlowFan(vertices, camera, hx, hy, hz, m.size * 1.65, 0.92, 0.98, 1.0, 0.31, sectors)
end

local function appendMeteorGlow(vertices, camera, m, hx, hy, hz, distance)
  if m.temperature < 0.10 then return end
  local quality = math.floor(setting("visual_quality", 3))
  local sectors = quality == 1 and 8 or (quality == 2 and 12 or 16)
  local heat = clamp(m.temperature, 0, 1)
  local distanceFade = clamp(1 - distance / 900, 0.20, 1)
  appendGlowFan(vertices, camera, hx, hy, hz,
    m.size * (2.4 + heat * 3.5), 1.0, 0.10, 0.01,
    heat * 0.10 * distanceFade, sectors)
  appendGlowFan(vertices, camera, hx, hy, hz,
    m.size * (1.3 + heat * 1.8), 1.0, 0.48, 0.06,
    heat * 0.19 * distanceFade, sectors)
  if heat > 0.65 and distance < 420 then
    appendRing(vertices, camera, hx, hy, hz,
      m.size * 1.05, m.size * (1.35 + heat * 0.25),
      1.0, 0.78, 0.28, (heat - 0.65) * 0.32, sectors)
  end
end

local function appendEffects(vertices, camera)
  local i
  for i = 1, #effects do
    local e = effects[i]
    local t = clamp(e.age / e.maxAge, 0, 1)
    if e.kind == "impact" then
      local pulse = 1 - t
      appendGlowFan(vertices, camera, e.x, e.y, e.z,
        e.size * (0.6 + t * 6.0), 1.0, 0.45, 0.05,
        pulse * pulse * 0.45, 16)
      appendRing(vertices, camera, e.x, e.y, e.z,
        e.size * (0.7 + t * 5.2), e.size * (0.9 + t * 6.3),
        1.0, 0.80, 0.30, pulse * 0.34, 16)
    elseif e.kind == "fragment" then
      appendGlowFan(vertices, camera, e.x, e.y, e.z,
        e.size * (0.8 + t * 3.0), 1.0, 0.72, 0.22,
        (1 - t) * 0.30, 12)
    end
  end
end

local function drawMeteorBody(m, x, y, z, distance)
  local lod = modelLOD(distance)
  local handle = buildMeteorModel(m.isComet and "comet" or "meteor", m.variant, lod, false)
  if handle then
    minecraft.model.draw(handle, {
      x = x, y = y, z = z,
      yaw = m.yaw, pitch = m.pitch, roll = m.roll,
      scale = m.size * 2,
      brightness = m.isComet and 0.88 or 1.0,
      a = 1.0,
      cull = false,
      blend = false,
      depth_test = true,
      depth_write = true,
    })
  end

  if not m.isComet and m.temperature > 0.10 then
    local hotHandle = buildMeteorModel("meteor", m.variant, lod, true)
    if hotHandle then
      local heat = clamp((m.temperature - 0.08) / 0.92, 0, 1)
      minecraft.model.draw(hotHandle, {
        x = x, y = y, z = z,
        yaw = m.yaw, pitch = m.pitch, roll = m.roll,
        scale = m.size * (2.015 + heat * 0.035),
        brightness = 1.0,
        a = 0.08 + heat * 0.70,
        cull = false,
        blend = true,
        depth_test = true,
        depth_write = false,
      })
    end
  end
end

-- ---------------------------------------------------------------------------
-- Simulation, fragmentation, impacts
-- ---------------------------------------------------------------------------

local function addEffect(kind, x, y, z, size, lifetime)
  effects[#effects + 1] = {
    kind = kind, x = x, y = y, z = z,
    size = size, age = 0, maxAge = lifetime,
  }
end

local function spawnMeteorAt(x, y, z, vx, vy, vz, size, isComet, options)
  if #meteors >= math.floor(setting("max_meteors", 50)) then return nil end
  options = options or {}
  local speed = length3(vx, vy, vz)
  local tumblingSpeed = isComet and 0.18 or (1.2 + speed * 0.42)
  local seed = options.seed or math.random(1, 999999)
  local meteor = {
    id = nextId,
    x = x, y = y, z = z,
    px = x, py = y, pz = z,
    vx = vx, vy = vy, vz = vz,
    yaw = hash01(seed, 1, 0, 0) * 360,
    pitch = hash01(seed, 2, 0, 0) * 360,
    roll = hash01(seed, 3, 0, 0) * 360,
    rotSpeed = {
      x = (hash01(seed, 4, 0, 0) - 0.5) * tumblingSpeed,
      y = (hash01(seed, 5, 0, 0) - 0.5) * tumblingSpeed,
      z = (hash01(seed, 6, 0, 0) - 0.5) * tumblingSpeed,
    },
    size = size,
    temperature = isComet and 0 or 0.02,
    age = 0,
    maxAge = options.maxAge or (isComet and 4200 or (420 + math.random() * 360)),
    seed = seed,
    variant = seed % 12,
    trail = {},
    isComet = isComet or false,
    isFragment = options.isFragment or false,
    fragmentDepth = options.fragmentDepth or 0,
    alive = true,
  }
  nextId = nextId + 1
  meteors[#meteors + 1] = meteor
  return meteor
end

local function visibleSpawnOrigin()
  local player = minecraft.world.player()
  if not player then return nil end
  return player
end

local function spawnMeteor()
  local player = visibleSpawnOrigin()
  if not player then return nil end
  local angle = math.random() * TAU
  local horizontalDistance = 82 + math.random() * 78
  local sx = player.x + math.cos(angle) * horizontalDistance
  local sz = player.z + math.sin(angle) * horizontalDistance
  local sy = 125 + math.random() * 58
  local targetX = player.x + (math.random() - 0.5) * 70
  local targetY = player.y + 8 + math.random() * 26
  local targetZ = player.z + (math.random() - 0.5) * 70
  local dx, dy, dz = normalize3(targetX - sx, targetY - sy, targetZ - sz)
  local speed = 4.4 + math.random() * 4.8
  local size = 0.34 + math.pow(math.random(), 1.8) * 1.18
  return spawnMeteorAt(sx, sy, sz, dx * speed, dy * speed, dz * speed, size, false)
end

local function spawnMeteorFromKeybind()
  if spawnCooldown > 0 then return end
  if spawnMeteor() then spawnCooldown = 5 end
end

local function spawnComet()
  local player = visibleSpawnOrigin()
  if not player then return nil end
  local angle = math.random() * TAU
  local distance = 155 + math.random() * 145
  local sx = player.x + math.cos(angle) * distance
  local sz = player.z + math.sin(angle) * distance
  local sy = 132 + math.random() * 62
  local tangent = angle + (math.random() < 0.5 and math.pi * 0.5 or -math.pi * 0.5)
  local speed = 0.55 + math.random() * 0.75
  local vx = math.cos(tangent) * speed
  local vz = math.sin(tangent) * speed
  local vy = -0.015 - math.random() * 0.035
  local size = 1.25 + math.random() * 2.15
  return spawnMeteorAt(sx, sy, sz, vx, vy, vz, size, true, { maxAge = 5200 })
end

local function spawnBurstParticles(m, x, y, z, impact)
  local density = math.floor(setting("effect_density", 2))
  local count = math.floor((impact and 13 or 7) * density + m.size * (impact and 9 or 5))
  count = math.min(count, impact and 55 or 32)
  local speed = length3(m.vx, m.vy, m.vz)
  local bx, by, bz = normalize3(-m.vx, -m.vy, -m.vz)
  local i
  for i = 1, count do
    local rx, ry, rz = randomUnit(m.seed + m.age * 17, i + (impact and 500 or 300))
    local scatter = (impact and 0.35 or 0.20) + math.random() * (impact and 1.25 or 0.65)
    local hot = i % 4 ~= 0
    minecraft.particles.spawn({
      x = x + rx * m.size * 0.4,
      y = y + ry * m.size * 0.4,
      z = z + rz * m.size * 0.4,
      vx = rx * scatter + bx * speed * 0.04,
      vy = ry * scatter + (impact and 0.22 or 0.05),
      vz = rz * scatter + bz * speed * 0.04,
      scale = hot and (0.35 + math.random() * 1.15) or (0.8 + math.random() * 1.6),
      r = hot and 1.0 or 0.19,
      g = hot and (0.34 + math.random() * 0.52) or 0.15,
      b = hot and 0.045 or 0.12,
      max_age = hot and (12 + math.random(18)) or (28 + math.random(36)),
      gravity = hot and 0.035 or -0.004,
    })
  end
end

local function impactMeteor(m, x, y, z)
  addEffect("impact", x, y, z, math.max(0.8, m.size), 22)
  spawnBurstParticles(m, x, y, z, true)
  m.alive = false
end

local function fragmentMeteor(m)
  addEffect("fragment", m.x, m.y, m.z, math.max(0.6, m.size), 14)
  spawnBurstParticles(m, m.x, m.y, m.z, false)
  local minSize = setting("fragment_min_size", 0.15)
  local count = 2 + math.floor(hash01(m.seed, m.age, 900, 0) * 3)
  local baseSize = m.size * math.pow(1 / count, 0.38)
  local i
  for i = 1, count do
    local rx, ry, rz = randomUnit(m.seed + m.age, 1000 + i)
    local fragmentSize = baseSize * (0.68 + hash01(m.seed, i, m.age, 7) * 0.48)
    if fragmentSize >= minSize then
      local spread = 0.10 + hash01(m.seed, i, 77, m.age) * 0.34
      spawnMeteorAt(m.x, m.y, m.z,
        m.vx + rx * spread,
        m.vy + ry * spread,
        m.vz + rz * spread,
        fragmentSize, false, {
          isFragment = true,
          fragmentDepth = m.fragmentDepth + 1,
          maxAge = math.max(100, m.maxAge - m.age),
          seed = m.seed + i * 7919,
        })
    end
  end
  m.alive = false
end

local function collidedAlongPath(m, oldX, oldY, oldZ, newX, newY, newZ)
  local dx, dy, dz = newX - oldX, newY - oldY, newZ - oldZ
  local distance = length3(dx, dy, dz)
  local steps = math.min(36, math.max(1, math.ceil(distance * 2.2)))
  local i
  for i = 1, steps do
    local t = i / steps
    local x = lerp(oldX, newX, t)
    local y = lerp(oldY, newY, t)
    local z = lerp(oldZ, newZ, t)
    local by = math.floor(y)
    if by >= 0 and by < 128 then
      local block = minecraft.world.get_block(math.floor(x + 0.5), by, math.floor(z + 0.5))
      if block ~= 0 then return true, x, y, z end
    end
  end
  return false
end

local function updateMeteorPhysics(m)
  m.age = m.age + 1
  m.px, m.py, m.pz = m.x, m.y, m.z

  local densityBase = setting("air_density", 0.0012)
  local scaleHeight = setting("air_scale_height", 25.0)
  local airDensity = densityBase * math.exp(clamp(-(m.y - 64) / scaleHeight, -8, 8))
  if m.y > 100 then
    airDensity = airDensity * math.exp(clamp(-(m.y - 100) / 31, -8, 0))
  end

  local speedSq = m.vx * m.vx + m.vy * m.vy + m.vz * m.vz
  local speed = math.sqrt(speedSq)
  if speed > 0.001 and not m.isComet then
    local crossSection = math.pi * m.size * m.size
    local volume = math.max(0.04, m.size * m.size * m.size)
    local dragForce = 0.5 * airDensity * speedSq * crossSection * 0.050 / volume
    local drag = math.min(speed * 0.35, dragForce / speed)
    local invSpeed = 1 / speed
    m.vx = m.vx - m.vx * invSpeed * drag
    m.vy = m.vy - m.vy * invSpeed * drag
    m.vz = m.vz - m.vz * invSpeed * drag
  end

  if not m.isComet then
    m.vy = m.vy + setting("gravity", -0.04)
    local normalizedDensity = airDensity / math.max(0.00001, densityBase)
    local targetHeat = clamp(normalizedDensity * speedSq * 0.040, 0, 1)
    m.temperature = m.temperature * 0.84 + targetHeat * 0.16
  end

  local newX = m.x + m.vx
  local newY = m.y + m.vy
  local newZ = m.z + m.vz
  local hit, hitX, hitY, hitZ = collidedAlongPath(m, m.x, m.y, m.z, newX, newY, newZ)
  if hit then
    impactMeteor(m, hitX, hitY, hitZ)
    return
  end

  m.x, m.y, m.z = newX, newY, newZ
  m.yaw = m.yaw + m.rotSpeed.x
  m.pitch = m.pitch + m.rotSpeed.y
  m.roll = m.roll + m.rotSpeed.z

  m.trail[#m.trail + 1] = { x = m.x, y = m.y, z = m.z }
  local trailLimit = m.isComet and 110 or math.floor(setting("trail_length", 54))
  while #m.trail > trailLimit do table.remove(m.trail, 1) end

  if not m.isComet and not m.isFragment and m.fragmentDepth < 2 and m.size > setting("fragment_min_size", 0.15) * 2.2 then
    local atmosphericStress = airDensity * speedSq * 600
    local strength = setting("stone_strength", 8.0) / math.max(0.35, m.size)
    if atmosphericStress > strength then
      fragmentMeteor(m)
      return
    end
  end
end

local function updatePhysics()
  local i = 1
  while i <= #meteors do
    local m = meteors[i]
    if not m.alive or m.age > m.maxAge or m.y < -16 or m.y > 340 then
      table.remove(meteors, i)
    else
      updateMeteorPhysics(m)
      if not m.alive then table.remove(meteors, i) else i = i + 1 end
    end
  end
end

local function updateEffects()
  local i = 1
  while i <= #effects do
    local e = effects[i]
    e.age = e.age + 1
    if e.age >= e.maxAge then table.remove(effects, i) else i = i + 1 end
  end
end

local function spawnAtmosphereParticles()
  local density = math.floor(setting("effect_density", 2))
  local budget = 10 + density * 14
  local emitted = 0
  local i
  for i = 1, #meteors do
    if emitted >= budget then break end
    local m = meteors[i]
    if not m.isComet and m.temperature > 0.16 and m.y < 155 then
      local speed = length3(m.vx, m.vy, m.vz)
      local bx, by, bz = normalize3(-m.vx, -m.vy, -m.vz)
      local count = math.min(1 + math.floor(m.temperature * density * 1.7), budget - emitted)
      local p
      for p = 1, count do
        local jitterX = (math.random() - 0.5) * m.size
        local jitterY = (math.random() - 0.5) * m.size
        local jitterZ = (math.random() - 0.5) * m.size
        local smoke = math.random() < 0.20
        minecraft.particles.spawn({
          x = m.x + jitterX,
          y = m.y + jitterY,
          z = m.z + jitterZ,
          vx = bx * (0.08 + speed * 0.025) + (math.random() - 0.5) * 0.10,
          vy = by * (0.08 + speed * 0.025) + (math.random() - 0.5) * 0.10 + (smoke and 0.035 or 0),
          vz = bz * (0.08 + speed * 0.025) + (math.random() - 0.5) * 0.10,
          scale = smoke and (0.9 + m.temperature * 1.7) or (0.32 + m.temperature * 0.95),
          r = smoke and 0.16 or 1.0,
          g = smoke and 0.13 or (0.35 + m.temperature * 0.53),
          b = smoke and 0.11 or 0.045,
          max_age = smoke and (28 + math.floor(math.random() * 24)) or (8 + math.floor(m.temperature * 16)),
          gravity = smoke and -0.004 or 0.012,
        })
        emitted = emitted + 1
      end
    end
  end
end

-- ---------------------------------------------------------------------------
-- Events
-- ---------------------------------------------------------------------------

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
    minecraft.screen.add_button(x, 48, 200, 20, "Spawn Atmospheric Meteor", function()
      minecraft.screen.close()
      spawnMeteorFromKeybind()
    end)
    minecraft.screen.add_button(x, 76, 200, 20, "Spawn Cinematic Comet", function()
      minecraft.screen.close()
      spawnComet()
    end)
    minecraft.screen.add_button(x, 112, 200, 20, "Done", function()
      minecraft.screen.close()
    end)
  elseif event.phase == "render" then
    minecraft.gui.fill_rect(4, 4, event.width - 8, event.height - 8, 0xE30B111A)
    minecraft.gui.draw_text(math.floor(event.width / 2 - 58), 20, "CELESTIAL EVENTS", 0xFFFFFFFF)
    minecraft.gui.draw_text(math.floor(event.width / 2 - 83), 142, "M: meteor     N: comet", 0xFF9EC9FF)
  elseif event.phase == "key" and event.key == minecraft.keys.escape then
    minecraft.screen.close()
    event.handled = true
  end
  return event
end)

minecraft.on(minecraft.events.world_start, {}, function(event)
  meteors = {}
  effects = {}
  spawnCooldown = 0
  cometTimer = 0
  nextCometTick = 3600 + math.random(3600)
  return event
end)

minecraft.on(minecraft.events.client_tick, { after_world = true }, function(event)
  if event.paused then return event end
  if spawnCooldown > 0 then spawnCooldown = spawnCooldown - 1 end
  renderClock = renderClock + 1
  cometTimer = cometTimer + 1
  if cometTimer >= nextCometTick then
    spawnComet()
    cometTimer = 0
    nextCometTick = 3600 + math.random(4200)
  end
  updatePhysics()
  updateEffects()
  spawnAtmosphereParticles()
  return event
end)

minecraft.on(minecraft.events.world_render, { stage = "entities", moment = "after", priority = 120 }, function(event)
  if event.shadow_pass or not event.has_world then return event end

  local camera = {
    x = tonumber(event.camera_x) or lastCamera.x,
    y = tonumber(event.camera_y) or lastCamera.y,
    z = tonumber(event.camera_z) or lastCamera.z,
  }
  lastCamera.x, lastCamera.y, lastCamera.z = camera.x, camera.y, camera.z
  lastCamera.yaw = tonumber(event.camera_yaw) or lastCamera.yaw
  lastCamera.pitch = tonumber(event.camera_pitch) or lastCamera.pitch
  lastCamera.valid = true

  local tickDelta = tonumber(event.tick_delta) or 0
  local translucent = {}
  local drawList = {}
  local i

  for i = 1, #meteors do
    local m = meteors[i]
    if m.alive then
      local x, y, z = renderPosition(m, tickDelta)
      local dx, dy, dz = x - camera.x, y - camera.y, z - camera.z
      local distance = length3(dx, dy, dz)
      if distance < 1100 then
        if m.isComet then
          appendCometTail(translucent, camera, m, x, y, z, distance)
        else
          appendMeteorTrail(translucent, camera, m, x, y, z, distance)
          appendMeteorGlow(translucent, camera, m, x, y, z, distance)
        end
        drawList[#drawList + 1] = { meteor = m, x = x, y = y, z = z, distance = distance }
      end
    end
  end

  appendEffects(translucent, camera)

  if #translucent > 0 then
    minecraft.render.quads({
      world_space = true,
      blend = true,
      cull = false,
      depth_test = true,
      depth_write = false,
      vertices = translucent,
    })
  end

  for i = 1, #drawList do
    local d = drawList[i]
    drawMeteorBody(d.meteor, d.x, d.y, d.z, d.distance)
  end
  return event
end)

minecraft.log("info", "Meteors stunning geology renderer loaded")