#include "net/minecraft/client/gui/widget/EntryListWidget.hpp"

#include <algorithm>

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#ifdef _WIN32
#include "net/minecraft/client/input/InputSystem.hpp"
#endif
namespace net::minecraft::client::gui::widget {
void EntryListWidget::setViewport(int width, int height, int top, int bottom) {
    width_ = width;
    height_ = height;
    top_ = top;
    bottom_ = bottom;
    clampScrolling();
}

EntryListWidget::EntryListWidget(Minecraft& minecraft, int width, int height, int top, int bottom, int itemHeight)
    : minecraft_(minecraft),
      width_(width),
      height_(height),
      top_(top),
      bottom_(bottom),
      left_(0),
      right_(width),
      listLeft_(width / 2 - 110),
      listRight_(width / 2 + 110),
      entryX_(width / 2 - 92 - 16),
      itemHeight_(itemHeight) {
}

int EntryListWidget::getEntriesHeight() const {
    return getEntryCount() * itemHeight_ + headerHeight_;
}

void EntryListWidget::scrollBy(float delta) {
    scrollAmount_ += delta;
    mostYStart_ = -2.0f;
    clampScrolling();
}

void EntryListWidget::clampScrolling() {
    int maxScroll = getEntriesHeight() - (bottom_ - top_ - 4);
    if (maxScroll < 0) {
        maxScroll /= 2;
    }
    if (scrollAmount_ < 0.0f) {
        scrollAmount_ = 0.0f;
    }
    if (scrollAmount_ > static_cast<float>(maxScroll)) {
        scrollAmount_ = static_cast<float>(maxScroll);
    }
}

int EntryListWidget::getEntryAt(int x, int y) const {
    const int listLeft = listLeft_;
    const int listRight = listRight_;
    const int relativeY = y - top_ - headerHeight_ + static_cast<int>(scrollAmount_) - 4;
    const int rowHeight = itemHeight_ > 0 ? itemHeight_ : 1;
    const int index = relativeY / rowHeight;
    if (x >= listLeft && x <= listRight && index >= 0 && relativeY >= 0 && index < getEntryCount()) {
        return index;
    }
    return -1;
}

void EntryListWidget::renderHeader(int x, int y, render::Tessellator& tessellator) {
    (void) x;
    (void) y;
    (void) tessellator;
}

void EntryListWidget::headerClicked(int x, int y) {
    (void) x;
    (void) y;
}

void EntryListWidget::renderDecorations(int mouseX, int mouseY) {
    (void) mouseX;
    (void) mouseY;
}

void EntryListWidget::renderBackgroundStrip(
    render::Tessellator& tessellator, int yStart, int yEnd, int colorTop, int colorBottom) {
    constexpr float tile = 32.0f;
    draw::appendVerticalGradientTexturedQuad(tessellator,
                                             0,
                                             yStart,
                                             width_,
                                             yEnd,
                                             0.0f,
                                             static_cast<float>(yStart) / tile,
                                             static_cast<float>(width_) / tile,
                                             static_cast<float>(yEnd) / tile,
                                             0x404040,
                                             colorTop,
                                             0x404040,
                                             colorBottom);
}

void EntryListWidget::render(int mouseX, int mouseY, float tickDelta) {
    (void) tickDelta;
    renderBackground();
#ifdef _WIN32
    const bool mouseDown = input::InputSystem::instance().isMouseButtonDown(0);
    if (mouseDown) {
        if (mostYStart_ == -1.0f) {
            bool trackScroll = true;
            if (mouseY >= top_ && mouseY <= bottom_) {
                const int listLeft = listLeft_;
                const int listRight = listRight_;
                const int relativeY = mouseY - top_ - headerHeight_ + static_cast<int>(scrollAmount_) - 4;
                const int rowHeight = itemHeight_ > 0 ? itemHeight_ : 1;
                const int index = relativeY / rowHeight;
                if (mouseX >= listLeft && mouseX <= listRight && index >= 0 && relativeY >= 0 &&
                    index < getEntryCount()) {
                    const auto now = std::chrono::steady_clock::now();
                    const bool doubleClick =
                        index == lastClickedPos_ &&
                        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClickTime_).count() < 250;
                    entryClicked(index, doubleClick);
                    lastClickedPos_ = index;
                    lastClickTime_ = now;
                } else if (mouseX >= listLeft && mouseX <= listRight && relativeY < 0) {
                    headerClicked(mouseX - listLeft, mouseY - top_ + static_cast<int>(scrollAmount_) - 4);
                    trackScroll = false;
                }
                const int scrollbarLeft = listRight_ + 6;
                const int scrollbarRight = scrollbarLeft + 6;
                if (mouseX >= scrollbarLeft && mouseX <= scrollbarRight) {
                    scrollSpeedMultiplier_ = -1.0f;
                    int scrollRange = getEntriesHeight() - (bottom_ - top_ - 4);
                    if (scrollRange < 1) {
                        scrollRange = 1;
                    }
                    const int entriesHeight = std::max(1, getEntriesHeight());
                    int thumbHeight = (bottom_ - top_) * (bottom_ - top_) / entriesHeight;
                    if (thumbHeight < 32) {
                        thumbHeight = 32;
                    }
                    if (thumbHeight > bottom_ - top_ - 8) {
                        thumbHeight = bottom_ - top_ - 8;
                    }
                    scrollSpeedMultiplier_ /=
                        static_cast<float>(bottom_ - top_ - thumbHeight) / static_cast<float>(scrollRange);
                } else {
                    scrollSpeedMultiplier_ = 1.0f;
                }
                mostYStart_ = trackScroll ? static_cast<float>(mouseY) : -2.0f;
            } else {
                mostYStart_ = -2.0f;
            }
        } else if (mostYStart_ >= 0.0f) {
            scrollAmount_ -= (static_cast<float>(mouseY) - mostYStart_) * scrollSpeedMultiplier_;
            mostYStart_ = static_cast<float>(mouseY);
        }
    } else {
        mostYStart_ = -1.0f;
    }
#endif
    clampScrolling();
    const gl::preset::ListDraw listCaps;
    render::Tessellator& tessellator = render::Tessellator::INSTANCE;
    const int textureId = minecraft_.textureManager.getTextureId("/gui/background.png");
    gl::pass::bindAtlas2D(textureId);
    constexpr float tile = 32.0f;
    const int scroll = static_cast<int>(scrollAmount_);
    draw::coloredTexturedQuad(tessellator,
                              left_,
                              top_,
                              right_,
                              bottom_,
                              static_cast<float>(left_) / tile,
                              static_cast<float>(top_ + scroll) / tile,
                              static_cast<float>(right_) / tile,
                              static_cast<float>(bottom_ + scroll) / tile,
                              0x202020);
    const int entryCount = getEntryCount();
    const int entryX = entryX_;
    int entryY = top_ + 4 - static_cast<int>(scrollAmount_);
    if (renderHeader_) {
        renderHeader(entryX, entryY, tessellator);
    }
    if (renderSelectionHighlight_) {
        gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
        {
            const gl::preset::ListRowHighlight highlightCaps;
            tessellator.startQuads();
            for (int index = 0; index < entryCount; ++index) {
                if (!isSelectedEntry(index)) {
                    continue;
                }
                const int rowY = entryY + index * itemHeight_ + headerHeight_;
                const int rowHeight = itemHeight_ - 4;
                if (rowY > bottom_ || rowY + rowHeight < top_) {
                    continue;
                }
                const int selLeft = listLeft_;
                const int selRight = listRight_;
                draw::appendColoredQuad(tessellator, selLeft, rowY - 2, selRight, rowY + rowHeight + 2, 0x808080);
                draw::appendColoredQuad(tessellator, selLeft + 1, rowY - 1, selRight - 1, rowY + rowHeight + 1, 0);
            }
            tessellator.draw();
        }
    }
    for (int index = 0; index < entryCount; ++index) {
        const int rowY = entryY + index * itemHeight_ + headerHeight_;
        const int rowHeight = itemHeight_ - 4;
        if (rowY > bottom_ || rowY + rowHeight < top_) {
            continue;
        }
        renderEntry(index, entryX, rowY, rowHeight, tessellator);
    }
    {
        const gl::preset::ListScrollbar scrollbarCaps;
        constexpr int fadeHeight = 4;
        gl::pass::bindAtlas2D(textureId);
        tessellator.startQuads();
        renderBackgroundStrip(tessellator, 0, top_, 255, 255);
        renderBackgroundStrip(tessellator, bottom_, height_, 255, 255);
        tessellator.draw();
        {
            const gl::preset::ListRowHighlight scrollbarFillCaps;
            tessellator.startQuads();
            draw::appendVerticalGradientQuad(tessellator, left_, top_, right_, top_ + fadeHeight, 0, 255, 0, 0);
            draw::appendVerticalGradientQuad(tessellator, left_, bottom_ - fadeHeight, right_, bottom_, 0, 0, 0, 255);
            const int scrollRange = getEntriesHeight() - (bottom_ - top_ - 4);
            if (scrollRange > 0) {
                const int scrollbarLeft = listRight_ + 6;
                const int scrollbarRight = scrollbarLeft + 6;
                const int entriesHeight = std::max(1, getEntriesHeight());
                int thumbHeight = (bottom_ - top_) * (bottom_ - top_) / entriesHeight;
                if (thumbHeight < 32) {
                    thumbHeight = 32;
                }
                if (thumbHeight > bottom_ - top_ - 8) {
                    thumbHeight = bottom_ - top_ - 8;
                }
                int thumbY = static_cast<int>(scrollAmount_) * (bottom_ - top_ - thumbHeight) / scrollRange + top_;
                if (thumbY < top_) {
                    thumbY = top_;
                }
                draw::appendColoredQuad(tessellator, scrollbarLeft, top_, scrollbarRight, bottom_, 0, 255);
                draw::appendColoredQuad(
                    tessellator, scrollbarLeft, thumbY, scrollbarRight, thumbY + thumbHeight, 0x808080, 255);
                draw::appendColoredQuad(
                    tessellator, scrollbarLeft, thumbY, scrollbarRight - 1, thumbY + thumbHeight - 1, 0xC0C0C0, 255);
            }
            tessellator.draw();
        }
        renderDecorations(mouseX, mouseY);
    }
}
}  // namespace net::minecraft::client::gui::widget
