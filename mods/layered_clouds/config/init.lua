-- ================================================================
-- CONFIG: layered_clouds
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("layered_clouds", {
  name = "Layered Clouds",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Layered Clouds",
      default = true,
    },
    cloud_height = {
      type = "slider",
      label = "Cloud Height",
      min = 64,
      max = 256,
      step = 1,
      default = 128,
    },
    cloud_speed = {
      type = "slider",
      label = "Cloud Speed",
      min = 0,
      max = 5,
      step = 0.1,
      default = 1.0,
    },
    density = {
      type = "slider",
      label = "Cloud Density",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.5,
    },
  },
})

return config
