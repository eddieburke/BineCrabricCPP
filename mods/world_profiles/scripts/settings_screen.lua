local settings_screen = {}

function settings_screen.register(spec)
  local ui = { controls = {}, drag = nil, page = 1, pages = 1, width = 0, height = 0 }
  local priority = spec.priority or 100
  local parent_region = spec.parent_region or minecraft.screen.regions.footer
  local function values()
    return type(spec.values) == "function" and spec.values() or spec.values
  end
  local function layout(width, height)
    local all = {}
    for _, slider in ipairs(spec.sliders or {}) do
      slider.kind = "slider"
      all[#all + 1] = slider
    end
    for _, toggle in ipairs(spec.toggles or {}) do
      toggle.kind = "toggle"
      all[#all + 1] = toggle
    end
    ui.width, ui.height = width, height
    local y0 = 44
    local rows = math.max(1, math.floor((height - y0 - 64) / 24))
    local page_size = rows * 2
    ui.pages = math.max(1, math.ceil(#all / page_size))
    ui.page = minecraft.util.clamp(ui.page, 1, ui.pages)
    local first = (ui.page - 1) * page_size + 1
    local last = math.min(#all, first + page_size - 1)
    local controls = {}
    for index = first, last do
      local control = all[index]
      local visible_index = index - first
      control.x = math.floor(width / 2 - 155) + (visible_index % 2) * 160
      control.y = y0 + math.floor(visible_index / 2) * 24
      control.w, control.h = 150, 20
      controls[#controls + 1] = control
    end
    ui.controls = controls
  end
  local function apply_change()
    if spec.on_change then spec.on_change() end
  end
  local function set_slider(control, mouse_x)
    local normalized = minecraft.util.clamp((mouse_x - control.x - 4) / (control.w - 8), 0, 1)
    local value = control.min + normalized * (control.max - control.min)
    if control.integer then value = math.floor(value + 0.5) end
    values()[control.key] = value
    apply_change()
  end
  local function close()
    if spec.on_save then spec.on_save() end
    minecraft.screen.close()
  end
  local function open()
    minecraft.screen.open(spec.id, { title = spec.title })
  end
  local function change_page(delta)
    ui.page = minecraft.util.clamp(ui.page + delta, 1, ui.pages)
    ui.drag = nil
    layout(ui.width, ui.height)
  end
  minecraft.on("screen_ui", {
    screen_id = spec.parent_screen,
    region = parent_region,
    priority = priority,
  }, function(event)
    if event.ui == nil then return event end
    event.ui:add_stacked_centered_button(spec.button_label, open)
    return event
  end)
  minecraft.on("screen_event", {
    screen_id = spec.id,
    priority = priority,
  }, function(event)
    if event.phase == "init" then
      ui.page = 1
      layout(event.width, event.height)
      if ui.pages > 1 then
        minecraft.screen.add_button(math.floor(event.width / 2 - 155), event.height - 52, 150, 20, "< Previous", function()
          change_page(-1)
        end)
        minecraft.screen.add_button(math.floor(event.width / 2 + 5), event.height - 52, 150, 20, "Next >", function()
          change_page(1)
        end)
      end
      if spec.on_reset then
        minecraft.screen.add_button(math.floor(event.width / 2 - 155), event.height - 28, 150, 20, "Reset", spec.on_reset)
        minecraft.screen.add_button(math.floor(event.width / 2 + 5), event.height - 28, 150, 20, "Done", close)
      else
        minecraft.screen.add_button(math.floor(event.width / 2 - 100), event.height - 28, 200, 20, "Done", close)
      end
    elseif event.phase == "render" then
      if ui.drag then set_slider(ui.drag, event.mouse_x) end
      if ui.pages > 1 then
        minecraft.gui.draw_centered_text(event.width / 2, 32,
          "Page " .. tostring(ui.page) .. " / " .. tostring(ui.pages), 0xFFA0A0A0)
      end
      local current = values()
      for _, control in ipairs(ui.controls) do
        local value = current[control.key]
        if control.kind == "slider" then
          local normalized = (value - control.min) / (control.max - control.min)
          local label = control.format and control.format(value) or ((control.label or control.key) .. ": " .. tostring(value))
          minecraft.gui.draw_slider({ x = control.x, y = control.y, width = control.w, height = control.h,
            value = normalized, text = label, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
        else
          minecraft.gui.draw_toggle({ x = control.x, y = control.y, width = control.w, height = control.h,
            label = control.label, value = value, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
        end
      end
    elseif event.phase == "mouse" then
      if event.button ~= 0 then return event end
      if event.released then ui.drag = nil return event end
      for i = #ui.controls, 1, -1 do
        local control = ui.controls[i]
        if minecraft.util.in_rect(event.x, event.y, control.x, control.y, control.w, control.h) then
          if control.kind == "slider" then
            ui.drag = control
            set_slider(control, event.x)
          else
            values()[control.key] = not values()[control.key]
            apply_change()
          end
          event.handled = true
          return event
        end
      end
    elseif event.phase == "key" and event.key == minecraft.key_code("escape") then
      close()
      event.handled = true
    end
    return event
  end)
  return open
end

return settings_screen
