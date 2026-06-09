#pragma once



#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"

#include "net/minecraft/client/gui/screen/Screen.hpp"

#include "net/minecraft/client/resource/language/I18n.hpp"



#include <string>

#include <vector>



namespace net::minecraft::client::gui::screen {



class DisconnectedScreen : public Screen {

public:

    DisconnectedScreen(std::string title, std::string reason)

        : title_(resource::language::I18n::getTranslation(title)),

          reason_(resource::language::I18n::getTranslation(reason))

    {

    }



    DisconnectedScreen(std::string titleKey, std::string reasonKey, std::vector<std::string> formatArgs)

        : title_(resource::language::I18n::getTranslation(titleKey)),

          reason_(formatArgs.empty()

                      ? resource::language::I18n::getTranslation(reasonKey)

                      : resource::language::I18n::formatJava(

                            resource::language::I18n::getTranslation(reasonKey), formatArgs))

    {

    }



    void tick() override {}

    void keyPressed(char character, int keyCode) override

    {

        (void)character;

        (void)keyCode;

    }



    void init() override

    {

        buttons_.clear();

        addCenteredActionButton(layout::dialogFooterY(height_),

            resource::language::I18n::getTranslation("gui.toMenu"),

            [this] { quitToTitle(); });

    }



    void render(int mouseX, int mouseY, float tickDelta) override

    {

        renderBackground();

        if (textRenderer_ != nullptr) {

            drawCenteredTextWithShadow(*textRenderer_, title_, width_ / 2, height_ / 2 - 50, 0xFFFFFF);

            drawCenteredTextWithShadow(*textRenderer_, reason_, width_ / 2, height_ / 2 - 10, 0xFFFFFF);

        }

        Screen::render(mouseX, mouseY, tickDelta);

    }



private:

    std::string title_;

    std::string reason_;

};



} // namespace net::minecraft::client::gui::screen

