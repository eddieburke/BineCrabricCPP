-- ================================================================
-- CONFIG: colorful_skies
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("colorful_skies", {
  name = "Colorful Skies",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Colorful Skies",
      default = true,
    },
    dynamic_colors = {
      type = "bool",
      label = "Daily Twilight Colors",
      default = false,
    },
    day_sky_r = {
      type = "slider",
      label = "Day Sky Red",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.36,
    },
    day_sky_g = {
      type = "slider",
      label = "Day Sky Green",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.62,
    },
    day_sky_b = {
      type = "slider",
      label = "Day Sky Blue",
      min = 0,
      max = 1,
      step = 0.01,
      default = 1.0,
    },
    twilight_strength = {
      type = "slider",
      label = "Twilight Strength",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.35,
    },
    night_brightness = {
      type = "slider",
      label = "Night Brightness",
      min = 0.05,
      max = 0.6,
      step = 0.01,
      default = 0.18,
    },
    night_sky_r = {
      type = "slider",
      label = "Night Sky Red",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.02,
    },
    night_sky_g = {
      type = "slider",
      label = "Night Sky Green",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.02,
    },
    night_sky_b = {
      type = "slider",
      label = "Night Sky Blue",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.08,
    },
    fog_blend = {
      type = "slider",
      label = "Fog Color Blend",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.35,
    },
  },
})

return config
