#include "net/minecraft/client/gui/screen/TitleScreen.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <random>
#include <vector>

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/gui/auth/AccountUiState.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/MultiplayerScreen.hpp"
#include "net/minecraft/client/gui/screen/mod/ModsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/OptionsScreen.hpp"
#include "net/minecraft/client/gui/screen/world/SelectWorldScreen.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::gui::screen {
namespace {
std::vector<std::string> loadSplashes() {
    std::vector<std::string> lines;
    const std::filesystem::path path = texture::TextureManager::resolveResourcePath("title/splashes.txt");
    std::ifstream input(path);
    if (!input.is_open()) {
        return lines;
    }
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}
}  // namespace

TitleScreen::TitleScreen() {
    splashText_ = chooseSplashText();
}

std::string TitleScreen::chooseSplashText() const {
    const std::vector<std::string> splashes = loadSplashes();
    if (splashes.empty()) {
        return "missingno";
    }
    static std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<std::size_t> dist(0, splashes.size() - 1);
    return splashes[dist(rng)];
}

void TitleScreen::init() {
    ticks_ = 0.0f;
    applyCalendarSplash();
    const int w = width();
    const int h = height();
    const auto returnToTitle = []() { return std::make_unique<TitleScreen>(); };
    addCenteredActionButton(
        layout::menuRowY(h, 0), resource::language::I18n::getTranslation("menu.singleplayer"), [this, returnToTitle] {
            navigateTo(std::make_unique<world::SelectWorldScreen>(returnToTitle));
        });
    multiplayerButton_ =
        &addActionButton(layout::centerBtnX(w),
                         layout::menuRowY(h, 1),
                         layout::kSplitButtonWidth,
                         layout::kDefaultButtonHeight,
                         resource::language::I18n::getTranslation("menu.multiplayer"),
                         [this, returnToTitle] { navigateTo(std::make_unique<MultiplayerScreen>(returnToTitle)); });
    addActionButton(layout::centerBtnX(w) + layout::kSplitButtonWidth + 4,
                    layout::menuRowY(h, 1),
                    layout::kSplitButtonWidth,
                    layout::kDefaultButtonHeight,
                    resource::language::I18n::getTranslation("menu.mods"),
                    [this, returnToTitle] { navigateTo(std::make_unique<mod::ModsScreen>(returnToTitle)); });
    addActionButton(layout::centerBtnX(w),
                    layout::menuRowY(h, 2),
                    layout::kSplitButtonWidth,
                    layout::kDefaultButtonHeight,
                    resource::language::I18n::getTranslation("menu.options"),
                    [this, returnToTitle] {
                        if (minecraft() != nullptr) {
                            navigateTo(std::make_unique<option::OptionsScreen>(returnToTitle, &minecraft()->options));
                        }
                    });
    addActionButton(layout::centerBtnX(w) + layout::kSplitButtonWidth + 4,
                    layout::menuRowY(h, 2),
                    layout::kSplitButtonWidth,
                    layout::kDefaultButtonHeight,
                    resource::language::I18n::getTranslation("menu.quit"),
                    [this] { quitGame(); });
    const auth::AccountUiSnapshot accountUi =
        minecraft() != nullptr ? auth::pollAccountUi(*minecraft()) : auth::AccountUiSnapshot{};
    constexpr int kAccountButtonWidth = 100;
    accountButton_ = &addActionButton(w - kAccountButtonWidth - 2,
                                      2,
                                      kAccountButtonWidth,
                                      layout::kDefaultButtonHeight,
                                      accountUi.buttonLabel,
                                      [this, returnToTitle] {
                                          if (minecraft() == nullptr) {
                                              return;
                                          }
                                          const auth::AccountUiSnapshot ui = auth::pollAccountUi(*minecraft());
                                          if (ui.showSignOutButton) {
                                              auth::signOutAccount(*minecraft());
                                              navigateTo(returnToTitle());
                                              return;
                                          }
                                          navigateTo(auth::createLoginScreen(returnToTitle));
                                      });
    if (multiplayerButton_ != nullptr) {
        multiplayerButton_->active = accountUi.multiplayerReady;
    }
}

