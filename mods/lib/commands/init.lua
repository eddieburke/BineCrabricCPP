-- Chat command registry shared by every mod. Mods call commands.register{...};
-- the library installs one chat_send hook that parses "/name args" and dispatches.
-- Slash input is only claimed in a local world -- in multiplayer it belongs to the server.

local commands = {}

local SECTION = "\194\167"
commands.GRAY = SECTION .. "7"
commands.RED = SECTION .. "c"
commands.YELLOW = SECTION .. "e"
commands.PURPLE = SECTION .. "d"

local registry = {}
local order = {}
local installed = false

function commands.reply(text)
  minecraft.notify(tostring(text))
end

local function usage_text(entry)
  local line = "/" .. entry.name
  if entry.params ~= "" then
    line = line .. " " .. entry.params
  end
  return line
end

local function help_line(entry)
  local left = usage_text(entry)
  if #left < 28 then
    left = left .. string.rep(" ", 28 - #left)
  end
  return commands.GRAY .. left .. entry.help
end

function commands.number(value, label)
  local parsed = tonumber(value)
  assert(parsed ~= nil, "expected a number for " .. (label or "argument"))
  return parsed
end

function commands.integer(value, label)
  local parsed = commands.number(value, label)
  parsed = math.floor(parsed)
  return parsed
end

local function split(message)
  local args = {}
  for token in message:gmatch("%S+") do
    args[#args + 1] = token
  end
  return args
end

local function execute(message, event)
  if event ~= nil and (event.canceled == true or event.remote == true) then
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
  local name = tostring(spec.name or ""):lower()
  assert(name:match("^/?[%a?][%w_%-]*$") ~= nil, "commands.register expects a name")
  local entry = {
    name = name,
    params = spec.params or "",
    help = spec.help or "",
    run = spec.run,
  }
  if registry[name] == nil then
    order[#order + 1] = name
    table.sort(order)
  end
  registry[name] = entry
  for _, alias in ipairs(spec.aliases or {}) do
    registry[tostring(alias):lower()] = entry
  end
  commands.install()
  return true
end

function commands.execute(message, event)
  return execute(message, event)
end

function commands.unregister(name)
  name = tostring(name or ""):lower()
  if registry[name] == nil then
    return false
  end
  registry[name] = nil
  for index, value in ipairs(order) do
    if value == name then
      table.remove(order, index)
      break
    end
  end
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
  help = "shows this message",
  run = function(context)
    context.reply("Commands:")
    for _, entry in ipairs(commands.list()) do
      context.reply(help_line(entry))
    end
  end,
})

return commands
