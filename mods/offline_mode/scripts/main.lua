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

-- Apply any saved preference as soon as the mod loads.
load_config()
apply()

-- Custom settings screen opened from the authenticator menu.
minecraft.screen.on_lua_screen("offline_mode:settings", {
  init = function(event)
    event.add_field("username", 60, 64, 200, 20, state.username, 16)
    event.add_button(60, 92, 200, 20, toggle_label(), function()
      state.enabled = not state.enabled
      apply()
    end)
    event.add_button(60, 118, 200, 20, "Randomize name", function()
      local name = random_name()
      event.set_field_text("username", name)
      state.username = name
      if state.enabled then
        apply()
      else
        save_config()
      end
    end)
    event.add_button(60, 144, 200, 20, "Apply", function()
      state.username = minecraft.screen.field_text("username") or ""
      apply()
    end)
    event.add_button(60, 200, 200, 20, "Done", function()
      minecraft.screen.close()
    end)
  end,
  render = function(event)
    local w = event.width or 320
    minecraft.gui.draw_centered_text({ x = 0, y = 24, width = w, text = "Offline Mode", color = 0xFFFFFF })
    minecraft.gui.draw_centered_text({ x = 0, y = 176, width = w, text = status_line(), color = 0xA0A0A0 })
  end,
}, 100)

-- Inject the entry button into the authenticator (login) screen while logged out.
-- Placed at a fixed y so repeated re-inits overlap the same button instead of stacking.
minecraft.screen.on_ui(minecraft.screen.ids.login, minecraft.screen.regions.screen, function(event)
  event.ui.add_centered_button(186, state.enabled and "Offline Mode: ON" or "Offline Mode: OFF", function()
    minecraft.screen.open("offline_mode:settings")
  end)
end, 100)
