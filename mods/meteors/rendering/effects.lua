-- Meteors Rendering: Visual effects, trails, and model rendering
local config = require("meteors.config")
local rendering = {}

local TAU = config.tau
local MAX_RENDER_VERTICES = config.max_render_vertices

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

local function cross(ax, ay, az, bx, by, bz)
  return ay * bz - az * by,
         az * bx - ax * bz,
         ax * by - ay * bx
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

-- Render a meteor with its model and trail
function rendering.renderMeteor(meteor, camera, geologyModule)
  local vertices = {}
  local modelHandle = geologyModule.buildModel(meteor.kind, meteor.variant, meteor.thermal)
  
  if modelHandle then
    -- Render the meteor model
    minecraft.render.push()
    minecraft.render.translate(meteor.x, meteor.y, meteor.z)
    minecraft.render.scale(meteor.size, meteor.size, meteor.size)
    minecraft.render.model(modelHandle)
    minecraft.render.pop()
  end
  
  -- Render trail
  local effectDensity = config.get("effect_density") or 2
  if #meteor.trail > 1 and effectDensity >= 1 then
    local trailLength = math.min(#meteor.trail, config.get("trail_length") or 54)
    local i
    for i = 2, trailLength do
      local p1 = meteor.trail[i-1]
      local p2 = meteor.trail[i]
      local t = i / trailLength
      local width = (1 - t) * meteor.size * 2.5
      local alpha = (1 - t) * 0.6 * (meteor.thermal and 1.0 or 0.7)
      local r, g, b = 1.0, 0.4 + t * 0.3, 0.1
      
      if meteor.thermal then
        r, g, b = 1.0, 0.2 + t * 0.5, 0.05 + t * 0.2
      end
      
      appendRibbonSegment(vertices, camera, p1, p2, width, width * 0.8,
        r, g, b, alpha, r, g, b, alpha * 0.3)
    end
  end
  
  -- Render glow effect for thermal meteors
  if meteor.thermal then
    local glowRadius = meteor.size * 4
    local glowAlpha = 0.3 + math.sin(meteor.age * 10) * 0.1
    appendGlowFan(vertices, camera, meteor.x, meteor.y, meteor.z,
      glowRadius, 1.0, 0.5, 0.1, glowAlpha, 8)
  end
  
  return vertices
end

-- Render explosion flash at impact
function rendering.renderExplosion(x, y, z, size, age, camera)
  local vertices = {}
  local maxAge = 0.5
  local t = math.min(age / maxAge, 1)
  local alpha = (1 - t) * 0.8
  local radius = size * (2 + t * 4)
  
  appendGlowFan(vertices, camera, x, y, z, radius, 1.0, 0.9, 0.5, alpha, 12)
  
  return vertices
end

-- Render comet coma and tail
function rendering.renderCometEffects(meteor, camera, dt)
  local vertices = {}
  local effectDensity = config.get("effect_density") or 2
  
  if meteor.kind ~= "comet" or effectDensity < 2 then
    return vertices
  end
  
  -- Coma (glowing envelope around nucleus)
  local comaRadius = meteor.size * 6
  local comaAlpha = 0.15 + math.sin(meteor.age * 3) * 0.05
  appendGlowFan(vertices, camera, meteor.x, meteor.y, meteor.z,
    comaRadius, 0.6, 0.8, 1.0, comaAlpha, 10)
  
  -- Tail (pointing away from sun approximation)
  local lightDirX, lightDirY, lightDirZ = normalize3(0.42, 0.82, -0.38)
  local tailLength = meteor.size * 15
  local tailStart = {
    x = meteor.x - lightDirX * meteor.size,
    y = meteor.y - lightDirY * meteor.size,
    z = meteor.z - lightDirZ * meteor.size,
  }
  local tailEnd = {
    x = meteor.x - lightDirX * tailLength,
    y = meteor.y - lightDirY * tailLength,
    z = meteor.z - lightDirZ * tailLength,
  }
  
  local tailWidth = meteor.size * 3
  appendRibbonSegment(vertices, camera, tailStart, tailEnd,
    tailWidth, tailWidth * 0.2,
    0.5, 0.7, 1.0, 0.2,
    0.5, 0.7, 1.0, 0.0)
  
  return vertices
end

return rendering
