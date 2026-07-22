-- ================================================================
-- CONFIG: fog_settings
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("fog_settings", {
  name = "Fog Settings",
  fields = {
    enabled = {
      type = "bool",
      label = "Custom Fog",
      default = false,
    },
    spherical = {
      type = "bool",
      label = "Spherical Projection",
      default = true,
    },
    start = {
      type = "slider",
      label = "Fog Start",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.2,
    },
    exponential = {
      type = "bool",
      label = "Exponential Fog",
      default = false,
    },
    end_val = {
      type = "slider",
      label = "Fog End",
      key = "end",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.8,
    },
    density = {
      type = "slider",
      label = "Fog Density",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.1,
    },
    custom_color = {
      type = "bool",
      label = "Custom Fog Color",
      default = false,
    },
    red = {
      type = "slider",
      label = "Fog Red",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0,
    },
    green = {
      type = "slider",
      label = "Fog Green",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0,
    },
    blue = {
      type = "slider",
      label = "Fog Blue",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0,
    },
  },
})

return config
