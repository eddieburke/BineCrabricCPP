local math_util = {}

function math_util.clamp(value, lo, hi)
  return math.min(math.max(value, lo), hi)
end

function math_util.lerp(a, b, t)
  return a + (b - a) * t
end

function math_util.smoothstep(edge0, edge1, value)
  if edge0 == edge1 then return value < edge0 and 0.0 or 1.0 end
  local t = math_util.clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

function math_util.normalize3(x, y, z)
  local len = math.sqrt(x * x + y * y + z * z)
  if len < 0.000001 then return 0, 1, 0 end
  return x / len, y / len, z / len
end

function math_util.hash01(a, b, c, d)
  local n = math.sin((a or 0) * 12.9898 + (b or 0) * 78.233 +
    (c or 0) * 37.719 + (d or 0) * 19.913) * 43758.5453123
  return n - math.floor(n)
end

function math_util.hash_for_day(seed, day, offset)
  local x = seed + day * 3 + offset
  x = ((x * 1103515245) + 12345) % 2147483647
  x = ((x * 1103515245) + 12345) % 2147483647
  return (x % 10000) / 10000.0
end

function math_util.hash_string(value)
  local hash = 2166136261
  for index = 1, #value do
    hash = (hash ~ string.byte(value, index)) * 16777619
  end
  return hash & 0x7fffffff
end

function math_util.hash_float(seed)
  local hash = seed & 0x7fffffff
  hash = (hash ~ (hash >> 16)) * 0x45d9f3b
  hash = (hash ~ (hash >> 16)) * 0x45d9f3b
  hash = hash ~ (hash >> 16)
  return (hash & 0x7fffffff) / 0x7fffffff
end

return math_util
