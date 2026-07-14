local MOD_ID = "offline_mode"
local CONFIG_PATH = "offline_mode.cfg"

-- Persisted state: whether offline mode is active and the chosen runtime username.
local state = { enabled = false, username = "" }

-- Simple Lua RNG (no world required, so it works from the title/login screen).
local rng_state = (minecraft.time.utc_millis() or os.time() or 1) % 2147483647
if rng_state == 0 then
  rng_state = 12345
end
local function rand()
  rng_state = (rng_state * 1103515245 + 12345) % 2147483648
  return rng_state
end
local function pick(t)
  return t[(rand() % #t) + 1]
end

local ADJECTIVES = {
  "Brave", "Clever", "Silent", "Lucky", "Cosmic", "Hidden", "Rapid", "Ancient",
  "Frozen", "Golden", "Crimson", "Mystic", "Wild", "Shadow", "Emerald", "Azure",
}
local NOUNS = {
  "Wolf", "Falcon", "Otter", "Raven", "Tiger", "Comet", "Ember", "Storm",
  "Pixel", "Quartz", "Willow", "Onyx", "Maple", "Breeze", "Cinder", "Vortex",
}

local function random_name()
  local num = rand() % 9000 + 1000
  return pick(ADJECTIVES) .. pick(NOUNS) .. tostring(num)
end

local function load_config()
  local raw = minecraft.storage.read(CONFIG_PATH)
  if raw then
    local t, err = minecraft.util.json_decode(raw)
    if t and type(t) == "table" then
      state.enabled = not not t.enabled
      state.username = type(t.username) == "string" and t.username or ""
    end
  end
end

local function save_config()
  minecraft.storage.write(CONFIG_PATH, minecraft.util.json_encode(state))
end

-- Push the current state into the runtime session so offline-mode servers receive it.
-- While a Microsoft account is signed in, the engine ignores this (name is session-locked).
local function apply()
  if state.enabled and state.username ~= "" then
    minecraft.session.set_offline_username(state.username)
  else
    minecraft.session.clear_offline_username()
  end
  save_config()
end

local function toggle_label()
  return "Offline Mode: " .. (state.enabled and "ON" or "OFF")
end

local function status_line()
  if state.enabled and state.username ~= "" then
    return "Joining offline servers as: " .. state.username
  end
  return "Offline mode off - joins use the default name."
end

-- Apply any saved preference once the client is ready.
minecraft.on(minecraft.events.client_tick, {
  has_player = true,
  once = true,
  priority = 0,
}, function()
  load_config()
  apply()
end)

-- Custom settings screen opened from the authenticator menu.
minecraft.on(minecraft.events.screen_event, { screen_id = "offline_mode:settings", priority = 100 }, function(event)
  if event.phase == "init" then
    minecraft.screen.add_field("username", 60, 64, 200, 20, { text = state.username, max_len = 16 })
    minecraft.screen.add_button(60, 92, 200, 20, toggle_label(), function()
      state.enabled = not state.enabled
      apply()
    end)
    minecraft.screen.add_button(60, 118, 200, 20, "Randomize name", function()
      local name = random_name()
      minecraft.screen.set_field_text("username", name)
      state.username = name
      if state.enabled then
        apply()
      else
        save_config()
      end
    end)
    minecraft.screen.add_button(60, 144, 200, 20, "Apply", function()
      state.username = minecraft.screen.field_text("username") or ""
      apply()
    end)
    minecraft.screen.add_button(60, 200, 200, 20, "Done", function()
      minecraft.screen.close()
    end)
  elseif event.phase == "render" then
    local w = event.width or 320
    minecraft.gui.draw_centered_text({ x = 0, y = 24, width = w, text = "Offline Mode", color = 0xFFFFFF })
    minecraft.gui.draw_centered_text({ x = 0, y = 176, width = w, text = status_line(), color = 0xA0A0A0 })
  end
end)

-- Inject the entry button into the authenticator (login) screen while logged out.
-- Placed at a fixed y so repeated re-inits overlap the same button instead of stacking.
minecraft.on(minecraft.events.screen_ui, {
  screen_id = minecraft.screen.ids.login,
  region = minecraft.screen.regions.screen,
  priority = 100,
}, function(event)
  event.ui:add_centered_button(186, state.enabled and "Offline Mode: ON" or "Offline Mode: OFF", function()
    minecraft.screen.open("offline_mode:settings")
  end)
end)
