-- Sprint Configuration
local settings = require("lib.settings")

local cfg = settings.define("sprint", {
  name = "Sprint",
  fields = {
    double_tap_window_ticks = { type = "int", label = "Double-Tap Window (ticks)", min = 3, max = 15, default = 7 },
    sprint_multiplier = { type = "slider", label = "Sprint Speed Multiplier", min = 1.0, max = 2.0, step = 0.01, default = 1.45 },
    start_boost_multiplier = { type = "slider", label = "Start Boost Multiplier", min = 1.0, max = 1.5, step = 0.01, default = 1.08 },
    start_boost_ticks = { type = "int", label = "Start Boost Duration (ticks)", min = 0, max = 10, default = 4 },
    sprint_fov_multiplier = { type = "slider", label = "Sprint FOV Multiplier", min = 1.0, max = 1.3, step = 0.01, default = 1.08 },
    fov_lerp_rate = { type = "slider", label = "FOV Transition Speed", min = 1.0, max = 20.0, step = 0.5, default = 8.0 },
  }
})

cfg.SPRINT_KEY_FALLBACK = 29

return cfg
