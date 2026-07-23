local MASK = 0xffffffffffff
local MULTIPLIER = 0x5deece66d
local ADDEND = 0xb
local TWO_POW_24 = 16777216.0
local TWO_POW_53 = 9007199254740992.0
local PI = math.pi
local SIN_PI = 3.14159

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
  local random = Random.new(seed)
  local state = {
    low = Octaves.new(random, 16),
    high = Octaves.new(random, 16),
    selector = Octaves.new(random, 8),
    surface = Octaves.new(random, 4),
    depth = Octaves.new(random, 4),
    heights = {},
    height_order = {},
  }
  Octaves.new(random, 5)
  state.forest = Octaves.new(random, 5)
  return state
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

local function density(state, x, y, z)
  local vertical = y * 4.0 - 64.0
  if vertical < 0.0 then
    vertical = vertical * 3.0
  end
  local selector = state.selector:sample(x * 684.412 / 80.0, y * 684.412 / 400.0, z * 684.412 / 80.0) / 2.0
  local low = state.low:sample(x * 684.412, y * 984.412, z * 684.412) / 512.0 - vertical
  if low < -10.0 then low = -10.0 end
  if low > 10.0 then low = 10.0 end
  if selector < -1.0 then
    return low
  end
  local high = state.high:sample(x * 684.412, y * 984.412, z * 684.412) / 512.0 - vertical
  if high < -10.0 then high = -10.0 end
  if high > 10.0 then high = 10.0 end
  if selector > 1.0 then
    return high
  end
  return lerp((selector + 1.0) / 2.0, low, high)
end

local function density_grid(state, chunk_x, chunk_z)
  local grid = {}
  for gx = 0, 4 do
    grid[gx] = {}
    for gz = 0, 4 do
      local column = {}
      grid[gx][gz] = column
      for gy = 0, 32 do
        column[gy] = density(state, chunk_x * 4 + gx, gy, chunk_z * 4 + gz)
      end
    end
  end
  return grid
end

local function interpolated_density(grid, x, y, z)
  local gx = x // 4
  local gy = y // 4
  local gz = z // 4
  local fx = (x & 3) / 4.0
  local fy = (y & 3) / 4.0
  local fz = (z & 3) / 4.0
  local x00 = lerp(fx, grid[gx][gz][gy], grid[gx + 1][gz][gy])
  local x01 = lerp(fx, grid[gx][gz + 1][gy], grid[gx + 1][gz + 1][gy])
  local x10 = lerp(fx, grid[gx][gz][gy + 1], grid[gx + 1][gz][gy + 1])
  local x11 = lerp(fx, grid[gx][gz + 1][gy + 1], grid[gx + 1][gz + 1][gy + 1])
  return lerp(fz, lerp(fy, x00, x10), lerp(fy, x01, x11))
end

local function height_key(chunk_x, chunk_z)
  return tostring(chunk_x) .. ":" .. tostring(chunk_z)
end

