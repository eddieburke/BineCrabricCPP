-- ================================================================
-- CONFIG: coral
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("coral", {
  name = "Coral",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Coral",
      default = true,
    },
  },
})

return config
