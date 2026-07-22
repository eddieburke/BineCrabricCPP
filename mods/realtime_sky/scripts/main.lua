-- Realtime Sky - Refactored Main Module
-- Thin initialization layer with clear separation of concerns

local config = require("realtime_sky.config.init")
local astronomy = require("realtime_sky.astronomy.calculator")
local skybox = require("realtime_sky.rendering.skybox")
local celestial = require("realtime_sky.rendering.celestial")
local core = require("lib.core.init")

local SETTINGS_SCREEN_ID = "realtime_sky:settings"
local SCREEN_ID = "realtime_sky:globe"
local CONFIG_FILE = "realtime_sky.txt"
local SKY_PROVIDER_PRIORITY = 50

-- Initialize settings from config defaults
local settings = core.util_copy(config.DEFAULTS)

-- UI state (separated from logic)
local ui_state = {
  search = "",
  list_scroll = 0,
  selected_index = 1,
  filtered = {},
  globe_x = 10,
  globe_y = 32,
  globe_size = 220,
  globe_yaw = 0.0,
  globe_pitch = 0.0,
  globe_cam = 2.05,
  dragging = false,
  press_x = 0,
  press_y = 0,
  drag_last_x = 0,
  drag_last_y = 0,
  coast_loaded = false,
}

--------------------------------------------------------------------------------
-- EVENT HANDLERS
--------------------------------------------------------------------------------

local function on_render_overlay(event)
  if not settings.enabled then return end
  
  local frame = event.frame
  if not frame then return end
  
  -- Draw sky dome
  skybox.draw_sky_dome(event, frame)
  
  -- Calculate sun alpha based on altitude
  local sun_alpha = math.max(0, math.min(1, (frame.sun_altitude_deg + 8) / 20))
  if sun_alpha > 0.01 then
    celestial.draw_procedural_sun(event, frame, sun_alpha)
  end
end

local function on_world_tick(event)
  -- Update astronomical calculations if needed
  if settings.drive_sun then
    -- Astronomy calculations handled by frame data
  end
end

--------------------------------------------------------------------------------
-- INITIALIZATION
--------------------------------------------------------------------------------

local function init()
  -- Register event handlers using standardized event bus
  core.register_event("render_overlay", on_render_overlay, SKY_PROVIDER_PRIORITY)
  core.register_event("world_tick", on_world_tick)
  
  -- Load saved settings
  -- (Settings loading logic would go here)
  
  print("[realtime_sky] Initialized with separated architecture")
end

init()
