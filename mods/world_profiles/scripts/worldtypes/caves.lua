local common = minecraft.require("scripts.common")

return {
  id = "caves",
  label = "Caves",
  order = 40,
  options = {
    {
      key = "cave_min_y",
      save_key = "world_profiles:cave_min_y",
      label = "Cave Spawn Min Y",
      min = 8,
      max = 80,
      integer = true,
      default = 40,
    },
    {
      key = "cave_max_y",
      save_key = "world_profiles:cave_max_y",
      label = "Cave Spawn Max Y",
      min = 16,
      max = 100,
      integer = true,
      default = 72,
    },
  },
  spawn = function(options)
    local min_y = options.cave_min_y
    local max_y = options.cave_max_y
    if min_y > max_y then
      min_y, max_y = max_y, min_y
    end
    return min_y, max_y
  end,
  decorate = function(event)
    local grass_id = common.block_id("grass_block")
    local dirt_id = common.block_id("dirt")
    if grass_id <= 0 or dirt_id <= 0 then
      return
    end
    local chunk = event.chunk
    local seed = math.floor(event.world_seed or 0)
    for x = 0, 15 do
      for z = 0, 15 do
        local surface = math.max(0, chunk:get_height(x, z) - 1)
        local top = math.max(48, surface - 8)
        if surface > top then
          chunk:fill(x, top + 1, z, x, surface, z, common.AIR_ID)
        end
        chunk:set_block(x, top, z, grass_id)
        if top - 1 > 0 then chunk:set_block(x, top - 1, z, dirt_id) end
        if top - 2 > 0 then chunk:set_block(x, top - 2, z, dirt_id) end
        if top - 3 > 0 then chunk:set_block(x, top - 3, z, dirt_id) end
        local wx = event.chunk_x * 16 + x
        local wz = event.chunk_z * 16 + z
        local pocket = common.hash_coord(wx, wz, seed + 17) % 11
        if pocket < 2 then
          local cy = math.max(8, top - 6 - (pocket * 3))
          for dy = -2, 2 do
            for dx = -1, 1 do
              for dz = -1, 1 do
                local lx = x + dx
                local lz = z + dz
                if lx >= 0 and lx <= 15 and lz >= 0 and lz <= 15 then
                  chunk:set_block(lx, cy + dy, lz, common.AIR_ID)
                end
              end
            end
          end
        end
      end
    end
  end,
}
