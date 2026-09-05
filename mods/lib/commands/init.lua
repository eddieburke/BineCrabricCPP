-- Chat command registry shared by every mod. Mods call commands.register{...};
-- the library installs one chat_send hook that parses "/name args" and dispatches.
-- Slash input is only claimed in a local world -- in multiplayer it belongs to the server.
--
-- Every mod registers into one registry, so /help groups by the category a mod names in
-- commands.help_title and pages the result. Chat shows 20 lines while it is open, and the
-- installed mods register well over a hundred commands between them: an unpaged dump
-- scrolls its own first page out of reach.

local commands = {}
commands.help_title = "Commands:"

local SECTION = "\194\167"
commands.GRAY = SECTION .. "7"
commands.RED = SECTION .. "c"
commands.YELLOW = SECTION .. "e"
commands.PURPLE = SECTION .. "d"
commands.WHITE = SECTION .. "f"
commands.AQUA = SECTION .. "b"

local PAGE_SIZE = 8

local registry = {}
local order = {}
local categories = {}
local help_lines = nil
local installed = false

function commands.reply(text)
  minecraft.notify(tostring(text))
end

local function usage_text(entry)
  if entry.params == "" then
    return "/" .. entry.name
  end
  return "/" .. entry.name .. " " .. entry.params
end

function commands.number(value, label)
  local parsed = tonumber(value)
  assert(parsed ~= nil, "expected a number for " .. (label or "argument"))
  return parsed
end

function commands.integer(value, label)
  return math.floor(commands.number(value, label))
end

local function split(message)
  local args = {}
  for token in message:gmatch("%S+") do
    args[#args + 1] = token
  end
  return args
end

-- One flat list of ready-to-print lines -- category headers included -- so a page is a
-- slice of it and never lands mid-entry. Rebuilt only when the registry changes.
local function rendered_help()
  if help_lines ~= nil then
    return help_lines
  end
  local grouped = {}
  for _, name in ipairs(order) do
    local entry = registry[name]
    local bucket = grouped[entry.category]
    if bucket == nil then
      bucket = {}
      grouped[entry.category] = bucket
    end
    bucket[#bucket + 1] = entry
  end
  local lines = {}
  for _, category in ipairs(categories) do
    local bucket = grouped[category]
    if bucket ~= nil then
      lines[#lines + 1] = commands.YELLOW .. category
      for _, entry in ipairs(bucket) do
        lines[#lines + 1] = commands.WHITE .. usage_text(entry) .. commands.GRAY .. " - " .. entry.help
      end
    end
  end
  help_lines = lines
  return lines
end

local function show_help_page(page)
  local lines = rendered_help()
  local pages = math.max(1, math.ceil(#lines / PAGE_SIZE))
  page = math.max(1, math.min(pages, math.floor(page)))
  commands.reply(commands.YELLOW .. "--- Help " .. commands.WHITE .. page .. commands.GRAY .. "/" .. pages ..
      commands.YELLOW .. " ---" .. commands.GRAY .. " " .. #order .. " commands")
  for index = (page - 1) * PAGE_SIZE + 1, math.min(#lines, page * PAGE_SIZE) do
    commands.reply(lines[index])
  end
  commands.reply(commands.GRAY .. "/help <page> for more, /help <command> for details")
end

local function show_command_help(name)
  -- "/help //set" and "/help /set" should both find WorldEdit's entry.
  local entry = registry[name] or registry[(name:gsub("^/", ""))]
  assert(entry ~= nil, "unknown command: /" .. name)
  commands.reply(commands.YELLOW .. usage_text(entry))
  commands.reply(commands.GRAY .. entry.help)
  if entry.details ~= nil then
    commands.reply(commands.GRAY .. entry.details)
  end
  if #entry.aliases > 0 then
    commands.reply(commands.GRAY .. "also: /" .. table.concat(entry.aliases, ", /"))
  end
  commands.reply(commands.GRAY .. "from: " .. entry.category)
end

local function execute(message, event)
  if event ~= nil and event.remote == true then
    return false
  end
  message = message or ""
  if message:sub(1, 1) ~= "/" then
    return false
  end
  local args = split(message:sub(2))
  if #args == 0 then
    return false
  end
  local name = args[1]:lower()
  if event ~= nil and event.canceled == true and name ~= "help" and name ~= "?" then
    return false
  end
  local entry = registry[name]
  if entry == nil then
    return false
  end
  table.remove(args, 1)
  local context = {
    name = name,
    args = args,
    raw = message,
    rest = message:sub(2):match("^%S+%s*(.-)%s*$") or "",
    event = event or {},
    entry = entry,
    reply = commands.reply,
  }
  function context.usage()
    commands.reply(commands.RED .. "Usage: " .. usage_text(entry))
  end
  local ok, err = pcall(entry.run, context)
  if not ok then
    commands.reply(commands.RED .. tostring(err):gsub("^.-:%d+:%s*", ""))
  end
  return true
end

local function dispatch(event)
  if execute(event.message, event) then
    event.canceled = true
  end
  return event
end

function commands.install()
  if installed then
    return
  end
  installed = true
  minecraft.on("chat_send", {}, dispatch)
end

function commands.register(spec)
  assert(type(spec) == "table", "commands.register expects a table")
  assert(type(spec.run) == "function", "commands.register expects run to be a function")
  -- The leading slash is kept: chat strips exactly one, so WorldEdit's "/set" is reached
  -- as //set and leaves plain /set to whichever mod claims it.
  local name = tostring(spec.name or ""):lower()
  assert(name:match("^/?[%a?][%w_%-]*$") ~= nil, "commands.register expects a name")
  local category = tostring(spec.category or commands.help_title):gsub(":%s*$", "")
  local entry = {
    name = name,
    params = spec.params or "",
    help = spec.help or "",
    details = spec.details,
    category = category,
    aliases = {},
    run = spec.run,
  }
  if registry[name] == nil then
    order[#order + 1] = name
    table.sort(order)
  elseif registry[name].category ~= category then
    minecraft.log("warn", "command /" .. name .. " from " .. category .. " replaces the one from " ..
        registry[name].category)
  end
  registry[name] = entry
  for _, alias in ipairs(spec.aliases or {}) do
    alias = tostring(alias):lower()
    registry[alias] = entry
    entry.aliases[#entry.aliases + 1] = alias
  end
  local known = false
  for _, value in ipairs(categories) do
    if value == category then
      known = true
      break
    end
  end
  if not known then
    categories[#categories + 1] = category
  end
  help_lines = nil
  commands.install()
  return true
end

function commands.execute(message, event)
  return execute(message, event)
end

function commands.unregister(name)
  name = tostring(name or ""):lower()
  local entry = registry[name]
  if entry == nil then
    return false
  end
  registry[name] = nil
  for _, alias in ipairs(entry.aliases) do
    registry[alias] = nil
  end
  for index, value in ipairs(order) do
    if value == entry.name then
      table.remove(order, index)
      break
    end
  end
  help_lines = nil
  return true
end

function commands.list()
  local entries = {}
  for _, name in ipairs(order) do
    entries[#entries + 1] = registry[name]
  end
  return entries
end

commands.register({
  name = "help",
  aliases = { "?" },
  category = "General",
  params = "[page|command]",
  help = "lists every command, one page at a time",
  details = "/help 3 jumps to page 3; /help fly explains one command",
  run = function(context)
    local argument = context.args[1]
    if argument == nil then
      return show_help_page(1)
    end
    local page = tonumber(argument)
    if page ~= nil then
      return show_help_page(page)
    end
    return show_command_help(argument:lower())
  end,
})

return commands
