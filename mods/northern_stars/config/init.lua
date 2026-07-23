-- ================================================================
-- CONFIG: northern_stars
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("northern_stars", {
  name = "Northern Stars",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Northern Lights",
      default = true,
    },
    intensity = {
      type = "slider",
      label = "Aurora Intensity",
      min = 0,
      max = 1,
      step = 0.01,
      default = 0.8,
    },
    frequency = {
      type = "slider",
      label = "Aurora Frequency",
      min = 0,
      max = 2,
      step = 0.1,
      default = 0.5,
    },
  },
})

return config
