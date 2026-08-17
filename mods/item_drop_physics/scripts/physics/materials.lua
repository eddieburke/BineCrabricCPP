-- Material model. Replaces the per-item physics database.
-- An item is mapped to one of a handful
-- of material classes; everything the solver needs (mass, buoyancy, angular
-- damping, rest thresholds) is derived from four numbers plus the item's own
-- model bounds. Unknown ids -- including modded ones -- fall back to "generic"
-- instead of needing a table entry.

local M = {}

-- density:     relative to water (1.0). Drives mass and buoyancy.
-- friction:    Coulomb coefficient at contacts. Drives rolling resistance too.
-- restitution: bounciness, 0..1.
-- drag:        per-tick velocity retention in air.
local CLASSES = {
  stone   = { density = 2.60, friction = 0.72, restitution = 0.26, drag = 0.988 },
  metal   = { density = 4.50, friction = 0.50, restitution = 0.42, drag = 0.992 },
  glass   = { density = 2.40, friction = 0.36, restitution = 0.48, drag = 0.988 },
  sand    = { density = 1.60, friction = 0.85, restitution = 0.05, drag = 0.975 },
  soil    = { density = 1.45, friction = 0.78, restitution = 0.10, drag = 0.980 },
  food    = { density = 1.00, friction = 0.66, restitution = 0.18, drag = 0.978 },
  wood    = { density = 0.65, friction = 0.58, restitution = 0.34, drag = 0.985 },
  plant   = { density = 0.55, friction = 0.70, restitution = 0.08, drag = 0.960 },
  cloth   = { density = 0.30, friction = 0.80, restitution = 0.05, drag = 0.955 },
  generic = { density = 1.10, friction = 0.56, restitution = 0.32, drag = 0.985 },
}

M.CLASSES = CLASSES
M.DEFAULT = CLASSES.generic

-- Id -> class name. Written as runs so the whole registry fits on a screen;
-- `{lo, hi}` covers an inclusive id range, a bare number covers one id.
local ASSIGNMENTS = {
  stone = { 1, 4, 7, 14, 15, 16, 21, 22, 24, 43, 44, 45, 48, 49, 52, 56, 67,
            70, 73, 74, 87, 97, 98, {272, 275}, 318, 331, 405 },
  metal = { 41, 42, 57, 71, {256, 259}, {265, 267}, {283, 286}, {292, 294},
            {302, 317}, 330, 335, 341, 346, 359 },
  glass = { 20, 79, 89, 102, 374, 381 },
  sand  = { 12, 13, 289 },
  soil  = { 2, 3, 60, 82, 88, 110, 337 },
  food  = { {260, 261}, 282, 297, {319, 322}, {349, 350}, 357, 360, 363, 365,
            367, 391, 392, 393, 394, 396, 400 },
  wood  = { 5, 17, 25, 47, 53, 54, 58, 63, 64, 65, 68, 72, 84, 85, 96,
            {268, 271}, 280, 323, 324, 325, 326, 328, 329, 333, 355 },
  plant = { 6, 18, 31, 32, 37, 38, 39, 40, 50, 59, 81, 83, 86, 91, 295, 296,
            338, 351, 361, 362 },
  cloth = { 30, 35, 287, 334, {298, 301}, 339, 340, 345, 358 },
}

local CLASS_BY_ID = {}
for name, ids in pairs(ASSIGNMENTS) do
  local class = CLASSES[name]
  for _, entry in ipairs(ids) do
    if type(entry) == "table" then
      for id = entry[1], entry[2] do CLASS_BY_ID[id] = class end
    else
      CLASS_BY_ID[entry] = class
    end
  end
end

--- Material class for an item id. Never nil.
function M.get(item_id)
  return CLASS_BY_ID[item_id] or M.DEFAULT
end

return M
