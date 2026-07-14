local CONFIG_FILE = "judaism.txt"
local SCREEN_ID = "judaism:settings"

local SETTINGS_DEFAULTS = {
  enabled = true,
  shabbat_enabled = true,
  show_messages = true,
  block_restricted = true,
  block_crafting = true,
  block_combat = true,
  block_redstone = true,
  block_pistons = true,
}

local SETTINGS_KEYS = {
  "enabled", "shabbat_enabled", "show_messages", "block_restricted",
  "block_crafting", "block_combat", "block_redstone", "block_pistons",
}

local SETTINGS_NAMES = {
  enabled = "enabled",
  shabbat_enabled = "shabbatEnabled",
  show_messages = "showMessages",
  block_restricted = "blockRestricted",
  block_crafting = "blockCrafting",
  block_combat = "blockCombat",
  block_redstone = "blockRedstone",
  block_pistons = "blockPistons",
}

local SETTINGS_ALIASES = {}
for internal_name, file_name in pairs(SETTINGS_NAMES) do
  SETTINGS_ALIASES[file_name] = internal_name
end

local settings = minecraft.util.copy(SETTINGS_DEFAULTS)

local shabbat_active = false
local last_shabbat_state = false

local function save_settings()
  minecraft.config.save(CONFIG_FILE, settings, {
    keys = SETTINGS_KEYS,
    names = SETTINGS_NAMES,
    separator = ":",
  })
end

local function load_settings()
  local found
  settings, found = minecraft.config.load(CONFIG_FILE, SETTINGS_DEFAULTS, {
    aliases = SETTINGS_ALIASES,
  })
  if not found then
    save_settings()
  end
end

local function is_friday_or_saturday()
  local day = os.date("*t").wday
  return day == 6 or day == 7
end

local function is_shabbat_time()
  local now = os.date("*t")
  local hour = now.hour
  local min = now.min
  local day = now.wday

  if day == 6 then
    if hour >= 18 then
      return true
    end
    if hour == 17 and min >= 0 then
      return true
    end
  end

  if day == 7 then
    if hour < 21 then
      return true
    end
    if hour == 21 and min == 0 then
      return true
    end
  end

  return false
end

local function update_shabbat_state()
  shabbat_active = settings.shabbat_enabled and is_shabbat_time()

  if shabbat_active and not last_shabbat_state then
    if settings.show_messages then
      minecraft.chat.send("Shabbat Shalom! Shabbat has begun.")
    end
  elseif not shabbat_active and last_shabbat_state then
    if settings.show_messages then
      minecraft.chat.send("Shabbat is over. Good week!")
    end
  end

  last_shabbat_state = shabbat_active
end

local function is_restricted_item(item_name)
  if not shabbat_active then
    return false
  end
  local restricted = {
    "sword", "axe", "pickaxe", "shovel", "hoe",
    "bow", "crossbow", "trident", "mace",
    "shears", "flint_and_steel",
    "end_rod", "campfire", "soul_campfire",
    "fire_charge", "firework_rocket",
    "elytra", "trident",
    "ender_pearl", "ender_eye", "chorus_fruit",
    "shulker_box", "bundle",
  }
  for _, r in ipairs(restricted) do
    if item_name and string.find(string.lower(item_name), r) then
      return true
    end
  end
  return false
end

minecraft.on(minecraft.events.client_tick, {
  before = false,
  after_world = false,
  priority = 50,
}, function()
  update_shabbat_state()
end)

minecraft.on(minecraft.events.player_travel, {
  is_local_player = true,
  priority = 50,
}, function(event)
  if not settings.enabled then
    return
  end
  if shabbat_active and event.speed_multiplier then
    event.speed_multiplier = event.speed_multiplier * 0.85
  end
end)

minecraft.on(minecraft.events.attack, {
  is_local_player = true,
  priority = 50,
}, function(event)
  if not settings.enabled or not shabbat_active then
    return
  end
  if settings.block_combat then
    event.cancelled = true
    if settings.show_messages then
      minecraft.chat.send("Combat is prohibited during Shabbat.")
    end
  end
end)

minecraft.on(minecraft.events.block_break, {
  is_local_player = true,
  priority = 50,
}, function(event)
  if not settings.enabled or not shabbat_active then
    return
  end
  if settings.block_restricted then
    event.cancelled = true
    if settings.show_messages then
      minecraft.chat.send("Block breaking is prohibited during Shabbat.")
    end
  end
end)

