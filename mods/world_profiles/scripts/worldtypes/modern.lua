local common = require("scripts.common")
local java_floor = common.java_floor
local block_id = common.block_id

local MASK = 0xffffffffffff
local MULTIPLIER = 0x5deece66d
local ADDEND = 0xb
local TWO_POW_53 = 9007199254740992.0
local MODERN_SEED_SALT = 0x4D6F6465726E

local function java_seed(value)
  assert(type(value) == "number", "world_profiles: seed must be a signed 64-bit integer")
  return assert(math.tointeger(value), "world_profiles: seed must be a signed 64-bit integer")
end

local function clamp(value, lo, hi)
  if value < lo then return lo end
  if value > hi then return hi end
  return value
end

local function lerp(a, b, t)
  return a + (b - a) * t
end

local function smoothstep(edge0, edge1, value)
  local t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

local Random = {}
Random.__index = Random

function Random.new(seed)
  local self = setmetatable({}, Random)
  self.seed = (java_seed(seed) ~ MULTIPLIER) & MASK
  return self
end

function Random:next(bits)
  self.seed = (self.seed * MULTIPLIER + ADDEND) & MASK
  return self.seed >> (48 - bits)
end

function Random:next_int(bound)
  if bound <= 0 then return 0 end
  if (bound & (bound - 1)) == 0 then
    return (bound * self:next(31)) >> 31
  end
  while true do
    local bits = self:next(31)
    local value = bits % bound
    if bits - value + bound - 1 < 0x80000000 then
      return value
    end
  end
end

function Random:next_double()
  return ((self:next(26) << 27) + self:next(27)) / TWO_POW_53
end

local function fade(value)
  return value * value * value * (value * (value * 6.0 - 15.0) + 10.0)
end

local function grad(hash, x, y, z)
  hash = hash & 15
  local u = hash < 8 and x or y
  local v = hash < 4 and y or ((hash == 12 or hash == 14) and x or z)
  return ((hash & 1) == 0 and u or -u) + ((hash & 2) == 0 and v or -v)
end

local Perlin = {}
Perlin.__index = Perlin

function Perlin.new(random)
  local self = setmetatable({ permutation = {} }, Perlin)
  self.x_offset = random:next_double() * 256.0
  self.y_offset = random:next_double() * 256.0
  self.z_offset = random:next_double() * 256.0
  for index = 0, 255 do
    self.permutation[index] = index
  end
  for index = 0, 255 do
    local swap_index = random:next_int(256 - index) + index
    self.permutation[index], self.permutation[swap_index] = self.permutation[swap_index], self.permutation[index]
    self.permutation[index + 256] = self.permutation[index]
  end
  return self
end

function Perlin:sample(x, y, z)
  x = x + self.x_offset
  y = y + self.y_offset
  z = z + self.z_offset
  local floor_x = java_floor(x)
  local floor_y = java_floor(y)
  local floor_z = java_floor(z)
  local ix = floor_x & 255
  local iy = floor_y & 255
  local iz = floor_z & 255
  x = x - floor_x
  y = y - floor_y
  z = z - floor_z
  local u = fade(x)
  local v = fade(y)
  local w = fade(z)
  local p = self.permutation
  local a = p[ix] + iy
  local aa = p[a] + iz
  local ab = p[a + 1] + iz
  local b = p[ix + 1] + iy
  local ba = p[b] + iz
  local bb = p[b + 1] + iz
  return lerp(w,
    lerp(v, lerp(u, grad(p[aa], x, y, z), grad(p[ba], x - 1.0, y, z)),
      lerp(u, grad(p[ab], x, y - 1.0, z), grad(p[bb], x - 1.0, y - 1.0, z))),
    lerp(v, lerp(u, grad(p[aa + 1], x, y, z - 1.0), grad(p[ba + 1], x - 1.0, y, z - 1.0)),
      lerp(u, grad(p[ab + 1], x, y - 1.0, z - 1.0), grad(p[bb + 1], x - 1.0, y - 1.0, z - 1.0))))
end

local Octaves = {}
Octaves.__index = Octaves

function Octaves.new(random, count)
  local self = setmetatable({ layers = {}, count = count }, Octaves)
  for index = 1, count do
    self.layers[index] = Perlin.new(random)
  end
  return self
end

