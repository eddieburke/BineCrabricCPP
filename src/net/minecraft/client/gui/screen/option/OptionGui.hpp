#pragma once

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/OptionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/SliderWidget.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

#include <cstdio>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <string>

namespace net::minecraft::client::gui::screen::option {

namespace client_option = net::minecraft::client::option;

inline std::string optionLabel(const char* title, const char* value)
{
    return std::string(title) + ": " + value;
}

inline std::string optionLabel(const char* title, const std::string& value)
{
    return std::string(title) + ": " + value;
}

inline std::string percentLabel(const char* title, float value)
{
    if (value == 0.0f) {
        return optionLabel(title, resource::language::I18n::getTranslation("options.off"));
    }
    return optionLabel(title, std::to_string(static_cast<int>(value * 100.0f)) + "%");
}

class OptionGuiBuilder {
public:
    OptionGuiBuilder(screen::Screen& screen, client::Minecraft& minecraft, client_option::GameOptions& options)
        : screen_(screen),
          minecraft_(minecraft),
          options_(options)
    {
        client_option::OptionRegistry::registerAll();
    }

    [[nodiscard]] int gridX(int column) const
    {
        return layout::optionsGridX(screen_.width(), column);
    }

    [[nodiscard]] int gridY(int row) const
    {
        return layout::optionsGridY(screen_.height(), row);
    }

    void toggle(int x, int y, const char* key, const char* title,
        bool (*isEnabled)(const client_option::GameOptions&) = nullptr)
    {
        const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
        if (!idx) {
            return;
        }
        auto format = [title = std::string(title), key = std::string(key)](const client_option::GameOptions& o) {
            const bool on = o.getBoolean(key);
            return optionLabel(title.c_str(),
                resource::language::I18n::getTranslation(on ? "options.on" : "options.off"));
        };
        widget::OptionButtonWidget& btn = screen_.addButton<widget::OptionButtonWidget>(
            static_cast<int>(*idx), x, y, *idx, format(options_), std::move(format));
        if (isEnabled != nullptr) {
            btn.active = isEnabled(options_);
        }
    }

    void boolLabels(int x, int y, const char* key, const char* title, const char* on, const char* off,
        bool (*isEnabled)(const client_option::GameOptions&) = nullptr)
    {
        const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
        if (!idx) {
            return;
        }
        auto format = [title = std::string(title), on = std::string(on), off = std::string(off),
            key = std::string(key)](const client_option::GameOptions& o) {
            return optionLabel(title.c_str(), o.getBoolean(key) ? on.c_str() : off.c_str());
        };
        widget::OptionButtonWidget& btn = screen_.addButton<widget::OptionButtonWidget>(
            static_cast<int>(*idx), x, y, *idx, format(options_), std::move(format));
        if (isEnabled != nullptr) {
            btn.active = isEnabled(options_);
        }
    }

    void intCycle(int x, int y, const char* key, const char* title,
        std::initializer_list<const char*> labels, int client_option::GameOptions::* member,
        bool (*isEnabled)(const client_option::GameOptions&) = nullptr)
    {
        const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
        if (!idx) {
            return;
        }
        const int count = static_cast<int>(labels.size());
        auto format = [title = std::string(title), labels, member, count](const client_option::GameOptions& o) {
            const int value = o.*member;
            const int i = count > 0 ? ((value % count) + count) % count : 0;
            auto it = labels.begin();
            std::advance(it, i);
            return optionLabel(title.c_str(), *it);
        };
        widget::OptionButtonWidget& btn = screen_.addButton<widget::OptionButtonWidget>(
            static_cast<int>(*idx), x, y, *idx, format(options_), std::move(format));
        if (isEnabled != nullptr) {
            btn.active = isEnabled(options_);
        }
    }

    void i18nCycle(int x, int y, const char* key, const char* title,
        std::initializer_list<const char*> i18nKeys, int client_option::GameOptions::* member,
        bool (*isEnabled)(const client_option::GameOptions&) = nullptr)
    {
        const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
        if (!idx) {
            return;
        }
        const int count = static_cast<int>(i18nKeys.size());
        auto format = [title = std::string(title), i18nKeys, member, count](const client_option::GameOptions& o) {
            const int value = o.*member;
            const int i = count > 0 ? ((value % count) + count) % count : 0;
            auto it = i18nKeys.begin();
            std::advance(it, i);
            return optionLabel(title.c_str(), resource::language::I18n::getTranslation(*it));
        };
        widget::OptionButtonWidget& btn = screen_.addButton<widget::OptionButtonWidget>(
            static_cast<int>(*idx), x, y, *idx, format(options_), std::move(format));
        if (isEnabled != nullptr) {
            btn.active = isEnabled(options_);
        }
    }

    void customCycle(int x, int y, const char* key,
        std::function<std::string(const client_option::GameOptions&)> format,
        bool (*isEnabled)(const client_option::GameOptions&) = nullptr)
    {
        const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
        if (!idx) {
            return;
        }
        widget::OptionButtonWidget& btn = screen_.addButton<widget::OptionButtonWidget>(
            static_cast<int>(*idx), x, y, *idx, format(options_), std::move(format));
        if (isEnabled != nullptr) {
            btn.active = isEnabled(options_);
        }
    }

    void slider(int x, int y, const char* key, const char* title,
        std::function<std::string(const client_option::GameOptions&)> format = nullptr,
        bool (*isEnabled)(const client_option::GameOptions&) = nullptr)
    {
        const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
        if (!idx) {
            return;
        }
        if (!format) {
            format = [title = std::string(title), key = std::string(key)](const client_option::GameOptions& o) {
                return percentLabel(title.c_str(), o.getFloat(key));
            };
        }
        widget::SliderWidget& btn = screen_.addButton<widget::SliderWidget>(
            static_cast<int>(*idx), x, y, *idx, minecraft_, format(options_),
            options_.getFloat(key), std::move(format));
        if (isEnabled != nullptr) {
            btn.active = isEnabled(options_);
        }
    }

    void mappedSlider(int x, int y, const char* key,
        std::function<float(const client_option::GameOptions&)> getValue,
        std::function<std::string(const client_option::GameOptions&)> format,
        bool (*isEnabled)(const client_option::GameOptions&) = nullptr)
    {
        const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
        if (!idx) {
            return;
        }
        widget::SliderWidget& btn = screen_.addButton<widget::SliderWidget>(
            static_cast<int>(*idx), x, y, *idx, minecraft_, format(options_),
            getValue(options_), std::move(format));
        if (isEnabled != nullptr) {
            btn.active = isEnabled(options_);
        }
    }

private:
    screen::Screen& screen_;
    client::Minecraft& minecraft_;
    client_option::GameOptions& options_;
};

} // namespace net::minecraft::client::gui::screen::option
