-- ================================================================
-- CONFIG: void_fog
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("void_fog", {
  name = "Void Fog",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Void Fog",
      default = true,
    },
    density = {
      type = "slider",
      label = "Void Fog Density",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.5,
    },
  },
})

return config
