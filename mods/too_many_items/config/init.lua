-- ================================================================
-- CONFIG: too_many_items
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("too_many_items", {
  name = "Too Many Items",
  fields = {
    enabled = {
      type = "bool",
      label = "Enable TMI",
      default = true,
    },
    items_per_page = {
      type = "slider",
      label = "Items Per Page",
      min = 9,
      max = 108,
      step = 9,
      default = 54,
    },
  },
})

return config
