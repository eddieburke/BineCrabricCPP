local DOUBLE_TAP_WINDOW_TICKS = 7
local SPRINT_KEY_FALLBACK = 29
local SPRINT_MULTIPLIER = 1.45
local START_BOOST_MULTIPLIER = 1.08
local START_BOOST_TICKS = 4

local sprinting = false
local sprinting_by_key = false
local was_forward_down = false
local sprint_toggle_timer = 0
local sprint_boost_timer = 0

local function key_code(name, fallback)
  local code = minecraft.key_code(name)
  if code ~= nil and code ~= 0 then
    return code
  end
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
  sprint_boost_timer = START_BOOST_TICKS
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
      sprint_toggle_timer = DOUBLE_TAP_WINDOW_TICKS
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
end)

-- Forward input is normalized by moveNonSolid; scale speed_multiplier instead.
minecraft.on(minecraft.events.player_travel, {
  is_local_player = true,
  priority = 100,
}, function(event)
  if sprinting and event.forward > 0.0 then
    local multiplier = SPRINT_MULTIPLIER
    if sprint_boost_timer > 0 then
      multiplier = multiplier * START_BOOST_MULTIPLIER
      sprint_boost_timer = sprint_boost_timer - 1
    end
    event.speed_multiplier = (event.speed_multiplier or 1.0) * multiplier
  end
end)

minecraft.on(minecraft.events.fov, { priority = 100 }, function(event)
  if sprinting then
    event.fov = event.fov * 1.08
  end
end)
