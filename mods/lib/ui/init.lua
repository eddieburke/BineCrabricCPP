-- UI Component Library
-- Standardized UI components for consistent mod interfaces

local ui = {}

--------------------------------------------------------------------------------
-- UI ELEMENT BASE CLASS
--------------------------------------------------------------------------------

ui.Element = {}
ui.Element.__index = ui.Element

function ui.Element.new(x, y, width, height)
    local self = setmetatable({}, ui.Element)
    self.x = x or 0
    self.y = y or 0
    self.width = width or 100
    self.height = height or 30
    self.visible = true
    self.enabled = true
    self.children = {}
    self.parent = nil
    return self
end

function ui.Element:add_child(child)
    child.parent = self
    table.insert(self.children, child)
end

function ui.Element:remove_child(child)
    for i, c in ipairs(self.children) do
        if c == child then
            table.remove(self.children, i)
            break
        end
    end
end

function ui.Element:update(dt)
    for _, child in ipairs(self.children) do
        if child.update then child:update(dt) end
    end
end

function ui.Element:render()
    if not self.visible then return end
    for _, child in ipairs(self.children) do
        if child.render then child:render() end
    end
end

function ui.Element:contains_point(x, y)
    return x >= self.x and x <= self.x + self.width and
           y >= self.y and y <= self.y + self.height
end

--------------------------------------------------------------------------------
-- BUTTON
--------------------------------------------------------------------------------

ui.Button = setmetatable({}, {__index = ui.Element})
ui.Button.__index = ui.Button

function ui.Button.new(x, y, width, height, text, callback)
    local self = setmetatable(ui.Element.new(x, y, width, height), ui.Button)
    self.text = text or ""
    self.callback = callback
    self.hovered = false
    self.pressed = false
    self.style = {
        bg_color = {0.2, 0.2, 0.2},
        hover_color = {0.3, 0.3, 0.3},
        text_color = {1, 1, 1},
        border_color = {0.5, 0.5, 0.5}
    }
    return self
end

function ui.Button:handle_input(event_type, x, y)
    if not self.enabled or not self.visible then return false end
    
    local contains = self:contains_point(x, y)
    
    if event_type == "mouse_move" then
        self.hovered = contains
        return contains
    elseif event_type == "mouse_down" and contains then
        self.pressed = true
        return true
    elseif event_type == "mouse_up" and self.pressed then
        self.pressed = false
        if contains and self.callback then
            self.callback()
        end
        return contains
    end
    
    return false
end

function ui.Button:render()
    if not self.visible then return end
    
    -- Render button background
    local color = self.hovered and self.style.hover_color or self.style.bg_color
    if self.pressed then
        color = {color[1] * 0.8, color[2] * 0.8, color[3] * 0.8}
    end
    
    -- Draw rectangle (placeholder for actual rendering)
    love.graphics.setColor(color)
    love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
    
    -- Draw border
    love.graphics.setColor(self.style.border_color)
    love.graphics.rectangle("line", self.x, self.y, self.width, self.height)
    
    -- Draw text
    love.graphics.setColor(self.style.text_color)
    love.graphics.print(self.text, self.x + 5, self.y + 5)
    
    -- Render children
    ui.Element.render(self)
end

--------------------------------------------------------------------------------
-- LABEL
--------------------------------------------------------------------------------

ui.Label = setmetatable({}, {__index = ui.Element})
ui.Label.__index = ui.Label

function ui.Label.new(x, y, text, font_size)
    local self = setmetatable(ui.Element.new(x, y, 0, 0), ui.Label)
    self.text = text or ""
    self.font_size = font_size or 16
    self.style = {
        text_color = {1, 1, 1},
        align = "left"
    }
    return self
end

function ui.Label:render()
    if not self.visible then return end
    
    love.graphics.setColor(self.style.text_color)
    love.graphics.print(self.text, self.x, self.y)
    
    -- Render children
    ui.Element.render(self)
end

--------------------------------------------------------------------------------
-- SLIDER
--------------------------------------------------------------------------------

ui.Slider = setmetatable({}, {__index = ui.Element})
ui.Slider.__index = ui.Slider

