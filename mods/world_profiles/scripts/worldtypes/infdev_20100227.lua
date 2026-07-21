local MASK = 0xffffffffffff
local MULTIPLIER = 0x5deece66d
local ADDEND = 0xb
local TWO_POW_24 = 16777216.0
local TWO_POW_53 = 9007199254740992.0

local function trunc(value)
  return math.modf(value)
end

local function java_seed(value)
  return math.tointeger(value) or trunc(tonumber(value) or 0)
end

local Random = {}
Random.__index = Random

function Random.new(seed)
  local self = setmetatable({}, Random)
  self:set_seed(seed)
  return self
end

function Random:set_seed(seed)
  self.seed = (java_seed(seed) ~ MULTIPLIER) & MASK
end

function Random:next(bits)
  self.seed = (self.seed * MULTIPLIER + ADDEND) & MASK
  return self.seed >> (48 - bits)
end

function Random:next_int(bound)
  if bound <= 0 then
    return 0
  end
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

function Random:next_float()
  return self:next(24) / TWO_POW_24
end

function Random:next_double()
  return ((self:next(26) << 27) + self:next(27)) / TWO_POW_53
end

function Random:next_long()
  local high = self:next(32)
  local low = self:next(32)
  if high >= 0x80000000 then high = high - 0x100000000 end
  if low >= 0x80000000 then low = low - 0x100000000 end
  return (high << 32) + low
end

local function fade(value)
  return value * value * value * (value * (value * 6.0 - 15.0) + 10.0)
end

local function lerp(amount, low, high)
  return low + amount * (high - low)
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
  local floor_x = math.floor(x)
  local floor_y = math.floor(y)
  local floor_z = math.floor(z)
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
  local self = setmetatable({ layers = {} }, Octaves)
  for index = 1, count do
    self.layers[index] = Perlin.new(random)
  end
  return self
end

function Octaves:sample(x, y, z)
  local result = 0.0
  local scale = 1.0
  for index = 1, #self.layers do
    result = result + self.layers[index]:sample(x / scale, y / scale, z / scale) * scale
    scale = scale * 2.0
  end
  return result
end

function Octaves:sample2(x, z)
  return self:sample(x, z, 0.0)
end

local states = {}

local function make_state(seed)
  local random = Random.new(seed ~ 0x2271433)
  return {
    low = Octaves.new(random, 16),
    high = Octaves.new(random, 16),
    selector = Octaves.new(random, 8),
    detail_a = Octaves.new(random, 4),
    detail_b = Octaves.new(random, 4),
    shape = Octaves.new(random, 5),
  }
end

local function state_for(seed)
  seed = java_seed(seed)
  local key = tostring(seed)
  local state = states[key]
  if state == nil then
    state = make_state(seed)
    states[key] = state
  end
  return state
end

local function block_id(name)
  return minecraft.world.block_id(name) or 0
end

local function fill_column(chunk, x, z, block_at)
  local start_y = 0
  local current = block_at(0)
  for y = 1, 128 do
    local next_block = y < 128 and block_at(y) or -1
    if next_block ~= current then
      chunk:fill(x, start_y, z, x, y - 1, z, current)
      start_y = y
      current = next_block
    end
  end
end

local function terrain(event, state)
  local stone = block_id("stone")
  local grass = block_id("grass_block")
  local dirt = block_id("dirt")
  local water = block_id("still_water")
  local obsidian = block_id("obsidian")
  local bricks = block_id("bricks")
  local flower = block_id("dandelion")
  for x = 0, 15 do
    for z = 0, 15 do
      local wx = event.chunk_x * 16 + x
      local wz = event.chunk_z * 16 + z
      local cell_x = trunc(wx / 1024)
      local cell_z = trunc(wz / 1024)
      local broad = (state.low:sample(wx * 32.0, 0.0, wz * 32.0) - state.high:sample(wx * 64.0, 0.0, wz * 64.0)) / 2048.0
      local selector = state.detail_b:sample2(wx / 4.0, wz / 4.0)
      local shape = state.shape:sample2(wx / 8.0, wz / 8.0) / 8.0
      local detail
      if selector > 0.0 then
        detail = state.selector:sample2(wx * 0.51428568, wz * 0.51428568) * shape / 4.0
      else
        detail = state.detail_a:sample2(wx * 0.25714284, wz * 0.25714284) * shape
      end
      local height = trunc(broad + 64.0 + detail)
      if state.detail_b:sample2(wx, wz) < 0.0 then
        height = trunc(height / 2) * 2
        if state.detail_b:sample2(wx / 5.0, wz / 5.0) < 0.0 then
          height = height + 1
        end
      end
      local cell_random = Random.new(cell_x + cell_z * 13871)
      local center_x = (cell_x << 10) + 128 + cell_random:next_int(512)
      local center_z = (cell_z << 10) + 128 + cell_random:next_int(512)
      local distance = math.max(math.abs(wx - center_x), math.abs(wz - center_z))
      local pyramid = math.max(127 - distance, height)
      local flower_random = Random.new(java_seed(event.world_seed) ~ (wx * 73428767) ~ (wz * 912931))
      fill_column(event.chunk, x, z, function(y)
        local value = 0
        if (wx == 0 or wz == 0) and y <= height + 2 then
          value = obsidian
        elseif y == height + 1 and height >= 64 and flower_random:next_double() < 0.02 then
          value = flower
        elseif y == height and height >= 64 then
          value = grass
        elseif y <= height - 2 then
          value = stone
        elseif y <= height then
          value = dirt
        elseif y <= 64 then
          value = water
        end
        if y <= pyramid and (value == 0 or value == water) then
          value = bricks
        end
        return value
      end)
    end
  end
end

return {
  id = "infdev_20100227",
  label = "Infdev 20100227",
  order = 60,
  cancel = { terrain = true, surface = true, carver = true, features = true },
  spawn = function()
    return 1, nil
  end,
  generate = function(event)
    if event.stage == minecraft.generation.stages.terrain then
      terrain(event, state_for(event.world_seed))
    end
  end,
}
