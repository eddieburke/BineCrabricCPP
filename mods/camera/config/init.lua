-- ================================================================
-- CONFIG: camera
-- Centralized configuration with validation and persistence
-- ================================================================

local settings = require("lib.settings")

local config = settings.define("camera", {
  name = "Camera",
  fields = {
    auto_rotate = {
      type = "bool",
      label = "Auto Rotate Camera",
      default = false,
    },
    zoom_sensitivity = {
      type = "slider",
      label = "Zoom Sensitivity",
      min = 0.1,
      max = 5,
      step = 0.1,
      default = 1.0,
    },
  },
})

return config
