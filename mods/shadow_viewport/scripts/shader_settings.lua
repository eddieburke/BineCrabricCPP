local settings_api = minecraft.require("scripts.settings")
local settings = settings_api.values
local defaults = settings_api.defaults
local save_settings = settings_api.save

local SCREEN_ID = "shadow_viewport:shader_settings"

local ui = { controls = {}, dragging = nil }

local sections = {
  {
    label = "SHADOWS",
    color = 0xFF7EC8F2,
    entries = {
      { key = "shadows", kind = "toggle", label = "Dynamic shadows" },
      { key = "shadow_quality", kind = "slider", label = "Map quality", min = 0, max = 3, integer = true },
      { key = "shadow_filter", kind = "slider", label = "PCF quality", min = 0, max = 2, integer = true },
      { key = "shadow_distance", kind = "slider", label = "Radius", min = 32, max = 192, digits = 0 },
      { key = "shadow_depth", kind = "slider", label = "Depth", min = 128, max = 512, digits = 0 },
      { key = "shadow_strength", kind = "slider", label = "Strength", min = 0, max = 1 },
      { key = "shadow_bias", kind = "slider", label = "Bias", min = 0.005, max = 0.25, digits = 3 },
      { key = "shadow_softness", kind = "slider", label = "Softness", min = 0.4, max = 3 },
      { key = "entity_shadows", kind = "toggle", label = "Entity shadows" },
    },
  },
  {
    label = "LIGHTING",
    color = 0xFFFFD666,
    entries = {
      { key = "pbr", kind = "toggle", label = "PBR-style lighting" },
      { key = "pbr_strength", kind = "slider", label = "PBR blend", min = 0, max = 1 },
      { key = "ambient", kind = "slider", label = "Ambient", min = 0.25, max = 1.25 },
      { key = "sunlight", kind = "slider", label = "Sunlight", min = 0, max = 1.5 },
      { key = "roughness", kind = "slider", label = "Roughness", min = 0.05, max = 1 },
      { key = "specular", kind = "slider", label = "Specular", min = 0, max = 1 },
      { key = "wetness", kind = "slider", label = "Wetness", min = 0, max = 1 },
    },
  },
  {
    label = "POST-PROCESSING",
    color = 0xFFB07EE8,
    entries = {
      { key = "post", kind = "toggle", label = "Post processing" },
      { key = "bloom", kind = "toggle", label = "Bloom" },
      { key = "bloom_quality", kind = "slider", label = "Bloom quality", min = 0, max = 2, integer = true },
      { key = "bloom_intensity", kind = "slider", label = "Bloom", min = 0, max = 2 },
      { key = "bloom_threshold", kind = "slider", label = "Threshold", min = 0.35, max = 1.25 },
      { key = "bloom_radius", kind = "slider", label = "Radius", min = 0.4, max = 3 },
      { key = "exposure", kind = "slider", label = "Exposure", min = 0.4, max = 2 },
      { key = "saturation", kind = "slider", label = "Saturation", min = 0, max = 2 },
      { key = "contrast", kind = "slider", label = "Contrast", min = 0.5, max = 1.6 },
      { key = "vibrance", kind = "slider", label = "Vibrance", min = 0, max = 1 },
      { key = "vignette", kind = "slider", label = "Vignette", min = 0, max = 0.5 },
    },
  },
}

local function slider_format(entry)
  if entry.integer then
    return function(value) return entry.label .. ": " .. tostring(math.floor(value + 0.5)) end
  end
  local pattern = "%." .. (entry.digits or 2) .. "f"
  return function(value) return entry.label .. ": " .. string.format(pattern, value) end
end

