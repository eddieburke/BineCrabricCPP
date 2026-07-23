-- Sprint: Double-tap and keyhold sprinting with FOV effects
-- Refactored with separation of concerns

local config = require("sprint.config")

-- Module state
local sprinting = false
local sprinting_by_key = false
local was_forward_down = false
local sprint_toggle_timer = 0
local sprint_boost_timer = 0
local current_fov_multiplier = 1.0

-- Input helpers
local function key_code(name, fallback)
  local code = minecraft.key_code(name)
  if code ~= nil and code ~= 0 then
    return code
  end
  return fallback
end

local function forward_down()
  return minecraft.is_key_down(key_code("forward", 17))
end

local function sprint_key_down()
  return minecraft.is_key_down(key_code("sprint", config.SPRINT_KEY_FALLBACK))
end

-- Sprint state management
local function start_sprinting(by_key)
  if sprinting then return end
  sprinting = true
  sprinting_by_key = by_key
  sprint_boost_timer = config.double_tap_window_ticks or 4
end

local function stop_sprinting()
  sprinting = false
  sprinting_by_key = false
  sprint_boost_timer = 0
end

local function update_sprint_state()
  local forward = forward_down()
  local sprint_key_active = sprint_key_down()

  if forward and not was_forward_down then
    if sprint_toggle_timer > 0 then
      start_sprinting(false)
    else
      sprint_toggle_timer = config.double_tap_window_ticks or 7
    end
  end

  if sprint_key_active and forward and not sprinting then
    start_sprinting(true)
  end

  if sprinting and ((not forward) or (sprinting_by_key and not sprint_key_active)) then
    stop_sprinting()
  end

  was_forward_down = forward
  if sprint_toggle_timer > 0 then
    sprint_toggle_timer = sprint_toggle_timer - 1
  end
end

minecraft.event.register("tick", function(dt)
  update_sprint_state()
  if sprint_boost_timer > 0 then
    sprint_boost_timer = sprint_boost_timer - 1
  end
  
  -- FOV transition
  local target = sprinting and (config.sprint_fov_multiplier or 1.08) or 1.0
  local lerp_rate = config.fov_lerp_rate or 8.0
  local t = 1.0 - math.exp(-lerp_rate * dt)
  current_fov_multiplier = current_fov_multiplier + (target - current_fov_multiplier) * t
end)

minecraft.event.register("player_move", function(event)
  if sprinting and event.forward > 0.0 then
    local multiplier = config.sprint_multiplier or 1.45
    if sprint_boost_timer > 0 then
      multiplier = multiplier * (config.start_boost_multiplier or 1.08)
    end
    event.speed_multiplier = (event.speed_multiplier or 1.0) * multiplier
  end
end)

minecraft.event.register("fov", function(event)
  event.fov = (tonumber(event.fov) or 70.0) * current_fov_multiplier
end)

minecraft.log("info", "Sprint mod loaded (refactored with lib.settings)")
