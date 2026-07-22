-- ================================================================
-- CONFIG: ravine_backport
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("ravine_backport", {
  name = "Ravine Backport",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Ravines",
      default = true,
    },
  },
})

return config
