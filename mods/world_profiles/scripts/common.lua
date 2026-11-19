local M = {}

M.AIR_ID = 0

function M.block_id(name)
  local id = minecraft.world.block_id(name)
  assert(type(id) == "number" and id > 0, "world_profiles: required block is missing: " .. name)
  return id
end

function M.java_floor(value)
  local integer
  if value >= 2147483647 then
    integer = 2147483647
  elseif value <= -2147483648 then
    integer = -2147483648
  else
    integer = math.tointeger(math.modf(value))
  end
  if value < integer then
    integer = integer - 1
    if integer < -2147483648 then return 2147483647 end
  end
  return integer
end

function M.hash_coord(x, z, seed)
  local n = (x * 374761393 + z * 668265263 + seed * 1013904223) % 2147483647
  return (n * n * n * 60493) % 2147483647
end

return M