function Octaves:sample3(x, y, z, scale_x, scale_y, scale_z)
  local value = 0.0
  local frequency = 1.0
  for index = 1, self.count do
    value = value + self.layers[index]:sample(
      x * scale_x * frequency,
      y * scale_y * frequency,
      z * scale_z * frequency
    ) / frequency
    frequency = frequency * 0.5
  end
  return value
end

function Octaves:sample2(x, z, scale_x, scale_z)
  return self:sample3(x, 10.0, z, scale_x, 1.0, scale_z)
end

local BASE_CURVE = {
  { -1.00, 26.0 },
  { -0.60, 34.0 },
  { -0.40, 46.0 },
  { -0.25, 57.0 },
  { -0.12, 62.0 },
  { -0.05, 65.5 },
  {  0.05, 67.0 },
  {  0.25, 72.0 },
  {  0.50, 80.0 },
  {  1.00, 90.0 },
}

local function normalize(value, octaves, gain)
  local total = (1 << octaves) - 1
  return clamp(value / total * gain, -1.0, 1.0)
end

local function base_height(continentalness)
  local c = clamp(continentalness, -1.0, 1.0)
  for i = 2, #BASE_CURVE do
    local hi = BASE_CURVE[i]
    if c <= hi[1] then
      local lo = BASE_CURVE[i - 1]
      return lerp(lo[2], hi[2], smoothstep(lo[1], hi[1], c))
    end
  end
  return BASE_CURVE[#BASE_CURVE][2]
end

local function mountain_amplitude(continentalness, erosion)
  local inland = smoothstep(0.0, 0.45, continentalness)
  return lerp(34.0, 3.0, smoothstep(-0.5, 0.7, erosion)) * inland
end

local function peaks_and_valleys(weirdness)
  return 1.0 - math.abs(3.0 * math.abs(weirdness) - 2.0)
end

local function mountainness(height)
  return smoothstep(72.0, 104.0, height)
end

local function surface_height(climate)
  local base = base_height(climate.continentalness)
  local amplitude = mountain_amplitude(climate.continentalness, climate.erosion)
  local shape = peaks_and_valleys(climate.weirdness)
  local ridge = shape > 0.0 and (shape ^ 1.5) or 0.0
  local valley = shape < 0.0 and shape or 0.0
  local height = base + amplitude * (ridge + valley * 0.35)
  if height > 100.0 then
    height = 100.0 + (height - 100.0) * 0.6
  end
  local land = smoothstep(-0.12, -0.02, climate.continentalness)
  local river_cut = (1.0 - smoothstep(0.0, 0.05, math.abs(climate.river))) * land
  if river_cut > 0.0 and height > 59.0 then
    height = lerp(height, 59.0, river_cut)
  end
  return height
end

local states = {}
local state_order = {}

local function state_for(seed)
  seed = java_seed(seed)
  local key = tostring(seed)
  local state = states[key]
  if state == nil then
    local random = Random.new(seed ~ MODERN_SEED_SALT)
    state = {
      continentalness = Octaves.new(random, 6),
      erosion = Octaves.new(random, 5),
      weirdness = Octaves.new(random, 5),
      river = Octaves.new(random, 4),
      detail = Octaves.new(random, 4),
    }
    states[key] = state
    state_order[#state_order + 1] = key
    if #state_order > 16 then
      local oldest = table.remove(state_order, 1)
      states[oldest] = nil
    end
  end
  return state
end

local function build_density_grid(state, cell_origin_x, cell_origin_z)
  local density = {}
  local index = 1
  for cx = 0, 4 do
    local wx = cell_origin_x + cx
    for cz = 0, 4 do
      local wz = cell_origin_z + cz
      local raw_c = normalize(state.continentalness:sample2(wx, wz, 0.08, 0.08), 6, 3.6) + 0.18
      local raw_e = normalize(state.erosion:sample2(wx, wz, 0.1, 0.1), 5, 2.8)
      local raw_w = normalize(state.weirdness:sample2(wx, wz, 0.16, 0.16), 5, 2.8)
      local raw_r = normalize(state.river:sample2(wx, wz, 0.067, 0.067), 4, 2.8)
      -- Keep a mild coastal/inland foothold right at the world origin so the
      -- 16x16-chunk spawn spiral always finds dry land below the alpine line,
      -- while blending out completely within a few chunks.
      local dist_sq = wx * wx + wz * wz
      if dist_sq < 64 then
        local blend = smoothstep(0.0, 64.0, dist_sq)
        raw_c = lerp(clamp(raw_c, 0.08, 0.35), raw_c, blend)
        raw_e = lerp( math.max(raw_e, 0.1), raw_e, blend)
        if math.abs(raw_r) < 0.08 then
          raw_r = lerp(0.12, raw_r, blend)
        end
      end
      local climate = {
        continentalness = clamp(raw_c, -1.0, 1.0),
        erosion = raw_e,
        weirdness = raw_w,
        river = raw_r,
      }
      local height = surface_height(climate)
      local detail_amp = 0.6 + 1.6 * mountainness(height)
      for iy = 0, 16 do
        local detail = normalize(state.detail:sample3(wx, iy, wz, 0.8, 1.6, 0.8), 4, 2.0)
        local value = (height - iy * 8.0) * 0.1 + detail * detail_amp
        if iy > 13 then
          local fade_t = (iy - 13.0) / 3.0
          value = value * (1.0 - fade_t) - 10.0 * fade_t
        end
        density[index] = value
        index = index + 1
      end
    end
  end
  return density
end

local function fallback_fill_density(chunk, density, sea_level)
  local stone = block_id("stone")
  local water = block_id("still_water")
  for cell_x = 0, 3 do
    for cell_z = 0, 3 do
      for cell_y = 0, 15 do
        local base00 = ((cell_x + 0) * 5 + (cell_z + 0)) * 17 + cell_y + 1
        local base01 = ((cell_x + 0) * 5 + (cell_z + 1)) * 17 + cell_y + 1
        local base10 = ((cell_x + 1) * 5 + (cell_z + 0)) * 17 + cell_y + 1
        local base11 = ((cell_x + 1) * 5 + (cell_z + 1)) * 17 + cell_y + 1
        local d00, d01, d10, d11 = density[base00], density[base01], density[base10], density[base11]
        local dy0 = (density[base00 + 1] - d00) * 0.125
        local dy1 = (density[base01 + 1] - d01) * 0.125
        local dy2 = (density[base10 + 1] - d10) * 0.125
        local dy3 = (density[base11 + 1] - d11) * 0.125
        for sub_y = 0, 7 do
          local x0, x1 = d00, d01
          local dx0 = (d10 - d00) * 0.25
          local dx1 = (d11 - d01) * 0.25
          local ly = cell_y * 8 + sub_y
          for sub_x = 0, 3 do
            local d = x0
            local dz = (x1 - x0) * 0.25
            local lx = cell_x * 4 + sub_x
            for sub_z = 0, 3 do
              local lz = cell_z * 4 + sub_z
              local id = 0
              if ly < sea_level then id = water end
              if d > 0.0 then id = stone end
              chunk:set_block(lx, ly, lz, id)
              d = d + dz
            end
            x0 = x0 + dx0
            x1 = x1 + dx1
          end
          d00 = d00 + dy0
          d01 = d01 + dy1
          d10 = d10 + dy2
          d11 = d11 + dy3
        end
      end
    end
  end
end

local function terrain(event)
  local state = state_for(event.world_seed)
  local density = build_density_grid(state, event.chunk_x * 4, event.chunk_z * 4)
  local chunk = event.chunk
  if chunk.fill_density then
    chunk:fill_density(density, 64)
  else
    fallback_fill_density(chunk, density, 64)
  end
end

local function alpine_surface(event)
  local chunk = event.chunk
  local stone = block_id("stone")
  local snow = block_id("snow_block")
  for x = 0, 15 do
    for z = 0, 15 do
      local top = chunk.top_solid_y and chunk:top_solid_y(x, z) or (chunk:get_height(x, z) - 1)
      if top >= 98 then
        local temp = chunk.get_temperature and chunk:get_temperature(x, z) or 0.5
        local cap = (top >= 108 or (top >= 102 and temp < 0.5)) and snow or stone
        chunk:set_block(x, top, z, cap)
        for depth = 1, 3 do
          local y = top - depth
          if y > 0 then
            chunk:set_block(x, y, z, stone)
          end
        end
      end
    end
  end
end

return {
  id = "modern",
  label = "Modern Inspired (128 High)",
  order = 60,
  dimensions = function(_, info) return info.terrain_styles end,
  spawn = function()
    return 65, 104
  end,
  stages = {
    terrain = terrain,
  },
  after = {
    surface = alpine_surface,
  },
  base_height = base_height,
  surface_height = surface_height,
}
