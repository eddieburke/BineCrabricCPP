local config = require("config")
local mathx = require("lib.mathx")
local generator = require("scripts.geology.generator")

local rendering = {}

local TAU = mathx.tau
local clamp, mix, smoothstep = mathx.clamp, mathx.mix, mathx.smoothstep
local normalize3, cross, length3 = mathx.normalize3, mathx.cross, mathx.length3
local sin, cos, min, floor, sqrt = math.sin, math.cos, math.min, math.floor, math.sqrt

local MAX_FLOATS = config.max_render_vertices * 9

local px, py, pz = {}, {}, {}

local EMBER_TRAIL = { 1.00, 0.96, 0.84, 1.00, 0.48, 0.12, 0.52, 0.08, 0.02 }
local ICE_TRAIL = { 0.90, 0.96, 1.00, 0.52, 0.74, 1.00, 0.16, 0.28, 0.68 }
local DUST_TAIL = { 1.00, 0.94, 0.82, 0.94, 0.82, 0.62, 0.62, 0.52, 0.42 }
local ION_TAIL = { 0.72, 0.90, 1.00, 0.42, 0.66, 1.00, 0.18, 0.32, 0.78 }

local function vertex(out, n, x, y, z, r, g, b, a)
  out[n + 1] = x
  out[n + 2] = y
  out[n + 3] = z
  out[n + 4] = 0
  out[n + 5] = 0
  out[n + 6] = r
  out[n + 7] = g
  out[n + 8] = b
  out[n + 9] = a
  return n + 9
end

local function billboardBasis(cam, x, y, z)
  local vx, vy, vz = normalize3(cam.x - x, cam.y - y, cam.z - z)
  local rx, ry, rz = cross(0, 1, 0, vx, vy, vz)
  if length3(rx, ry, rz) < 0.001 then
    rx, ry, rz = cross(0, 0, 1, vx, vy, vz)
  end
  rx, ry, rz = normalize3(rx, ry, rz)
  local ux, uy, uz = cross(vx, vy, vz, rx, ry, rz)
  ux, uy, uz = normalize3(ux, uy, uz)
  return rx, ry, rz, ux, uy, uz
end

local function glow(out, n, cam, x, y, z, radius, r, g, b, alpha, sectors)
  if alpha <= 0.003 or radius <= 0.001 then return n end
  local rx, ry, rz, ux, uy, uz = billboardBasis(cam, x, y, z)
  local inner = radius * 0.34
  local innerAlpha = alpha * 0.80
  for i = 0, sectors - 1 do
    if n + 72 > MAX_FLOATS then return n end
    local a0 = TAU * i / sectors
    local a1 = TAU * (i + 1) / sectors
    local c0, s0 = cos(a0), sin(a0)
    local c1, s1 = cos(a1), sin(a1)
    local d0x, d0y, d0z = rx * c0 + ux * s0, ry * c0 + uy * s0, rz * c0 + uz * s0
    local d1x, d1y, d1z = rx * c1 + ux * s1, ry * c1 + uy * s1, rz * c1 + uz * s1
    local i0x, i0y, i0z = x + d0x * inner, y + d0y * inner, z + d0z * inner
    local i1x, i1y, i1z = x + d1x * inner, y + d1y * inner, z + d1z * inner
    local o0x, o0y, o0z = x + d0x * radius, y + d0y * radius, z + d0z * radius
    local o1x, o1y, o1z = x + d1x * radius, y + d1y * radius, z + d1z * radius
    n = vertex(out, n, x, y, z, r, g, b, alpha)
    n = vertex(out, n, i0x, i0y, i0z, r, g, b, innerAlpha)
    n = vertex(out, n, i1x, i1y, i1z, r, g, b, innerAlpha)
    n = vertex(out, n, x, y, z, r, g, b, alpha)
    n = vertex(out, n, i0x, i0y, i0z, r, g, b, innerAlpha)
    n = vertex(out, n, o0x, o0y, o0z, r, g, b, 0)
    n = vertex(out, n, o1x, o1y, o1z, r, g, b, 0)
    n = vertex(out, n, i1x, i1y, i1z, r, g, b, innerAlpha)
  end
  return n
end

local function ramp(c, u)
  if u < 0.35 then
    local t = u / 0.35
    return mix(c[1], c[4], t), mix(c[2], c[5], t), mix(c[3], c[6], t)
  end
  local t = (u - 0.35) / 0.65
  return mix(c[4], c[7], t), mix(c[5], c[8], t), mix(c[6], c[9], t)
end

-- Fire as a chain of soft round billboard puffs. Puffs are rotationally
-- symmetric, so they look the same from every camera angle: no ribbons,
-- no side vectors, no end-on starbursts, no facing flips.
local function puffChain(out, n, cam, count, radiusHead, radiusTail, alphaHead, alphaCurve, colors, sectors)
  if count < 1 or alphaHead <= 0.003 then return n end
  local last = count - 1
  if last < 1 then last = 1 end
  -- Stride so a long trail never costs more than ~12 puffs.
  local step = 1
  if count > 13 then step = 2 end
  if count > 25 then step = 3 end
  for i = 1, count, step do
    local u = (i - 1) / last
    local a = alphaHead * (1 - u) ^ alphaCurve
    if a > 0.004 then
      local r, g, b = ramp(colors, u)
      local radius = mix(radiusHead, radiusTail, u)
      if radius > 0.004 then
        n = glow(out, n, cam, px[i], py[i], pz[i], radius, r, g, b, a, sectors)
      end
    end
  end
  return n
end

