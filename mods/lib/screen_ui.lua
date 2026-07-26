local screen_ui = {}

function screen_ui.add_mod_settings_button(label, callback, priority)
  minecraft.on("screen_ui", {
    screen_id = minecraft.screen.ids.mod_settings,
    region = minecraft.screen.regions.footer,
    priority = priority or 0,
  }, function(event)
      if event.ui ~= nil then
        event.ui:add_stacked_centered_button(label, callback)
      end
      return event
    end)
end

return screen_ui
