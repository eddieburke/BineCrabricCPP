#pragma once

#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/OptionButtonWidget.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace net::minecraft::client::font {
class TextRenderer;
}

namespace net::minecraft::client::gui::layout {

constexpr int kKeybindDoneId = 200;
constexpr int kKeybindButtonWidth = 70;
constexpr int kKeybindLabelOffsetX = kKeybindButtonWidth + 6;

struct KeybindBuildContext {
    screen::Screen& screen;
    option::GameOptions& gameOptions;
};

[[nodiscard]] inline int keybindListX(int screenWidth) noexcept
{
    return screenWidth / 2 - 155;
}

[[nodiscard]] inline int keybindDoneY(int screenHeight) noexcept
{
    return screenHeight / 6 + 168;
}

inline void addKeybindGrid(KeybindBuildContext& ctx)
{
    const int listX = keybindListX(ctx.screen.width());
    for (int i = 0; i < option::GameOptions::kKeybindCount; ++i) {
        ctx.screen.addButton<widget::OptionButtonWidget>(i, listX + (i % 2) * 160,
            ctx.screen.height() / 6 + kRowSpacing * (i / 2), kKeybindButtonWidth, kDefaultButtonHeight,
            ctx.gameOptions.getKeybindKey(i));
    }
}

inline widget::ActionButtonWidget& addKeybindDoneButton(KeybindBuildContext& ctx, std::string doneText,
    std::function<void()> onDone)
{
    return ctx.screen.addActionButton(kKeybindDoneId, centerBtnX(ctx.screen.width()), keybindDoneY(ctx.screen.height()),
        kDefaultButtonWidth, kDefaultButtonHeight, std::move(doneText), std::move(onDone));
}

inline void renderKeybindLabels(font::TextRenderer& textRenderer, KeybindBuildContext& ctx)
{
    const int listX = keybindListX(ctx.screen.width());
    for (int i = 0; i < option::GameOptions::kKeybindCount; ++i) {
        ctx.screen.drawTextWithShadow(textRenderer, ctx.gameOptions.getKeybindName(i),
            listX + (i % 2) * 160 + kKeybindLabelOffsetX, ctx.screen.height() / 6 + kRowSpacing * (i / 2) + 7,
            0xFFFFFFFF);
    }
}

inline void syncKeybindButtonTexts(KeybindBuildContext& ctx,
    const std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons)
{
    for (int i = 0; i < option::GameOptions::kKeybindCount; ++i) {
        if (buttons[static_cast<std::size_t>(i)] != nullptr) {
            buttons[static_cast<std::size_t>(i)]->text = ctx.gameOptions.getKeybindKey(i);
        }
    }
}

} // namespace net::minecraft::client::gui::layout
