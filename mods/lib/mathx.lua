local mathx = {}

local sqrt, sin, cos, floor, max = math.sqrt, math.sin, math.cos, math.floor, math.max

mathx.tau = math.pi * 2
mathx.deg_to_rad = math.pi / 180

function mathx.clamp(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

function mathx.mix(a, b, t)
  return a + (b - a) * t
end

function mathx.smoothstep(edge0, edge1, x)
  if edge0 == edge1 then return x >= edge1 and 1 or 0 end
  local t = (x - edge0) / (edge1 - edge0)
  if t < 0 then t = 0 elseif t > 1 then t = 1 end
  return t * t * (3 - 2 * t)
end

function mathx.length3(x, y, z)
  return sqrt(x * x + y * y + z * z)
end

function mathx.normalize3(x, y, z)
  local len = sqrt(x * x + y * y + z * z)
  if len < 1e-6 then return 0, 1, 0 end
  return x / len, y / len, z / len
end

function mathx.dot(ax, ay, az, bx, by, bz)
  return ax * bx + ay * by + az * bz
end

function mathx.cross(ax, ay, az, bx, by, bz)
  return ay * bz - az * by,
         az * bx - ax * bz,
         ax * by - ay * bx
end

function mathx.hash01(a, b, c, d)
  local n = sin((a or 0) * 12.9898 + (b or 0) * 78.233 +
    (c or 0) * 37.719 + (d or 0) * 19.913) * 43758.5453123
  return n - floor(n)
end

function mathx.randomUnit(seed, channel)
  local z = mathx.hash01(seed, channel, 1, 0) * 2 - 1
  local a = mathx.hash01(seed, channel, 2, 0) * mathx.tau
  local r = sqrt(max(0, 1 - z * z))
  return r * cos(a), z, r * sin(a)
end

return mathx
