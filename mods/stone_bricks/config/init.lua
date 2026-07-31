local settings = require("lib.settings")

local config = settings.define("stone_bricks", {
  name = "Stone Bricks",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Stone Bricks",
      default = true,
    },
  },
})

return config