local function remember_heights(state, chunk_x, chunk_z, heights)
  local key = height_key(chunk_x, chunk_z)
  if state.heights[key] == nil then
    state.height_order[#state.height_order + 1] = key
  end
  state.heights[key] = heights
  if #state.height_order > 512 then
    local old = table.remove(state.height_order, 1)
    state.heights[old] = nil
  end
end

local function calculate_heights(state, chunk_x, chunk_z, grid)
  local heights = {}
  grid = grid or density_grid(state, chunk_x, chunk_z)
  for x = 0, 15 do
    for z = 0, 15 do
      local top = -1
      for y = 127, 0, -1 do
        if interpolated_density(grid, x, y, z) > 0.0 then
          top = y
          break
        end
      end
      heights[x * 16 + z] = top
    end
  end
  remember_heights(state, chunk_x, chunk_z, heights)
  return heights
end

local function heights_at(state, chunk_x, chunk_z)
  local heights = state.heights[height_key(chunk_x, chunk_z)]
  if heights == nil then
    heights = calculate_heights(state, chunk_x, chunk_z)
  end
  return heights
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
  local water = block_id("still_water")
  local grid = density_grid(state, event.chunk_x, event.chunk_z)
  local heights = {}
  for x = 0, 15 do
    for z = 0, 15 do
      local top = -1
      fill_column(event.chunk, x, z, function(y)
        if interpolated_density(grid, x, y, z) > 0.0 then
          if y > top then top = y end
          return stone
        end
        if y < 64 then return water end
        return 0
      end)
      heights[x * 16 + z] = top
    end
  end
  remember_heights(state, event.chunk_x, event.chunk_z, heights)
end

local function surface(event, state)
  local stone = block_id("stone")
  local grass = block_id("grass_block")
  local dirt = block_id("dirt")
  local sand = block_id("sand")
  local gravel = block_id("gravel")
  local water = block_id("still_water")
  local random = Random.new(event.chunk_x * 341873128712 + event.chunk_z * 132897987541)
  for x = 0, 15 do
    for z = 0, 15 do
      local wx = event.chunk_x * 16 + x
      local wz = event.chunk_z * 16 + z
      local sandy = state.surface:sample(wx * 0.03125, wz * 0.03125, 0.0) + random:next_double() * 0.2 > 0.0
      local gravelly = state.surface:sample(wz * 0.03125, 109.0134, wx * 0.03125) + random:next_double() * 0.2 > 3.0
      local thickness = trunc(state.depth:sample2(wx * 0.0625, wz * 0.0625) / 3.0 + 3.0 + random:next_double() * 0.25)
      local depth = -1
      local top = grass
      local filler = dirt
      for y = 127, 0, -1 do
        local current = event.chunk:get_block(x, y, z)
        if current == 0 then
          depth = -1
        elseif current == stone then
          if depth == -1 then
            if thickness <= 0 then
              top = 0
              filler = stone
            elseif y >= 60 and y <= 65 then
              top = grass
              filler = dirt
              if gravelly then
                top = 0
                filler = gravel
              end
              if sandy then
                top = sand
                filler = sand
              end
            end
            if y < 64 and top == 0 then top = water end
            depth = thickness
            event.chunk:set_block(x, y, z, y >= 63 and top or filler)
          elseif depth > 0 then
            depth = depth - 1
            event.chunk:set_block(x, y, z, filler)
          end
        end
      end
    end
  end
end

local function in_target(wx, wz, target_x, target_z)
  local min_x = target_x * 16
  local min_z = target_z * 16
  return wx >= min_x and wx < min_x + 16 and wz >= min_z and wz < min_z + 16
end

local function set_target(chunk, target_x, target_z, wx, y, wz, block, replace)
  if y < 0 or y >= 128 or not in_target(wx, wz, target_x, target_z) then
    return
  end
  local x = wx - target_x * 16
  local z = wz - target_z * 16
  if replace == nil or replace[chunk:get_block(x, y, z)] then
    chunk:set_block(x, y, z, block)
  end
end

local function vein(random, chunk, target_x, target_z, x, y, z, ore, stone)
  local angle = random:next_float() * PI
  local x1 = x + 8 + math.sin(angle) * 2.0
  local x2 = x + 8 - math.sin(angle) * 2.0
  local z1 = z + 8 + math.cos(angle) * 2.0
  local z2 = z + 8 - math.cos(angle) * 2.0
  local y1 = y + random:next_int(3) + 2
  local y2 = y + random:next_int(3) + 2
  for step = 0, 16 do
    local cx = lerp(step / 16.0, x1, x2)
    local cy = lerp(step / 16.0, y1, y2)
    local cz = lerp(step / 16.0, z1, z2)
    local size = (math.sin(step / 16.0 * PI) + 1.0) * random:next_double() + 1.0
    local radius = size / 2.0
    for wx = trunc(cx - radius), trunc(cx + radius) do
      for wy = trunc(cy - radius), trunc(cy + radius) do
        for wz = trunc(cz - radius), trunc(cz + radius) do
          local dx = (wx + 0.5 - cx) / radius
          local dy = (wy + 0.5 - cy) / radius
          local dz = (wz + 0.5 - cz) / radius
          if dx * dx + dy * dy + dz * dz < 1.0 then
            set_target(chunk, target_x, target_z, wx, wy, wz, ore, { [stone] = true })
          end
        end
      end
    end
  end
end

local function draw_line(chunk, target_x, target_z, from, to, block)
  local dx = to[1] - from[1]
  local dy = to[2] - from[2]
  local dz = to[3] - from[3]
  local steps = math.max(math.abs(dx), math.abs(dy), math.abs(dz))
  if steps == 0 then
    set_target(chunk, target_x, target_z, from[1], from[2], from[3], block)
    return
  end
  for step = 0, steps do
    local amount = step / steps
    local x = math.floor(from[1] + dx * amount + 0.5)
    local y = math.floor(from[2] + dy * amount + 0.5)
    local z = math.floor(from[3] + dz * amount + 0.5)
    set_target(chunk, target_x, target_z, x, y, z, block)
  end
end

local function foliage_radius(layer)
  if layer < 0 or layer >= 5 then return -1.0 end
  if layer == 0 or layer == 4 then return 2.0 end
  return 3.0
end

local function layer_size(height, layer)
  if layer < height * 0.3 then return -1.618 end
  local half = height / 2.0
  local offset = half - layer
  if offset == 0.0 then return half * 0.5 end
  if math.abs(offset) >= half then return 0.0 end
  return math.sqrt(half * half - math.abs(offset) * math.abs(offset)) * 0.5
end

local function render_big_tree(chunk, target_x, target_z, root_x, root_y, root_z, height, random, log, leaves)
  local trunk_height = trunc(height * 0.618)
  if trunk_height >= height then trunk_height = height - 1 end
  local node_count = trunc(1.382 + (height / 13.0) ^ 2.0)
  if node_count <= 0 then node_count = 1 end
  local top_y = root_y + height - 5
  local trunk_top = root_y + trunk_height
  local nodes = { { root_x, top_y, root_z, trunk_top } }
  local layer = top_y - root_y
  local y = top_y - 1
  while layer >= 0 do
    local radius = layer_size(height, layer)
    if radius >= 0.0 then
      for _ = 1, node_count do
        local distance = radius * (random:next_float() + 0.328)
        local angle = random:next_float() * 2.0 * SIN_PI
        local node_x = trunc(distance * math.sin(angle) + root_x + 0.5)
        local node_z = trunc(distance * math.cos(angle) + root_z + 0.5)
        local horizontal = math.sqrt((root_x - node_x) ^ 2.0 + (root_z - node_z) ^ 2.0)
        local branch_y = y - horizontal * 0.381 > trunk_top and trunk_top or trunc(y - horizontal * 0.381)
        nodes[#nodes + 1] = { node_x, y, node_z, branch_y }
      end
    end
    y = y - 1
    layer = layer - 1
  end
  local replace_leaves = { [0] = true, [leaves] = true }
  for _, node in ipairs(nodes) do
    for leaf_y = node[2], node[2] + 4 do
      local radius = foliage_radius(leaf_y - node[2])
      local bound = trunc(radius + 0.618)
      for dx = -bound, bound do
        for dz = -bound, bound do
          local distance = math.sqrt((math.abs(dx) + 0.5) ^ 2.0 + (math.abs(dz) + 0.5) ^ 2.0)
          if distance <= radius then
            set_target(chunk, target_x, target_z, node[1] + dx, leaf_y, node[3] + dz, leaves, replace_leaves)
          end
        end
      end
    end
  end
  draw_line(chunk, target_x, target_z, { root_x, root_y, root_z }, { root_x, trunk_top, root_z }, log)
  for _, node in ipairs(nodes) do
    if node[4] - root_y >= height * 0.2 then
      draw_line(chunk, target_x, target_z, { root_x, node[4], root_z }, { node[1], node[2], node[3] }, log)
    end
  end
end

local function populate_source(event, state, source_x, source_z)
  local random = Random.new(source_x * 318279123 + source_z * 919871212)
  local origin_x = source_x * 16
  local origin_z = source_z * 16
  local stone = block_id("stone")
  local coal = block_id("coal_ore")
  local iron = block_id("iron_ore")
  local gold = block_id("gold_ore")
  local diamond = block_id("diamond_ore")
  local log = block_id("log")
  local leaves = block_id("leaves")
  for _ = 1, 20 do
    vein(random, event.chunk, event.chunk_x, event.chunk_z, origin_x + random:next_int(16), random:next_int(128), origin_z + random:next_int(16), coal, stone)
  end
  for _ = 1, 10 do
    vein(random, event.chunk, event.chunk_x, event.chunk_z, origin_x + random:next_int(16), random:next_int(64), origin_z + random:next_int(16), iron, stone)
  end
  if random:next_int(2) == 0 then
    vein(random, event.chunk, event.chunk_x, event.chunk_z, origin_x + random:next_int(16), random:next_int(32), origin_z + random:next_int(16), gold, stone)
  end
  if random:next_int(8) == 0 then
    vein(random, event.chunk, event.chunk_x, event.chunk_z, origin_x + random:next_int(16), random:next_int(16), origin_z + random:next_int(16), diamond, stone)
  end
  local tree_count = trunc(state.forest:sample2(origin_x * 0.25, origin_z * 0.25)) * 8
  local tree_height
  for _ = 1, math.max(0, tree_count) do
    local root_x = origin_x + random:next_int(16) + 8
    local root_z = origin_z + random:next_int(16) + 8
    local tree_random = Random.new(random:next_long())
    if tree_height == nil then
      tree_height = 5 + tree_random:next_int(12)
    end
    local chunk_x = root_x // 16
    local chunk_z = root_z // 16
    local heights = heights_at(state, chunk_x, chunk_z)
    local root_y = heights[(root_x - chunk_x * 16) * 16 + root_z - chunk_z * 16] + 1
    if root_y >= 64 and root_y + tree_height < 128 then
      render_big_tree(event.chunk, event.chunk_x, event.chunk_z, root_x, root_y, root_z, tree_height, tree_random, log, leaves)
    end
  end
end

local function features(event, state)
  for source_x = event.chunk_x - 1, event.chunk_x do
    for source_z = event.chunk_z - 1, event.chunk_z do
      populate_source(event, state, source_x, source_z)
    end
  end
end

return {
  id = "infdev_20100415",
  label = "Infdev 20100415",
  order = 50,
  cancel = { terrain = true, surface = true, carver = true, features = true },
  spawn = function()
    return 64, nil
  end,
  generate = function(event)
    local state = state_for(event.world_seed)
    if event.stage == minecraft.generation.stages.terrain then
      terrain(event, state)
    elseif event.stage == minecraft.generation.stages.surface then
      surface(event, state)
    elseif event.stage == minecraft.generation.stages.features then
      features(event, state)
    end
  end,
}
