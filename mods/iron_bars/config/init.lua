local settings = require("lib.settings")

local config = settings.define("iron_bars", {
  name = "Iron Bars",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Iron Bars",
      default = true,
    },
  },
})

return config
