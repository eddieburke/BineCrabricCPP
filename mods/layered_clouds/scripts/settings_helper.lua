local M = {}

function M.settings(spec)
  local ui = { controls = {}, drag = nil }
  local priority = spec.priority or 100
  local function values()
    return type(spec.values) == "function" and spec.values() or spec.values
  end
  local function layout(width, height)
    local controls = {}
    for _, slider in ipairs(spec.sliders or {}) do
      slider.kind = "slider"
      controls[#controls + 1] = slider
    end
    for _, toggle in ipairs(spec.toggles or {}) do
      toggle.kind = "toggle"
      controls[#controls + 1] = toggle
    end
    local y0 = math.floor(height / 4)
    for index, control in ipairs(controls) do
      control.x = math.floor(width / 2 - 155) + ((index - 1) % 2) * 160
      control.y = y0 + math.floor((index - 1) / 2) * 24
      control.w, control.h = 150, 20
    end
    ui.controls = controls
    return y0 + math.ceil(#controls / 2) * 24 + 24
  end
  local function apply_change()
    if spec.on_change then
      spec.on_change()
    end
    if spec.on_save then
      spec.on_save()
    end
  end
  local function set_slider(control, mouse_x)
    local normalized = minecraft.util.clamp((mouse_x - control.x - 4) / (control.w - 8), 0, 1)
    local value = control.min + normalized * (control.max - control.min)
    if control.integer then
      value = math.floor(value + 0.5)
    end
    values()[control.key] = value
    apply_change()
  end
  local function close()
    if spec.on_save then
      spec.on_save()
    end
    minecraft.screen.close()
  end
  local function open()
    minecraft.screen.open(spec.id, { title = spec.title })
  end

  minecraft.on(minecraft.events.screen_ui, {
    screen_id = spec.parent_screen,
    region = spec.parent_region,
    priority = priority,
  }, function(event)
    if event.ui == nil then
      return event
    end
    event.ui:add_stacked_centered_button(spec.button_label, open)
    return event
  end)

  minecraft.on(minecraft.events.screen_event, { screen_id = spec.id, priority = priority }, function(event)
    if event.phase == "init" then
      local button_y = layout(event.width, event.height)
      if spec.on_reset then
        minecraft.screen.add_button(math.floor(event.width / 2 - 100), button_y, 200, 20,
          "Reset to Defaults", spec.on_reset)
        button_y = button_y + 24
      end
      minecraft.screen.add_button(math.floor(event.width / 2 - 100), button_y, 200, 20, "Done", close)
    elseif event.phase == "render" then
      if ui.drag then
        set_slider(ui.drag, event.mouse_x)
      end
      local current = values()
      for _, control in ipairs(ui.controls) do
        local value = current[control.key]
        if control.kind == "slider" then
          local normalized = (value - control.min) / (control.max - control.min)
          local label = control.format and control.format(value) or
            ((control.label or control.key) .. ": " .. tostring(value))
          minecraft.gui.draw_slider({ x = control.x, y = control.y, width = control.w, height = control.h,
            value = normalized, text = label, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
        else
          minecraft.gui.draw_toggle({ x = control.x, y = control.y, width = control.w, height = control.h,
            label = control.label, value = value, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
        end
      end
    elseif event.phase == "mouse" then
      if event.button ~= 0 then
        return
      end
      if event.released then
        ui.drag = nil
        return
      end
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
          return
        end
      end
    elseif event.phase == "key" then
      if event.key == minecraft.keys.escape then
        close()
        event.handled = true
      end
    end
  end)
  return open
end

return M
