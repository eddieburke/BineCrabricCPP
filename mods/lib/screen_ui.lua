local screen_ui = {}

function screen_ui.add_mod_settings_button(label, callback, priority)
  priority = priority or 0
  minecraft.screen.on_ui(
    minecraft.screen.ids.mod_settings,
    minecraft.screen.regions.footer,
    function(event)
      if event.ui ~= nil then
        event.ui:add_stacked_centered_button(label, callback)
      end
      return event
    end,
    priority
  )
end

return screen_ui
