local common = require("scripts.common")

return {
  id = "flatlands",
  label = "Flatlands (Custom)",
  order = 20,
  dimensions = { 0 },
  options = {
    {
      key = "flat_height",
      label = "Grass Height",
      min = 1,
      max = 32,
      integer = true,
      default = 3,
    },
  },
  spawn = function(options)
    return options.flat_height + 1, options.flat_height + 1
  end,
  stages = {
    surface = false,
    carver = false,
    features = false,
    terrain = function(event, options)
      local grass_id = common.block_id("grass_block")
      local dirt_id = common.block_id("dirt")
      local bedrock_id = common.block_id("bedrock")
      local chunk = event.chunk
      local height = options.flat_height
      chunk:fill(0, 0, 0, 15, 127, 15, common.AIR_ID)
      if height >= 2 then
        chunk:fill(0, 1, 0, 15, height - 1, 15, dirt_id)
      end
      chunk:fill(0, height, 0, 15, height, 15, grass_id)
      chunk:fill(0, 0, 0, 15, 0, 15, bedrock_id)
    end,
  },
}
