-- ================================================================
-- CONFIG: repair_table
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("repair_table", {
  name = "Repair Table",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable Repair Table",
      default = true,
    },
    max_repair_cost = {
      type = "slider",
      label = "Max Repair Cost",
      min = 1,
      max = 100,
      step = 1,
      default = 40,
    },
  },
})

return config
