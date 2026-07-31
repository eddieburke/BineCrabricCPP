#pragma once
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::gui::layout {
// Shared scroller for two-column option button grids (Settings / Mod / Shaderpack).
// Tracks content-space Y for widgets + section headers; applies viewport clip, wheel,
// arrow-key scroll, and dragable scrollbar thumb.
class OptionsListScroll {
 public:
 struct Entry {
  int widgetIndex = -1;
  int contentY = 0;
 };
 struct Header {
  std::string text;
  int contentY = 0;
 };
 static constexpr int kDefaultListTop = 32;
 static constexpr int kModListTop = 42;
 static constexpr int kScrollStep = 24;
 static constexpr int kSectionLabelHeight = 14;
 static constexpr int kSectionGap = 8;
 void clear() {
  entries_.clear();
  headers_.clear();
  contentHeight_ = 0;
  maxScroll_ = 0;
  draggingThumb_ = false;
  // Keep scrollOffset_ so rebuilds (resize / pack toggle) stay put.
 }
 void setViewport(int listTop, int listBottom) noexcept {
  listTop_ = listTop;
  listBottom_ = std::max(listTop, listBottom);
  recomputeMaxScroll();
 }
 void setContentHeight(int contentHeight) noexcept {
  contentHeight_ = std::max(0, contentHeight);
  recomputeMaxScroll();
 }
 void addEntry(int widgetIndex, int contentY) {
  entries_.push_back({widgetIndex, contentY});
 }
 void addHeader(std::string text, int contentY) {
  headers_.push_back({std::move(text), contentY});
 }
 // Capture every button in [0, optionCount) using its current absolute Y as contentY
 // relative to listTop (scroll 0 keeps the built layout).
 void captureButtons(const std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons,
                     std::size_t optionCount) {
  entries_.clear();
  for(std::size_t i = 0; i < optionCount && i < buttons.size(); ++i) {
   if(buttons[i] == nullptr) {
    continue;
   }
   entries_.push_back({static_cast<int>(i), buttons[i]->y - listTop_});
  }
  int bottom = 0;
  for(const Entry& entry : entries_) {
   if(entry.widgetIndex < 0 || entry.widgetIndex >= static_cast<int>(buttons.size()) ||
      buttons[static_cast<std::size_t>(entry.widgetIndex)] == nullptr) {
    continue;
   }
   bottom = std::max(bottom, entry.contentY + buttons[static_cast<std::size_t>(entry.widgetIndex)]->height);
  }
  setContentHeight(bottom);
 }
 void apply(std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons) const {
  for(const Entry& entry : entries_) {
   if(entry.widgetIndex < 0 || entry.widgetIndex >= static_cast<int>(buttons.size())) {
    continue;
   }
   auto& button = buttons[static_cast<std::size_t>(entry.widgetIndex)];
   if(button == nullptr) {
    continue;
   }
   button->y = listTop_ + entry.contentY - scrollOffset_;
   // Soft clip: keep partially visible rows interactive.
   button->visible = button->y + button->height > listTop_ && button->y < listBottom_;
  }
 }
 void scrollBy(int amount) {
  if(maxScroll_ <= 0) {
   return;
  }
  scrollOffset_ = std::clamp(scrollOffset_ + amount, 0, maxScroll_);
 }
 [[nodiscard]] bool mouseScrolled(int mouseY, int delta) {
  if(delta == 0 || mouseY < listTop_ || mouseY > listBottom_) {
   return false;
  }
  scrollBy(delta < 0 ? kScrollStep : -kScrollStep);
  return true;
 }
 // Returns true when the click started a scrollbar drag (caller should skip other handling).
 [[nodiscard]] bool mouseClicked(int mouseX, int mouseY, int screenWidth) {
  if(maxScroll_ <= 0 || mouseY < listTop_ || mouseY > listBottom_) {
   draggingThumb_ = false;
   return false;
  }
  const int trackX = scrollbarTrackX(screenWidth);
  if(mouseX < trackX || mouseX > trackX + 6) {
   draggingThumb_ = false;
   return false;
  }
  draggingThumb_ = true;
  jumpThumbToMouse(mouseY);
  return true;
 }
 void mouseReleased() noexcept {
  draggingThumb_ = false;
 }
 void mouseDragged(int mouseY) {
  if(draggingThumb_) {
   jumpThumbToMouse(mouseY);
  }
 }
 void renderFooterDim(int screenWidth, int screenHeight) const;
 void renderScrollbar(int screenWidth) const;
 void renderHeaders(font::TextRenderer* textRenderer, int screenWidth) const;
 [[nodiscard]] int listTop() const noexcept {
  return listTop_;
 }
 [[nodiscard]] int listBottom() const noexcept {
  return listBottom_;
 }
 [[nodiscard]] int scrollOffset() const noexcept {
  return scrollOffset_;
 }
 [[nodiscard]] int maxScroll() const noexcept {
  return maxScroll_;
 }
 [[nodiscard]] int contentHeight() const noexcept {
  return contentHeight_;
 }
 [[nodiscard]] bool draggingThumb() const noexcept {
  return draggingThumb_;
 }
 [[nodiscard]] const std::vector<Header>& headers() const noexcept {
  return headers_;
 }
 void setScrollOffset(int offset) noexcept {
  scrollOffset_ = std::clamp(offset, 0, maxScroll_);
 }

 private:
 void recomputeMaxScroll() noexcept {
  maxScroll_ = std::max(0, contentHeight_ - (listBottom_ - listTop_));
  scrollOffset_ = std::clamp(scrollOffset_, 0, maxScroll_);
 }
 [[nodiscard]] static int scrollbarTrackX(int screenWidth) noexcept {
  return screenWidth / 2 + 164;
 }
 void jumpThumbToMouse(int mouseY) {
  const int trackHeight = std::max(1, listBottom_ - listTop_);
  const int thumbHeight = std::max(16, trackHeight * trackHeight / std::max(trackHeight, contentHeight_));
  const int thumbTravel = std::max(1, trackHeight - thumbHeight);
  const int relative = std::clamp(mouseY - listTop_ - thumbHeight / 2, 0, thumbTravel);
  scrollOffset_ = maxScroll_ * relative / thumbTravel;
 }
 int listTop_ = kDefaultListTop;
 int listBottom_ = kDefaultListTop;
 int contentHeight_ = 0;
 int scrollOffset_ = 0;
 int maxScroll_ = 0;
 bool draggingThumb_ = false;
 std::vector<Entry> entries_;
 std::vector<Header> headers_;
};
} // namespace net::minecraft::client::gui::layout
