local common = minecraft.require("scripts.common")

return {
  id = "flatlands",
  label = "Flatlands",
  order = 20,
  cancel = { carver = true, features = true },
  options = {
    {
      key = "flat_height",
      save_key = "world_profiles:flat_height",
      label = "Flat Height",
      min = 1,
      max = 32,
      integer = true,
      default = 4,
    },
  },
  decorate = function(event, options)
    local grass_id = common.block_id("grass_block")
    local dirt_id = common.block_id("dirt")
    local bedrock_id = common.block_id("bedrock")
    if grass_id <= 0 or dirt_id <= 0 then
      return
    end
    local chunk = event.chunk
    local height = math.max(1, math.min(64, math.floor(options.flat_height + 0.5)))
    chunk:fill(0, height, 0, 15, 127, 15, common.AIR_ID)
    if height >= 2 then
      chunk:fill(0, 1, 0, 15, height - 1, 15, dirt_id)
    end
    chunk:fill(0, height, 0, 15, height, 15, grass_id)
    if bedrock_id > 0 then
      chunk:fill(0, 0, 0, 15, 0, 15, bedrock_id)
    end
  end,
}
