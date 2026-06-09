#pragma once

// Faithful port of gui.widget.EntryListWidget (beta 1.7.3 MCP).

#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"

#include <chrono>
#include <cstdint>
#include <vector>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::render {
class Tessellator;
}

namespace net::minecraft::client::gui::widget {

class EntryListWidget {
public:
    EntryListWidget(Minecraft& minecraft, int width, int height, int top, int bottom, int itemHeight);
    virtual ~EntryListWidget() = default;

    void setRenderSelectionHighlight(bool renderSelectionHighlight) noexcept
    {
        renderSelectionHighlight_ = renderSelectionHighlight;
    }

    void setHeader(bool renderHeader, int headerHeight) noexcept
    {
        renderHeader_ = renderHeader;
        headerHeight_ = renderHeader ? headerHeight : 0;
    }

    void setListBounds(int listLeft, int listRight, int entryX) noexcept
    {
        listLeft_ = listLeft;
        listRight_ = listRight;
        entryX_ = entryX;
    }

    void setViewport(int width, int height, int top, int bottom);

    void registerButtons(std::vector<ButtonWidget>& buttons, int scrollUp, int scrollDown);
    void buttonClicked(ButtonWidget& button);
    void render(int mouseX, int mouseY, float tickDelta);

    [[nodiscard]] int getEntryAt(int x, int y) const;

protected:
    [[nodiscard]] virtual int getEntryCount() const = 0;
    virtual void entryClicked(int index, bool doubleClick) = 0;
    [[nodiscard]] virtual bool isSelectedEntry(int index) const = 0;
    [[nodiscard]] virtual int getEntriesHeight() const;
    virtual void renderBackground() = 0;
    virtual void renderEntry(int index, int x, int y, int height, render::Tessellator& tessellator) = 0;
    virtual void renderHeader(int x, int y, render::Tessellator& tessellator);
    virtual void headerClicked(int x, int y);
    virtual void renderDecorations(int mouseX, int mouseY);

    Minecraft& minecraft_;
    int width_ = 0;
    int height_ = 0;
    int top_ = 0;
    int bottom_ = 0;
    int left_ = 0;
    int right_ = 0;
    int listLeft_ = 0;
    int listRight_ = 0;
    int entryX_ = 0;
    int itemHeight_ = 0;

private:
    void clampScrolling();
    void renderBackgroundStrip(int yStart, int yEnd, int colorTop, int colorBottom);

    int scrollUpButtonId_ = 0;
    int scrollDownButtonId_ = 0;
    float mostYStart_ = -2.0f;
    float scrollSpeedMultiplier_ = 0.0f;
    float scrollAmount_ = 0.0f;
    int lastClickedPos_ = -1;
    std::chrono::steady_clock::time_point lastClickTime_ {};
    bool renderSelectionHighlight_ = true;
    bool renderHeader_ = false;
    int headerHeight_ = 0;
};

} // namespace net::minecraft::client::gui::widget
