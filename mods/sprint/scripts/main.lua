local SPRINT_KEY_FALLBACK = 29

minecraft.settings.register("Sprint", {
  { key = "double_tap_window_ticks", label = "Double-Tap Window (ticks)", kind = "slider", min = 3, max = 15, integer = true, default = 7 },
  { key = "sprint_multiplier", label = "Sprint Speed Multiplier", kind = "slider", min = 1.0, max = 2.0, step = 0.01, default = 1.45 },
  { key = "start_boost_multiplier", label = "Start Boost Multiplier", kind = "slider", min = 1.0, max = 1.5, step = 0.01, default = 1.08 },
  { key = "start_boost_ticks", label = "Start Boost Duration (ticks)", kind = "slider", min = 0, max = 10, integer = true, default = 4 },
  { key = "sprint_fov_multiplier", label = "Sprint FOV Multiplier", kind = "slider", min = 1.0, max = 1.3, step = 0.01, default = 1.08 },
  { key = "fov_lerp_rate", label = "FOV Transition Speed", kind = "slider", min = 1.0, max = 20.0, step = 0.5, default = 8.0 },
})

local sprinting = false
local sprinting_by_key = false
local was_forward_down = false
local sprint_toggle_timer = 0
local sprint_boost_timer = 0

local prev_fov_multiplier = 1.0
local current_fov_multiplier = 1.0

local function key_code(name, fallback)
  local code = minecraft.key_code(name)
  if code ~= nil and code ~= 0 then
    return code
  end
  minecraft.log("warn", "sprint: using fallback key code for '" .. name .. "'")
  return fallback
end

local function is_key_down(key)
  return minecraft.is_key_down(key)
end

local function forward_key()
  return key_code("forward", 17)
end

local function sprint_key()
  return key_code("sprint", SPRINT_KEY_FALLBACK)
end

local function forward_down()
  return is_key_down(forward_key())
end

local function sprint_key_down()
  return is_key_down(sprint_key())
end

local function start_sprinting(by_key)
  if sprinting then
    return
  end
  sprinting = true
  sprinting_by_key = by_key
  sprint_boost_timer = minecraft.settings.get("start_boost_ticks") or 4
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
      sprint_toggle_timer = minecraft.settings.get("double_tap_window_ticks") or 7
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

minecraft.on(minecraft.events.client_tick, {
  before = false,
  after_world = false,
  priority = 100,
}, function()
  update_sprint_state()
  prev_fov_multiplier = current_fov_multiplier
  local target = sprinting and (minecraft.settings.get("sprint_fov_multiplier") or 1.08) or 1.0
  local lerp_rate = minecraft.settings.get("fov_lerp_rate") or 8.0
  local t = 1.0 - math.exp(-lerp_rate * (1.0 / 20.0))
  current_fov_multiplier = current_fov_multiplier + (target - current_fov_multiplier) * t
end)

-- Forward input is normalized by moveNonSolid; scale speed_multiplier instead.
minecraft.on(minecraft.events.player_travel, {
  is_local_player = true,
  priority = 100,
}, function(event)
  if sprinting and event.forward > 0.0 then
    local multiplier = minecraft.settings.get("sprint_multiplier") or 1.45
    if sprint_boost_timer > 0 then
      multiplier = multiplier * (minecraft.settings.get("start_boost_multiplier") or 1.08)
      sprint_boost_timer = sprint_boost_timer - 1
    end
    event.speed_multiplier = (event.speed_multiplier or 1.0) * multiplier
  end
end)

minecraft.on(minecraft.events.fov, { priority = 100 }, function(event)
  -- Lua event fields use snake_case. Keep a fallback so a malformed render
  -- event cannot break every frame and flood the log.
  local t = tonumber(event.tick_delta) or 1.0
  t = math.max(0.0, math.min(1.0, t))

  local fov_mult =
    prev_fov_multiplier +
    (current_fov_multiplier - prev_fov_multiplier) * t

  event.fov = (tonumber(event.fov) or 70.0) * fov_mult
end)
