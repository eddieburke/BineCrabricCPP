local common = require("scripts.common")

return {
  id = "highlands",
  label = "Highlands (Custom)",
  order = 30,
  dimensions = { 0 },
  options = {
    {
      key = "highland_boost",
      label = "Highland Boost",
      min = 4,
      max = 40,
      integer = true,
      default = 16,
    },
  },
  spawn = function()
    return 81, nil
  end,
  after = {
    surface = function(event, options)
      local grass_id = common.block_id("grass_block")
      local dirt_id = common.block_id("dirt")
      local stone_id = common.block_id("stone")
      local chunk = event.chunk
      local seed = event.world_seed
      local boost_base = options.highland_boost
      for x = 0, 15 do
        for z = 0, 15 do
          local surface = math.max(0, chunk:get_height(x, z) - 1)
          local wx = event.chunk_x * 16 + x
          local wz = event.chunk_z * 16 + z
          local boost = boost_base + (common.hash_coord(wx, wz, seed) % 20)
          local top = math.min(118, surface + boost)
          if top > surface then
            chunk:fill(x, surface + 1, z, x, top, z, stone_id)
          end
          chunk:set_block(x, top, z, grass_id)
          if top - 1 > 0 then chunk:set_block(x, top - 1, z, dirt_id) end
          if top - 2 > 0 then chunk:set_block(x, top - 2, z, dirt_id) end
          if top - 3 > 0 then chunk:set_block(x, top - 3, z, dirt_id) end
        end
      end
    end,
  },
}
