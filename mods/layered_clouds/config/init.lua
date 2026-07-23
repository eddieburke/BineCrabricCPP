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
    layer_count = {
      type = "slider",
      label = "Layers",
      min = 1,
      max = 12,
      step = 1,
      integer = true,
      default = 7,
    },
    base_opacity = {
      type = "slider",
      label = "Base Opacity",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.7,
    },
    cloud_scale = {
      type = "slider",
      label = "Cloud Scale",
      min = 0.5,
      max = 2,
      step = 0.01,
      default = 1.0,
    },
    layer_height_spacing = {
      type = "slider",
      label = "Layer Spacing",
      min = 4,
      max = 32,
      step = 1,
      integer = true,
      default = 12,
    },
    wind_speed = {
      type = "slider",
      label = "Wind Speed",
      min = 0,
      max = 5,
      step = 0.1,
      default = 1.0,
    },
  },
})

return config
