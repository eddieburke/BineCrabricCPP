local settings = require("lib.settings")

local config = settings.define("critical_hit", {
  name = "Critical Hit",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Critical Hits",
      default = true,
    },
  },
})

return config