function ui.Slider.new(x, y, width, height, min_val, max_val, default_val, callback)
    local self = setmetatable(ui.Element.new(x, y, width, height), ui.Slider)
    self.min_val = min_val or 0
    self.max_val = max_val or 100
    self.value = default_val or ((min_val + max_val) / 2)
    self.callback = callback
    self.dragging = false
    self.handle_width = math.max(10, height * 2)
    self.style = {
        bg_color = {0.2, 0.2, 0.2},
        handle_color = {0.6, 0.6, 0.6},
        hover_color = {0.7, 0.7, 0.7}
    }
    return self
end

function ui.Slider:get_handle_position()
    local t = (self.value - self.min_val) / (self.max_val - self.min_val)
    return self.x + t * (self.width - self.handle_width)
end

function ui.Slider:handle_input(event_type, x, y)
    if not self.enabled or not self.visible then return false end
    
    local handle_x = self:get_handle_position()
    local handle_contains = x >= handle_x and x <= handle_x + self.handle_width and
                           y >= self.y and y <= self.y + self.height
    
    if event_type == "mouse_down" and handle_contains then
        self.dragging = true
        return true
    elseif event_type == "mouse_up" then
        self.dragging = false
        return false
    elseif event_type == "mouse_move" and self.dragging then
        local new_x = math.max(self.x, math.min(x - self.handle_width/2, self.x + self.width - self.handle_width))
        local t = (new_x - self.x) / (self.width - self.handle_width)
        local new_val = self.min_val + t * (self.max_val - self.min_val)
        if new_val ~= self.value then
            self.value = new_val
            if self.callback then
                self.callback(self.value)
            end
        end
        return true
    end
    
    return false
end

function ui.Slider:render()
    if not self.visible then return end
    
    -- Draw track
    love.graphics.setColor(self.style.bg_color)
    love.graphics.rectangle("fill", self.x, self.y + self.height/2 - 2, self.width, 4)
    
    -- Draw handle
    local handle_x = self:get_handle_position()
    local color = self.dragging and self.style.hover_color or self.style.handle_color
    love.graphics.setColor(color)
    love.graphics.rectangle("fill", handle_x, self.y, self.handle_width, self.height)
    
    -- Render children
    ui.Element.render(self)
end

--------------------------------------------------------------------------------
-- PANEL
--------------------------------------------------------------------------------

ui.Panel = setmetatable({}, {__index = ui.Element})
ui.Panel.__index = ui.Panel

function ui.Panel.new(x, y, width, height, title)
    local self = setmetatable(ui.Element.new(x, y, width, height), ui.Panel)
    self.title = title or ""
    self.draggable = true
    self.dragging = false
    self.drag_offset_x = 0
    self.drag_offset_y = 0
    self.style = {
        bg_color = {0.15, 0.15, 0.15, 0.9},
        title_color = {1, 1, 1},
        border_color = {0.3, 0.3, 0.3}
    }
    return self
end

function ui.Panel:handle_input(event_type, x, y)
    if not self.enabled or not self.visible then return false end
    
    if self.draggable and event_type == "mouse_down" and self:contains_point(x, y) then
        self.dragging = true
        self.drag_offset_x = x - self.x
        self.drag_offset_y = y - self.y
        return true
    elseif event_type == "mouse_up" then
        self.dragging = false
        return false
    elseif event_type == "mouse_move" and self.dragging then
        self.x = x - self.drag_offset_x
        self.y = y - self.drag_offset_y
        return true
    end
    
    -- Check children
    for _, child in ipairs(self.children) do
        if child.handle_input and child:handle_input(event_type, x, y) then
            return true
        end
    end
    
    return false
end

function ui.Panel:render()
    if not self.visible then return end
    
    -- Draw panel background
    love.graphics.setColor(self.style.bg_color)
    love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
    
    -- Draw border
    love.graphics.setColor(self.style.border_color)
    love.graphics.rectangle("line", self.x, self.y, self.width, self.height)
    
    -- Draw title bar
    love.graphics.setColor(self.style.title_color)
    love.graphics.print(self.title, self.x + 5, self.y + 5)
    
    -- Render children
    ui.Element.render(self)
end

return ui