void TitleScreen::tick() {
    ticks_ += 1.0f;
    if (minecraft() == nullptr) {
        return;
    }
    if (!msauth::isAuthenticated(minecraft()->session) && !msauth::isSavedAccountRestorePending() &&
        !msauth::lastSavedAccountRestoreError().has_value()) {
        msauth::beginRestoreSavedAccount(*minecraft());
    }
    const auth::AccountUiSnapshot accountUi = auth::pollAccountUi(*minecraft());
    if (multiplayerButton_ != nullptr) {
        multiplayerButton_->active = accountUi.multiplayerReady;
    }
    if (accountButton_ != nullptr && accountButton_->text != accountUi.buttonLabel) {
        accountButton_->text = accountUi.buttonLabel;
    }
}

void TitleScreen::render(int mouseX, int mouseY, float tickDelta) {
    renderBackground();
    if (minecraft() == nullptr || textRenderer() == nullptr) {
        return;
    }
    constexpr int logoWidth = 274;
    const int logoX = width() / 2 - logoWidth / 2;
    constexpr int logoY = 30;
    const int logoTexture = minecraft()->textureManager.getTextureId("/title/mclogo.png");
    gl::bindTexture(gl::cap::Texture2D, logoTexture);
    gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
    drawTexture(logoX + 0, logoY + 0, 0, 0, 155, 44);
    drawTexture(logoX + 155, logoY + 0, 0, 45, 155, 44);
    render::INSTANCE.color(0xFFFFFF);
    gl::pushMatrix();
    gl::translatef(static_cast<float>(width() / 2 + 90), 70.0f, 0.0f);
    gl::rotatef(-20.0f, 0.0f, 0.0f, 1.0f);
    const std::int64_t nowMillis =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    const float wave = net::minecraft::util::math::MathHelper::abs(
        net::minecraft::util::math::MathHelper::sin(static_cast<float>(nowMillis % 1000) / 1000.0f * 3.14159265f *
                                                    2.0f) *
        0.1f);
    float scale = 1.8f - wave;
    scale = scale * 100.0f / static_cast<float>(textRenderer()->getWidth(splashText_) + 32);
    gl::scalef(scale, scale, scale);
    drawCenteredTextWithShadow(*textRenderer(), splashText_, 0, -8, 0xFFFF00);
    gl::popMatrix();
    drawTextWithShadow(*textRenderer(), "Minecraft Beta 1.7.3", 2, 2, 0xFF505050);
    const auth::AccountUiSnapshot accountUi = auth::pollAccountUi(*minecraft());
    if (!accountUi.statusLine.empty()) {
        drawTextWithShadow(*textRenderer(), accountUi.statusLine, 2, 14, 0xFFAAAAAA);
    }
    const std::string copyright = "Copyright Mojang AB. Do not distribute.";
    drawTextWithShadow(
        *textRenderer(), copyright, width() - textRenderer()->getWidth(copyright) - 2, height() - 10, 0xFFFFFFFF);
    Screen::render(mouseX, mouseY, tickDelta);
}

void TitleScreen::keyPressed(char character, int keyCode) {
    (void) character;
    (void) keyCode;
}

void TitleScreen::applyCalendarSplash() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    const int month = localTime.tm_mon + 1;
    const int day = localTime.tm_mday;
    if (month == 11 && day == 9) {
        splashText_ = "Happy birthday, ez!";
    } else if (month == 6 && day == 1) {
        splashText_ = "Happy birthday, Notch!";
    } else if (month == 12 && day == 24) {
        splashText_ = "Merry X-mas!";
    } else if (month == 1 && day == 1) {
        splashText_ = "Happy new year!";
    }
}
}  // namespace net::minecraft::client::gui::screen
