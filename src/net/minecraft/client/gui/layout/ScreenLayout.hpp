#pragma once

namespace net::minecraft::client::gui::layout {

constexpr int kDefaultButtonWidth = 200;
constexpr int kDefaultButtonHeight = 20;
constexpr int kConfirmButtonWidth = 150;
constexpr int kRowSpacing = 24;
constexpr int kSplitRowExtra = 12;
constexpr int kGameMenuOffset = -16;
constexpr int kSplitButtonWidth = 98;
constexpr int kGameMenuBackOffset = 24;
constexpr int kGameMenuSplitOffset = 48;
constexpr int kGameMenuOptionsOffset = 96;
constexpr int kGameMenuQuitOffset = 120;

[[nodiscard]] constexpr int centerBtnX(int screenWidth) noexcept
{
    return screenWidth / 2 - 100;
}

[[nodiscard]] constexpr int dialogFooterY(int screenHeight) noexcept
{
    return screenHeight / 4 + 120 + 12;
}

[[nodiscard]] constexpr int dialogButtonY(int screenHeight) noexcept
{
    return screenHeight / 4 + 120;
}

[[nodiscard]] constexpr int deathScreenBtnY(int screenHeight, int rowIndex) noexcept
{
    return screenHeight / 4 + 72 + rowIndex * kRowSpacing;
}

[[nodiscard]] constexpr int confirmRowY(int screenHeight) noexcept
{
    return screenHeight / 6 + 96;
}

[[nodiscard]] constexpr int confirmBtnX(int screenWidth, bool isConfirm) noexcept
{
    return isConfirm ? screenWidth / 2 - 155 : screenWidth / 2 + 5;
}

[[nodiscard]] constexpr int titleMenuBaseY(int screenHeight) noexcept
{
    return screenHeight / 4 + 48;
}

[[nodiscard]] constexpr int menuRowY(int screenHeight, int rowIndex) noexcept
{
    return titleMenuBaseY(screenHeight) + rowIndex * kRowSpacing;
}

[[nodiscard]] constexpr int menuSplitRowY(int screenHeight, int rowIndex) noexcept
{
    return menuRowY(screenHeight, rowIndex) + kSplitRowExtra;
}

[[nodiscard]] constexpr int formTitleY(int screenHeight) noexcept
{
    return screenHeight / 4 - 60 + 20;
}

[[nodiscard]] constexpr int formBodyLeftX(int screenWidth) noexcept
{
    return screenWidth / 2 - 140;
}

[[nodiscard]] constexpr int formPrimaryBtnY(int screenHeight) noexcept
{
    return screenHeight / 4 + 96 + 12;
}

[[nodiscard]] constexpr int formCancelBtnY(int screenHeight) noexcept
{
    return screenHeight / 4 + 120 + 12;
}

// GameMenuScreen — Java GameMenuScreen.init() with n = kGameMenuOffset.
[[nodiscard]] constexpr int gameMenuButtonY(int screenHeight, int offset) noexcept
{
    return screenHeight / 4 + offset + kGameMenuOffset;
}

[[nodiscard]] constexpr int listFooterRow1Y(int screenHeight) noexcept
{
    return screenHeight - 52;
}

[[nodiscard]] constexpr int listFooterRow2Y(int screenHeight) noexcept
{
    return screenHeight - 28;
}

[[nodiscard]] constexpr int listFooterLeftX(int screenWidth) noexcept
{
    return screenWidth / 2 - 154;
}

[[nodiscard]] constexpr int listFooterRightX(int screenWidth) noexcept
{
    return screenWidth / 2 + 4;
}

[[nodiscard]] constexpr int twoColumnLeftX(int screenWidth) noexcept
{
    return screenWidth / 2 - 155;
}

[[nodiscard]] constexpr int twoColumnRightX(int screenWidth) noexcept
{
    return screenWidth / 2 + 5;
}


[[nodiscard]] constexpr int optionsGridX(int screenWidth, int column) noexcept
{
    return (column % 2 == 0) ? twoColumnLeftX(screenWidth) : twoColumnRightX(screenWidth);
}

[[nodiscard]] constexpr int optionsGridY(int screenHeight, int row) noexcept
{
    return screenHeight / 6 + row * kRowSpacing;
}
} // namespace net::minecraft::client::gui::layout
