-- ================================================================
-- CONFIG: world_profiles
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("world_profiles", {
  name = "World Profiles",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable World Profiles",
      default = true,
    },
  },
})

return config
