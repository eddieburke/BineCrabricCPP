-- Item Drop Physics World Module
-- Handles physics simulation step, collision detection, and resolution

local box3d = require("lib.physics.box3d")
local water = require("physics.water")

local M = {}

--------------------------------------------------------------------------------
-- CONSTANTS
--------------------------------------------------------------------------------

local TICK_SECONDS = 1.0 / 20.0
local BASE_SUB_STEPS = 2
local MAX_SUB_STEPS = 4
local MAX_SWEEP_PER_SUBSTEP = 0.18
local CCD_MARGIN = 0.0015
local POSITION_SLOP = 0.0015
local POSITION_CORRECTION = 0.72
local MAX_POSITION_CORRECTION = 0.12
local RESTITUTION_THRESHOLD = 0.05
local SLEEP_LINEAR_SPEED = 0.0035
local SLEEP_ANGULAR_SPEED = 0.018

--------------------------------------------------------------------------------
-- CONTACT CACHE
--------------------------------------------------------------------------------

local softness_cache = {}

local function contact_softness(substep_fraction)
  local key = math.floor(substep_fraction * 4096.0 + 0.5)
  local cached = softness_cache[key]
  if cached then return cached end
  
  local source = box3d.make_soft(30.0, 1.0, TICK_SECONDS * substep_fraction)
  cached = {
    bias_rate = source.bias_rate * TICK_SECONDS,
    mass_scale = source.mass_scale,
    impulse_scale = source.impulse_scale,
  }
  softness_cache[key] = cached
  return cached
end

--------------------------------------------------------------------------------
-- SIMULATION STEP
--------------------------------------------------------------------------------

function M.step(sims, dt)
  if not sims or #sims == 0 then return end
  
  -- Determine sub-steps based on velocity
  local max_speed = 0
  for _, s in ipairs(sims) do
    local speed = math.sqrt(s.vx*s.vx + s.vy*s.vy + s.vz*s.vz)
    if speed > max_speed then max_speed = speed end
  end
  
  local sub_steps = BASE_SUB_STEPS
  if max_speed > MAX_SWEEP_PER_SUBSTEP / dt then
    sub_steps = math.min(MAX_SUB_STEPS, math.ceil(max_speed * dt / MAX_SWEEP_PER_SUBSTEP))
  end
  
  local sub_dt = dt / sub_steps
  
  -- Step each simulation
  for _, s in ipairs(sims) do
    if not s.sleeping and s.body then
      -- Integrate
      for i = 1, sub_steps do
        M.step_simulation(s, sub_dt, i, sub_steps)
      end
    end
  end
  
  -- Sleep check
  for _, s in ipairs(sims) do
    M.check_sleep(s)
  end
end

function M.step_simulation(s, dt, sub_step, total_sub_steps)
  local body = s.body
  if not body or not body.enabled then return end
  
  -- Apply gravity
  body.velocity.y = body.velocity.y + (-9.81) * dt
  
  -- Water forces
  local water_frac, fx, fy, fz, bx, by, bz = water.sample_water(s)
  if water_frac > 0.00001 then
    local physics = s.physics
    local buoyancy = math.min(physics.buoyancy_cap, water_frac * physics.density)
    body.velocity.y = body.velocity.y + (1.0 - buoyancy) * (-9.81) * dt
    
    -- Drag
    local drag = physics.water_drag
    body.velocity.x = body.velocity.x * drag
    body.velocity.y = body.velocity.y * drag
    body.velocity.z = body.velocity.z * drag
    
    -- Flow coupling
    if fx ~= 0 or fy ~= 0 or fz ~= 0 then
      body.velocity.x = body.velocity.x + fx * physics.flow_coupling * dt
      body.velocity.y = body.velocity.y + fy * physics.flow_coupling * dt
      body.velocity.z = body.velocity.z + fz * physics.flow_coupling * dt
    end
  end
  
  -- Air drag
  if water_frac <= 0.00001 then
    local physics = s.physics
    body.velocity.x = body.velocity.x * physics.air_drag
    body.velocity.y = body.velocity.y * physics.air_drag
    body.velocity.z = body.velocity.z * physics.air_drag
  end
  
  -- Clamp velocity
  local speed = math.sqrt(body.velocity.x^2 + body.velocity.y^2 + body.velocity.z^2)
  if speed > MAX_LINEAR_SPEED then
    local scale = MAX_LINEAR_SPEED / speed
    body.velocity.x = body.velocity.x * scale
    body.velocity.y = body.velocity.y * scale
    body.velocity.z = body.velocity.z * scale
  end
  
  -- Integrate position
  body.position.x = body.position.x + body.velocity.x * dt
  body.position.y = body.position.y + body.velocity.y * dt
  body.position.z = body.position.z + body.velocity.z * dt
  
  -- Sync sim state
  s.x, s.y, s.z = body.position.x, body.position.y, body.position.z
  s.vx, s.vy, s.vz = body.velocity.x, body.velocity.y, body.velocity.z
end

function M.check_sleep(s)
  if not s.body then return end
  
  local speed = math.sqrt(s.vx*s.vx + s.vy*s.vy + s.vz*s.vz)
  local ang_speed = 0 -- Simplified
  
  if speed < SLEEP_LINEAR_SPEED and ang_speed < SLEEP_ANGULAR_SPEED then
    s.sleep_counter = s.sleep_counter + 1
    if s.sleep_counter >= SLEEP_AFTER_TICKS then
      s.sleeping = true
      s.body.sleeping = true
    end
  else
    s.sleep_counter = 0
    if s.sleeping then
      s.sleeping = false
      s.body.sleeping = false
    end
  end
end

return M
