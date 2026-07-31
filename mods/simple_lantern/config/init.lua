local settings = require("lib.settings")

local config = settings.define("simple_lantern", {
  name = "Simple Lantern",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Simple Lantern",
      default = true,
    },
  },
})

return config
