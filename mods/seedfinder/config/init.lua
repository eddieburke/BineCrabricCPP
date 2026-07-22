-- ================================================================
-- CONFIG: seedfinder
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("seedfinder", {
  name = "Seed Finder",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Seed Finder",
      default = true,
    },
  },
})

return config
