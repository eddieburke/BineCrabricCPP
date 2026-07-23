#pragma once
namespace net::minecraft::client::gui::layout {
constexpr int kPanelWidth = 176;
constexpr int kPanelHeightDefault = 166;
constexpr int kSlotStep = 18;
constexpr int kInventoryLabelYOffset = 96;
constexpr int kContainerTitleX = 8;
constexpr int kContainerTitleY = 6;
[[nodiscard]] constexpr int chestPanelHeight(int rows) noexcept {
 return 114 + rows * kSlotStep;
}
[[nodiscard]] constexpr int inventoryLabelY(int backgroundHeight) noexcept {
 return backgroundHeight - kInventoryLabelYOffset + 2;
}
[[nodiscard]] constexpr int containerOriginX(int screenWidth, int backgroundWidth) noexcept {
 return (screenWidth - backgroundWidth) / 2;
}
[[nodiscard]] constexpr int containerOriginY(int screenHeight, int backgroundHeight) noexcept {
 return (screenHeight - backgroundHeight) / 2;
}
} // namespace net::minecraft::client::gui::layout
