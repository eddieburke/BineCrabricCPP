-- ================================================================
-- MOD: camera
-- Standardized structure with separated config
-- ================================================================

local config = require("camera.config")

local state = {
  config = config,
  prev_yaw = 0,
  prev_pitch = 0,
  smooth_yaw = 0,
  smooth_pitch = 0,
  rotation_angle = 0,
}

local function lerp(a, b, t)
  return a + (b - a) * t
end

local function wrap_angle(angle)
  while angle > 180 do angle = angle - 360 end
  while angle < -180 do angle = angle + 360 end
  return angle
end

minecraft.event.register("tick", function(dt)
  if state.config.auto_rotate then
    state.rotation_angle = state.rotation_angle + dt * (state.config.rotate_speed or 5.0)
  end
end)

minecraft.event.register("fov", function(event)
  local target_fov = state.config.fov or 70.0
  event.fov = target_fov
end)

minecraft.event.register("render", function(camera)
  if camera then
    local smoothness = state.config.smoothness or 0.1
    state.smooth_yaw = lerp(state.smooth_yaw, camera.yaw or 0, smoothness)
    state.smooth_pitch = lerp(state.smooth_pitch, camera.pitch or 0, smoothness)
  end
  if state.config.auto_rotate then
    local rot = state.rotation_angle * math.pi / 180
  end
end)

local camera_model = minecraft.model and minecraft.model.load("models/camera/camera.json")
local tv_model = minecraft.model and minecraft.model.load("models/camera/tv/tv.json")
local tripod_model = minecraft.model and minecraft.model.load("models/camera/tripod_bottom/tripod_bottom.json")

minecraft.register_item({
  id = 400,
  name = "Camera",
  translation_key = "camera",
  texture = "mods/camera/camera.png",
  model = camera_model,
  max_count = 1,
})

minecraft.register_block({
  id = 185,
  name = "Camera",
  translation_key = "cameraBlock",
  texture = "mods/camera/camera.png",
  model = camera_model,
  opaque = false,
  full_cube = false,
})

minecraft.register_block({
  id = 186,
  name = "TV",
  translation_key = "tvBlock",
  texture = "mods/camera/tv.png",
  model = tv_model,
  opaque = false,
  full_cube = false,
})

minecraft.register_block({
  id = 187,
  name = "Tripod",
  translation_key = "tripodBlock",
  texture = "mods/camera/tripod.png",
  model = tripod_model,
  opaque = false,
  full_cube = false,
})

minecraft.log("info", "Camera mod loaded (refactored)")
