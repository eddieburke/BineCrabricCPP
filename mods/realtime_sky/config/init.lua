local settings = require("lib.settings")

local config = settings.define("realtime_sky", {
  name = "Realtime Sky",
  fields = {
    enabled = {
      type = "bool",
      label = "Enabled",
      default = false,
    },
    drive_sun = {
      type = "bool",
      label = "Realtime Sun",
      default = true,
    },
    show_simulate_panel = {
      type = "bool",
      label = "HUD panel",
      default = false,
    },
    override_enabled = {
      type = "bool",
      label = "Simulation",
      default = false,
    },
    simulate_date = {
      type = "bool",
      label = "Date override",
      default = false,
    },
    simulate_time = {
      type = "bool",
      label = "Time override",
      default = false,
    },
    sim_year = {
      type = "int",
      label = "Year",
      min = 1,
      max = 9999,
      default = 2000,
    },
    sim_month = {
      type = "int",
      label = "Month",
      min = 1,
      max = 12,
      default = 1,
    },
    sim_day = {
      type = "int",
      label = "Day",
      min = 1,
      max = 31,
      default = 1,
    },
    sim_hour = {
      type = "int",
      label = "Hour",
      min = 0,
      max = 23,
      default = 0,
    },
    sim_minute = {
      type = "int",
      label = "Minute",
      min = 0,
      max = 59,
      default = 0,
    },
    latitude = {
      type = "slider",
      label = "Latitude",
      min = -90,
      max = 90,
      step = 1,
      default = 45.0,
    },
    longitude = {
      type = "slider",
      label = "Longitude",
      min = -180,
      max = 180,
      step = 1,
      default = 0.0,
    },
    time_zone_id = {
      type = "string",
      label = "Time Zone",
      default = "GMT+0",
    },
  },
})

return config
