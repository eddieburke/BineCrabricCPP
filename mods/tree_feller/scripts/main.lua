local log_id = minecraft.world.block_id("log")
local max_extra_logs = 128

local function position_key(x, y, z)
  return x .. ":" .. y .. ":" .. z
end

minecraft.on("block_break", {
  block_id = log_id,
  remote = false,
  has_player = true,
  has_item = true,
  item_is_axe = true,
}, function(event)
  local queue = {}
  local seen = {}
  local first = 1
  local felled = 0
  local minimum_y = event.y
  local log_meta = event.block_meta or 0

  local function enqueue(x, y, z)
    if y < minimum_y then return end
    local key = position_key(x, y, z)
    if seen[key] then return end
    seen[key] = true
    queue[#queue + 1] = {x = x, y = y, z = z}
  end

  seen[position_key(event.x, event.y, event.z)] = true
  for dy = 0, 1 do
    for dz = -1, 1 do
      for dx = -1, 1 do
        enqueue(event.x + dx, event.y + dy, event.z + dz)
      end
    end
  end

  while first <= #queue and felled < max_extra_logs do
    local position = queue[first]
    first = first + 1
    if minecraft.world.get_block(position.x, position.y, position.z) == log_id and
       minecraft.world.get_block_meta(position.x, position.y, position.z) == log_meta then
      if not minecraft.world.harvest_block(position.x, position.y, position.z, event.item_id) then
        break
      end
      felled = felled + 1
      for dy = -1, 1 do
        for dz = -1, 1 do
          for dx = -1, 1 do
            enqueue(position.x + dx, position.y + dy, position.z + dz)
          end
        end
      end
    end
  end
end)