local function layout(width, height)
  local controls = {}
  local y = 40
  local x = math.floor(width / 2 - 155)
  local w = 150
  local h = 20
  local row_gap = 24
  local col_gap = 160

  for _, section in ipairs(sections) do
    y = y + 4
    for i, entry in ipairs(section.entries) do
      local col = (i - 1) % 2
      local row = math.floor((i - 1) / 2)
      local cx = x + col * col_gap
      local cy = y + row * row_gap
      controls[#controls + 1] = {
        key = entry.key,
        kind = entry.kind,
        label = entry.label,
        min = entry.min,
        max = entry.max,
        integer = entry.integer,
        digits = entry.digits,
        format = entry.kind == "slider" and slider_format(entry) or nil,
        x = cx,
        y = cy,
        w = w,
        h = h,
        section = section.label,
      }
    end
    local rows = math.ceil(#section.entries / 2)
    y = y + rows * row_gap
  end

  ui.controls = controls
  ui.done_y = y + 8
  ui.reset_y = y + 8
  return ui.done_y + 28
end

local function set_slider(control, mouse_x)
  local normalized = (mouse_x - control.x - 4) / (control.w - 8)
  normalized = math.max(0, math.min(1, normalized))
  local value = control.min + normalized * (control.max - control.min)
  if control.integer then
    value = math.floor(value + 0.5)
  end
  settings[control.key] = value
  save_settings()
end

local function save_and_close()
  save_settings()
  minecraft.screen.close()
end

local function reset_all()
  for key, value in pairs(defaults) do
    settings[key] = value
  end
  save_settings()
end

minecraft.screen.on_lua_screen(SCREEN_ID, {
  init = function(event)
    layout(event.width, event.height)
    local btn_w = 120
    local btn_gap = 8
    local total_w = btn_w * 2 + btn_gap
    local bx = math.floor(event.width / 2 - total_w / 2)
    minecraft.screen.add_button(bx, ui.done_y, btn_w, 20, "Done", save_and_close)
    minecraft.screen.add_button(bx + btn_w + btn_gap, ui.reset_y, btn_w, 20, "Reset to Defaults", reset_all)
  end,

  render = function(event)
    layout(event.width, event.height)

    minecraft.gui.fill_rect(4, 4, event.width - 8, 24, 0xD9121A25)
    minecraft.gui.draw_text(10, 12, "SHADER SETTINGS", 0xFFFFFFFF)

    local current_section = nil
    for _, control in ipairs(ui.controls) do
      if control.section ~= current_section then
        current_section = control.section
        local section_color = 0xFF9BB0C2
        for _, section in ipairs(sections) do
          if section.label == current_section then
            section_color = section.color
            break
          end
        end
        minecraft.gui.draw_text(control.x, control.y - 14, current_section, section_color)
      end

      local value = settings[control.key]
      if control.kind == "slider" then
        local normalized = (value - control.min) / (control.max - control.min)
        local label = control.format(value)
        minecraft.gui.draw_slider({
          x = control.x,
          y = control.y,
          width = control.w,
          height = control.h,
          value = normalized,
          text = label,
          mouse_x = event.mouse_x,
          mouse_y = event.mouse_y,
        })
      else
        minecraft.gui.draw_toggle({
          x = control.x,
          y = control.y,
          width = control.w,
          height = control.h,
          label = control.label,
          value = value,
          mouse_x = event.mouse_x,
          mouse_y = event.mouse_y,
        })
      end
    end
  end,

  mouse = function(event)
    if event.button ~= 0 then return end
    if event.released then
      ui.dragging = nil
      return
    end
    if ui.dragging then
      set_slider(ui.dragging, event.x)
      event.handled = true
      return
    end
    for i = #ui.controls, 1, -1 do
      local control = ui.controls[i]
      if minecraft.util.in_rect(event.x, event.y, control.x, control.y, control.w, control.h) then
        if control.kind == "slider" then
          ui.dragging = control
          set_slider(control, event.x)
        else
          settings[control.key] = not settings[control.key]
          save_settings()
        end
        event.handled = true
        return
      end
    end
  end,

  key = function(event)
    if event.key == minecraft.keys.escape then
      save_and_close()
      event.handled = true
    end
  end,
})

minecraft.screen.on_ui(minecraft.screen.ids.detail_settings, minecraft.screen.regions.footer, function(event)
  if event.ui == nil then return event end
  event.ui:add_stacked_centered_button("Shader Settings...", function()
    minecraft.screen.open(SCREEN_ID, { title = "Shader Settings" })
  end)
  return event
end)

return { screen_id = SCREEN_ID }
