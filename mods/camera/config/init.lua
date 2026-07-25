local settings = require("lib.settings")

local config = settings.define("camera", {
  name = "Camera",
  fields = {
    screenshot_width = {
      type = "slider",
      label = "Screenshot Width",
      min = 160,
      max = 1920,
      step = 16,
      default = 640,
    },
    screenshot_height = {
      type = "slider",
      label = "Screenshot Height",
      min = 120,
      max = 1080,
      step = 16,
      default = 480,
    },
    renewable = {
      type = "bool",
      label = "Renewable (drop item on use)",
      default = true,
    },
  },
})

return config
