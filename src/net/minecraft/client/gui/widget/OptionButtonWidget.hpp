#pragma once

#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace net::minecraft::client::gui::widget {

class OptionButtonWidget : public ButtonWidget {
public:
    using TextFormat = std::function<std::string(const option::GameOptions&)>;

    OptionButtonWidget(int id, int x, int y, std::string text)
        : OptionButtonWidget(id, x, y, std::nullopt, std::move(text), nullptr)
    {
    }

    OptionButtonWidget(int id, int x, int y, int width, int height, std::string text)
        : ButtonWidget(id, x, y, width, height, std::move(text))
    {
    }

    OptionButtonWidget(int id, int x, int y, std::optional<std::size_t> registryIndex, std::string text,
        TextFormat format)
        : ButtonWidget(id, x, y, 150, 20, std::move(text)),
          registryIndex_(registryIndex),
          format_(std::move(format))
    {
    }

    [[nodiscard]] std::optional<std::size_t> getRegistryIndex() const override
    {
        return registryIndex_;
    }

    void refreshText(const option::GameOptions& options)
    {
        if (format_) {
            text = format_(options);
        }
    }

private:
    std::optional<std::size_t> registryIndex_;
    TextFormat format_;
};

} // namespace net::minecraft::client::gui::widget
