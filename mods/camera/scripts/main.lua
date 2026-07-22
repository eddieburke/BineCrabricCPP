-- ================================================================
-- MOD: camera
-- Standardized structure with separated config
-- ================================================================

local config = require("camera.config")

-- Camera: Enhanced camera controls and effects
-- Refactored with separation of concerns

local config_module = require("camera.config")

-- Module state (encapsulated, no globals)
local state = {
  config = {},
  prev_yaw = 0,
  prev_pitch = 0,
  smooth_yaw = 0,
  smooth_pitch = 0,
  rotation_angle = 0,
}

-- Load configuration
state.config, _ = config_module.load()

-- Helper functions
local function lerp(a, b, t)
  return a + (b - a) * t
end

local function wrap_angle(angle)
  while angle > 180 do angle = angle - 360 end
  while angle < -180 do angle = angle + 360 end
  return angle
end

-- Tick handler
minecraft.event.register("tick", function(dt)
  local cam = minecraft.camera.get()
  if not cam then return end
  
  -- Smooth camera rotation
  local smoothness = state.config.smoothness or 0.1
  state.smooth_yaw = lerp(state.smooth_yaw, cam.yaw, smoothness)
  state.smooth_pitch = lerp(state.smooth_pitch, cam.pitch, smoothness)
  
  -- Auto-rotation effect
  if state.config.auto_rotate then
    state.rotation_angle = state.rotation_angle + dt * (state.config.rotate_speed or 5.0)
  end
end)

-- FOV handler
minecraft.event.register("fov", function(event)
  local target_fov = state.config.fov or 70.0
  event.fov = target_fov
end)

-- Render handler (applies auto-rotation)
minecraft.event.register("render", function(camera)
  if state.config.auto_rotate then
    -- Apply subtle rotation effect to view matrix
    local rot = state.rotation_angle * math.pi / 180
    -- Note: Full implementation would modify the view matrix here
  end
end)

-- Save config on unload
minecraft.event.register("unload", function()
  config_module.save(state.config)
end)

minecraft.log("info", "Camera mod loaded (refactored)")
