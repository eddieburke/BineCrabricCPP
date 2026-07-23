-- ================================================================
-- CONFIG: offline_mode
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("offline_mode", {
  name = "Offline Mode",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Offline Mode",
      default = false,
    },
  },
})

return config