local function gatherTrail(meteor, hx, hy, hz, limit, maxDist)
  px[1], py[1], pz[1] = hx, hy, hz
  local count = 1
  local cap = meteor.trailCap
  local head = meteor.trailHead
  local tx, ty, tz = meteor.tx, meteor.ty, meteor.tz
  local dist = 0
  for k = 0, floor(min(meteor.trailCount, limit)) - 1 do
    local idx = head - k
    if idx < 1 then idx = idx + cap end
    local x, y, z = tx[idx], ty[idx], tz[idx]
    local dx, dy, dz = x - px[count], y - py[count], z - pz[count]
    local d2 = dx * dx + dy * dy + dz * dz
    if d2 > 0.0025 then
      dist = dist + sqrt(d2)
      if maxDist ~= nil and dist > maxDist then break end
      count = count + 1
      px[count], py[count], pz[count] = x, y, z
    end
  end
  return count
end

local function cometTails(out, n, cam, meteor, hx, hy, hz, sunX, sunY, sunZ, quality, density)
  local ax, ay, az = normalize3(-sunX, -sunY, -sunZ)
  local size = meteor.size
  local segments = 4 + quality * 2
  local points = segments + 1

  -- Puffs along the anti-sun axis: a soft plume, never a needle.
  local tailSectors = 4 + quality
  local ionLength = size * 11
  for i = 1, points do
    local t = (i - 1) / segments
    -- Gentle lateral sway breaks the perfectly straight "needle" look.
    local sway = sin(t * 5.1 + meteor.id) * size * 0.22 * t
    local d = ionLength * t
    px[i] = hx + ax * d + (-az) * sway
    py[i] = hy + ay * d
    pz[i] = hz + az * d + ax * sway
  end
  n = puffChain(out, n, cam, points, size * 0.85, size * 1.5, 0.34, 1.6, ION_TAIL, tailSectors)

  if density < 2 then return n end

  local wx, wy, wz = normalize3(-meteor.vx, -meteor.vy, -meteor.vz)
  local dustLength = size * 8
  for i = 1, points do
    local t = (i - 1) / segments
    local bend = t * t * 0.55
    local dx, dy, dz = normalize3(mix(ax, wx, bend), mix(ay, wy, bend), mix(az, wz, bend))
    local sway = sin(t * 4.2 + meteor.id * 1.7) * size * 0.30 * t
    local d = dustLength * t
    px[i] = hx + dx * d + (-dz) * sway
    py[i] = hy + dy * d
    pz[i] = hz + dz * d + dx * sway
  end
  return puffChain(out, n, cam, points, size * 1.3, size * 3.0, 0.22, 1.7, DUST_TAIL, tailSectors)
end

function rendering.meteor(out, n, meteor, cam, tickDelta, sunX, sunY, sunZ, quality, density)
  local hx = meteor.px + (meteor.x - meteor.px) * tickDelta
  local hy = meteor.py + (meteor.y - meteor.py) * tickDelta
  local hz = meteor.pz + (meteor.z - meteor.pz) * tickDelta
  local heat = clamp(meteor.prevHeat + (meteor.heat - meteor.prevHeat) * tickDelta, 0, 1)
  local comet = meteor.kind == "comet"
  local sectors = 6 + quality * 3

  -- Fire trail as cooling puffs: wide and bright at the head, small and
  -- dim behind. Capped in world units so it reads as fire, not a contrail.
  local trailSectors = 4 + quality
  local count = gatherTrail(meteor, hx, hy, hz, config.get("trail_length") or 54, comet and 16 or 24)
  if count >= 1 then
    local colors = comet and ICE_TRAIL or EMBER_TRAIL
    local width = meteor.size * (comet and 1.1 or (0.55 + 1.35 * heat))
    local alpha = comet and 0.30 or (0.10 + 0.68 * heat)
    n = puffChain(out, n, cam, count, width, width * 0.15, alpha, 2.2, colors, trailSectors)
    if density >= 2 then
      n = puffChain(out, n, cam, count, width * 2.2, width * 0.35, alpha * 0.30, 1.8, colors, trailSectors)
    end
    if density >= 3 then
      n = puffChain(out, n, cam, count, width * 4.0, width * 0.70, alpha * 0.13, 1.5, colors, trailSectors)
    end
  end

  if comet then
    n = glow(out, n, cam, hx, hy, hz, meteor.size * 3.2, 0.58, 0.80, 1.00, 0.20, sectors)
    n = glow(out, n, cam, hx, hy, hz, meteor.size * 1.35, 0.90, 0.96, 1.00, 0.34, sectors)
    n = cometTails(out, n, cam, meteor, hx, hy, hz, sunX, sunY, sunZ, quality, density)
  elseif heat > 0.02 then
    local er, eg, eb = generator.ember(heat)
    n = glow(out, n, cam, hx, hy, hz, meteor.size * (1.8 + 5.0 * heat), er, eg, eb, 0.08 + 0.42 * heat, sectors)
    n = glow(out, n, cam, hx, hy, hz, meteor.size * (0.9 + 1.6 * heat),
      1.0, 0.55 + 0.40 * heat, 0.30 + 0.50 * heat, 0.20 + 0.55 * heat, sectors)
  end

  return n
end

function rendering.flash(out, n, flash, cam, tickDelta, quality)
  local t = clamp((flash.age + tickDelta) / 14, 0, 1)
  local alpha = (1 - t) * (1 - t) * 0.85
  if alpha <= 0.003 then return n end
  local sectors = 6 + quality * 3
  local radius = flash.size * (1.4 + t * 7.0)
  n = glow(out, n, cam, flash.x, flash.y + 0.4, flash.z, radius, 1.00, 0.86, 0.55, alpha, sectors)
  return glow(out, n, cam, flash.x, flash.y + 0.4, flash.z, radius * 0.42, 1.00, 0.97, 0.86, alpha, sectors)
end

return rendering
