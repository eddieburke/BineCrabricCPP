local LOG = minecraft.world.block_id("log")
local LEAVES = 18

local MAX_LOGS = 96
local MAX_RADIUS = 6
local MAX_HEIGHT = 20
local MIN_HEIGHT = 4
local MIN_TRUNK_COLUMN = 3
local MIN_LEAVES = 10

local NATURAL_NEIGHBORS = {
  [0] = true, [1] = true, [2] = true, [3] = true, [6] = true,
  [8] = true, [9] = true, [12] = true, [13] = true, [17] = true,
  [18] = true, [31] = true, [32] = true, [37] = true, [38] = true,
  [39] = true, [40] = true, [78] = true, [79] = true, [80] = true,
  [82] = true,
}

local GROUND = {[2] = true, [3] = true}

local function key(x, y, z)
  return x .. ":" .. y .. ":" .. z
end

local function analyze(event)
  local world = minecraft.world
  local ex, ey, ez = event.x, event.y, event.z
  local type_bits = (event.block_meta or 0) & 3

  local function is_log(x, y, z)
    if x == ex and y == ey and z == ez then return true end
    return world.get_block(x, y, z) == LOG
  end

  local by = ey
  while is_log(ex, by - 1, ez) do
    by = by - 1
    if ey - by > MAX_HEIGHT then return nil end
  end
  if not GROUND[world.get_block(ex, by - 1, ez)] then return nil end

  local bx, bz = ex, ez
  local logs = {}
  local seen = {[key(bx, by, bz)] = true}
  local queue = {{bx, by, bz}}
  local first = 1
  local top = by

  while first <= #queue do
    local p = queue[first]
    first = first + 1
    local px, py, pz = p[1], p[2], p[3]
    logs[#logs + 1] = p
    if py > top then top = py end
    if #logs > MAX_LOGS then return nil end
    for dy = -1, 1 do
      for dz = -1, 1 do
        for dx = -1, 1 do
          if dx ~= 0 or dy ~= 0 or dz ~= 0 then
            local nx, ny, nz = px + dx, py + dy, pz + dz
            local k = key(nx, ny, nz)
            if not seen[k] and is_log(nx, ny, nz) then
              seen[k] = true
              if math.abs(nx - bx) > MAX_RADIUS or math.abs(nz - bz) > MAX_RADIUS or
                 ny < by or ny > by + MAX_HEIGHT then
                return nil
              end
              local meta = (nx == ex and ny == ey and nz == ez) and (event.block_meta or 0)
                or world.get_block_meta(nx, ny, nz)
              if (meta & 3) ~= type_bits then return nil end
              queue[#queue + 1] = {nx, ny, nz}
            end
          end
        end
      end
    end
  end

  if not seen[key(ex, ey, ez)] then return nil end
  if top - by + 1 < MIN_HEIGHT then return nil end

  for h = 0, MIN_TRUNK_COLUMN - 1 do
    if not seen[key(bx, by + h, bz)] then return nil end
  end

  local upper_from = by + (top - by + 1) // 2
  local leaves = {}
  local leaf_count = 0
  local crown = false
  for i = 1, #logs do
    local p = logs[i]
    local px, py, pz = p[1], p[2], p[3]
    for dy = -1, 1 do
      for dz = -1, 1 do
        for dx = -1, 1 do
          if dx ~= 0 or dy ~= 0 or dz ~= 0 then
            local nx, ny, nz = px + dx, py + dy, pz + dz
            if not seen[key(nx, ny, nz)] then
              local id = world.get_block(nx, ny, nz)
              if not NATURAL_NEIGHBORS[id] then return nil end
              if id == LEAVES and py >= upper_from then
                local k = key(nx, ny, nz)
                if not leaves[k] and (world.get_block_meta(nx, ny, nz) & 3) == type_bits then
                  leaves[k] = true
                  leaf_count = leaf_count + 1
                  if ny >= top - 1 then crown = true end
                end
              end
            end
          end
        end
      end
    end
  end

  if leaf_count < MIN_LEAVES or not crown then return nil end
  return logs
end

minecraft.on("block_break", {
  block_id = LOG,
  remote = false,
  has_player = true,
  has_item = true,
  item_is_axe = true,
}, function(event)
  local logs = analyze(event)
  if not logs then return end
  table.sort(logs, function(a, b) return a[2] < b[2] end)
  for i = 1, #logs do
    local p = logs[i]
    if p[2] >= event.y and not (p[1] == event.x and p[2] == event.y and p[3] == event.z) then
      if not minecraft.world.harvest_block(p[1], p[2], p[3], event.item_id) then
        return
      end
    end
  end
end)
