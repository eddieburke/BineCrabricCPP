-- Every setting is declared once here; defaults, sanitization, the saved key
-- order, and the settings screens are all derived from this table.
-- Entries with `toggle` are booleans; entries with `min`/`max` are sliders
-- (`integer` snaps to whole numbers, `digits` controls the label format).
local groups = {
  {
    id = "shadow_viewport:shadows",
    title = "Vibrant Shadows",
    button = "Vibrant Shadows...",
    entries = {
      { key="shadows", default=true, toggle="Dynamic shadows" },
      { key="shadow_quality", default=1, min=0, max=3, integer=true, label="Map quality" },
      { key="shadow_filter", default=1, min=0, max=2, integer=true, label="PCF quality" },
      { key="shadow_distance", default=112.0, min=32, max=192, label="Radius", digits=0 },
      { key="shadow_depth", default=256.0, min=128, max=512, label="Depth", digits=0 },
      { key="shadow_strength", default=0.72, min=0, max=1, label="Strength" },
      { key="shadow_bias", default=0.025, min=0.005, max=0.25, label="Bias" },
      { key="shadow_softness", default=1.15, min=0.4, max=3, label="Softness" },
      { key="entity_shadows", default=true, toggle="Entity shadows" },
    },
  },
  {
    id = "shadow_viewport:lighting",
    title = "Vibrant Lighting",
    button = "Vibrant Lighting...",
    entries = {
      { key="pbr", default=true, toggle="PBR-style lighting" },
      { key="pbr_strength", default=0.62, min=0, max=1, label="PBR blend" },
      { key="ambient", default=0.72, min=0.25, max=1.25, label="Ambient" },
      { key="sunlight", default=0.82, min=0, max=1.5, label="Sunlight" },
      { key="roughness", default=0.68, min=0.05, max=1, label="Roughness" },
      { key="specular", default=0.22, min=0, max=1, label="Specular" },
      { key="wetness", default=0.0, min=0, max=1, label="Wetness" },
    },
  },
  {
    id = "shadow_viewport:post",
    title = "Bloom & Post Processing",
    button = "Bloom & Post...",
    entries = {
      { key="post", default=true, toggle="Post processing" },
      { key="bloom", default=true, toggle="Bloom" },
      { key="bloom_quality", default=1, min=0, max=2, integer=true, label="Bloom quality" },
      { key="bloom_intensity", default=0.34, min=0, max=2, label="Bloom" },
      { key="bloom_threshold", default=0.76, min=0.35, max=1.25, label="Threshold" },
      { key="bloom_radius", default=1.15, min=0.4, max=3, label="Radius" },
      { key="exposure", default=1.0, min=0.4, max=2, label="Exposure" },
      { key="saturation", default=1.12, min=0, max=2, label="Saturation" },
      { key="contrast", default=1.06, min=0.5, max=1.6, label="Contrast" },
      { key="vibrance", default=0.16, min=0, max=1, label="Vibrance" },
      { key="vignette", default=0.08, min=0, max=0.5, label="Vignette" },
    },
  },
}

local defaults = {}
local keys = {}
for _, group in ipairs(groups) do
  for _, entry in ipairs(group.entries) do
    defaults[entry.key] = entry.default
    keys[#keys + 1] = entry.key
  end
end

local settings = minecraft.config.load("visuals.properties", defaults)

local function clamp(value, low, high)
  if value < low then return low end
  if value > high then return high end
  return value
end

local function sanitize()
  for _, group in ipairs(groups) do
    for _, entry in ipairs(group.entries) do
      if entry.min then
        local value = clamp(tonumber(settings[entry.key]) or entry.default, entry.min, entry.max)
        if entry.integer then value = math.floor(value + 0.5) end
        settings[entry.key] = value
      end
    end
  end
end

local function save()
  sanitize()
  minecraft.config.save("visuals.properties", settings, { keys=keys })
end

local function reset(group)
  for _, entry in ipairs(group.entries) do settings[entry.key] = entry.default end
  save()
end

local function reset_all()
  for key, value in pairs(defaults) do settings[key] = value end
  save()
end

sanitize()

return {
  values = settings,
  defaults = defaults,
  save = save,
  reset = reset_all,
}
