local M = {}

M.AIR_ID = 0

function M.block_id(name)
  local id = minecraft.world.block_id(name)
  return id ~= nil and id or 0
end

function M.hash_coord(x, z, seed)
  local n = (x * 374761393 + z * 668265263 + seed * 1013904223) % 2147483647
  return (n * n * n * 60493) % 2147483647
end

return M