minecraft.on(minecraft.events.craft, {
  is_local_player = true,
  priority = 50,
}, function(event)
  if not settings.enabled or not shabbat_active then
    return
  end
  if settings.block_crafting then
    event.cancelled = true
    if settings.show_messages then
      minecraft.chat.send("Crafting is prohibited during Shabbat.")
    end
  end
end)

minecraft.on(minecraft.events.block_place, {
  is_local_player = true,
  priority = 50,
}, function(event)
  if not settings.enabled or not shabbat_active then
    return
  end
  if settings.block_restricted then
    local block_name = event.block_name or ""
    if string.find(string.lower(block_name), "redstone") or
       string.find(string.lower(block_name), "piston") or
       string.find(string.lower(block_name), "observer") or
       string.find(string.lower(block_name), "dispenser") or
       string.find(string.lower(block_name), "dropper") then
      event.cancelled = true
      if settings.show_messages then
        minecraft.chat.send("Redstone devices are prohibited during Shabbat.")
      end
    end
  end
end)

minecraft.on(minecraft.events.item_use, {
  is_local_player = true,
  priority = 50,
}, function(event)
  if not settings.enabled or not shabbat_active then
    return
  end
  local item_name = event.item_name or ""
  if is_restricted_item(item_name) then
    event.cancelled = true
    if settings.show_messages then
      minecraft.chat.send("Using that item is prohibited during Shabbat.")
    end
  end
end)

minecraft.screen.on_ui(minecraft.screen.ids.world_settings, minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil and settings.enabled then
    event.ui:add_stacked_centered_button("Judaism...", function()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)
  end
  return event
end, 80)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID, priority = 100 }, function(event)
  if event.phase == "init" then
    local y = 40
    local x = 20
    local w = 160
    local h = 16

    minecraft.screen.add_button(x, y, w, h, settings.shabbat_enabled and "Shabbat: ON" or "Shabbat: OFF", function()
      settings.shabbat_enabled = not settings.shabbat_enabled
      save_settings()
      minecraft.screen.close()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)

    y = y + 24
    minecraft.screen.add_button(x, y, w, h, settings.show_messages and "Messages: ON" or "Messages: OFF", function()
      settings.show_messages = not settings.show_messages
      save_settings()
      minecraft.screen.close()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)

    y = y + 24
    minecraft.screen.add_button(x, y, w, h, settings.block_restricted and "Block Breaking: ON" or "Block Breaking: OFF", function()
      settings.block_restricted = not settings.block_restricted
      save_settings()
      minecraft.screen.close()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)

    y = y + 24
    minecraft.screen.add_button(x, y, w, h, settings.block_crafting and "Crafting: ON" or "Crafting: OFF", function()
      settings.block_crafting = not settings.block_crafting
      save_settings()
      minecraft.screen.close()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)

    y = y + 24
    minecraft.screen.add_button(x, y, w, h, settings.block_combat and "Combat: ON" or "Combat: OFF", function()
      settings.block_combat = not settings.block_combat
      save_settings()
      minecraft.screen.close()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)

    y = y + 24
    minecraft.screen.add_button(x, y, w, h, settings.block_redstone and "Redstone: ON" or "Redstone: OFF", function()
      settings.block_redstone = not settings.block_redstone
      save_settings()
      minecraft.screen.close()
      minecraft.screen.open(SCREEN_ID, { title = "" })
    end)

    y = y + 24
    minecraft.screen.add_button(x, y, w, h, "Done", function()
      save_settings()
      minecraft.screen.close()
    end)

  elseif event.phase == "render" then
    minecraft.gui.fill_rect(4, 4, event.width - 8, event.height - 8, 0xD9121A25)
    minecraft.gui.draw_text(10, 12, "JUDAISM MOD", 0xFFFFFFFF)

    if shabbat_active then
      minecraft.gui.draw_text(10, 30, "SHABBAT IS ACTIVE", 0xFF00FF00)
    else
      minecraft.gui.draw_text(10, 30, "SHABBAT INACTIVE", 0xFFFF8080)
    end

    local day = os.date("*t").wday
    local day_names = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}
    minecraft.gui.draw_text(event.width - 80, 12, day_names[day], 0xFF9BB0C2)

  elseif event.phase == "key" then
    if event.key == minecraft.keys.escape then
      save_settings()
      minecraft.screen.close()
      event.handled = true
    end

  elseif event.phase == "close" then
    save_settings()
  end
end)

load_settings()
minecraft.log("info", "judaism mod loaded")
