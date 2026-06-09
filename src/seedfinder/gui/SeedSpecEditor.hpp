#pragma once

// Node-based "seed specification" editor — an immediate-mode panel (Shortcuts-app
// style) for authoring advanced search rules without touching raw NBT. Owned and
// driven by SeedfinderScreen; it is not a standalone Screen so the parent keeps all
// of its form state while the editor is open.

#include "seedfinder/config/ConfigSchema.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace net::minecraft::client::font {
class TextRenderer;
}

namespace net::minecraft::client::gui {
class DrawContext;
}

namespace net::minecraft::client::gui::widget {

class SeedSpecEditor {
public:
    // What a click resolved to. Done/Cancel ask the parent to close the editor.
    enum class Result : std::uint8_t { None, Done, Cancel };

    void setNodes(std::vector<seedfinder::config::RuleNode> nodes);
    [[nodiscard]] const std::vector<seedfinder::config::RuleNode>& nodes() const { return nodes_; }
    [[nodiscard]] int ruleCount() const { return static_cast<int>(nodes_.size()); }

    void render(gui::DrawContext& context, font::TextRenderer& textRenderer,
        int screenWidth, int screenHeight, int mouseX, int mouseY);

    [[nodiscard]] Result mouseClicked(font::TextRenderer& textRenderer,
        int screenWidth, int screenHeight, int mouseX, int mouseY, int button);

private:
    // One drawable + hit-testable rectangle. The whole panel is rebuilt into a flat
    // list of these every frame and every click, so there is no widget lifetime to
    // manage and mutations (add/remove/reorder) are plain vector edits.
    struct Control {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        std::string text;
        std::uint32_t bg = 0;   // 0 => label only (no fill, not clickable)
        int fg = 0xFFFFFF;
        bool center = false;
        std::function<void()> action;
    };

    void collect(font::TextRenderer& textRenderer, int screenWidth, int screenHeight,
        std::vector<Control>& out);
    void clampScroll(int screenHeight);

    std::vector<seedfinder::config::RuleNode> nodes_;
    int scroll_ = 0;
    int contentHeight_ = 0;
    Result pendingResult_ = Result::None;
};

} // namespace net::minecraft::client::gui::widget
