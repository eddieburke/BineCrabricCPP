#include "net/minecraft/client/gui/screen/world/CreateWorldScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/SingleplayerInteractionManager.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/world/SelectWorldScreen.hpp"
#include "seedfinder/gui/SeedfinderScreen.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "seedfinder/engine/SeedString.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"

#include "net/minecraft/client/input/InputSystem.hpp"

#include <stdexcept>
#include <string>

namespace net::minecraft::client::gui::screen::world {

CreateWorldScreen::CreateWorldScreen(
    screen::ScreenFactory parentFactory,
    std::string initialWorldName,
    std::string initialSeedText)
    : parentFactory_(std::move(parentFactory)),
      initialWorldName_(std::move(initialWorldName)),
      initialSeedText_(std::move(initialSeedText))
{
    if (!parentFactory_) {
        parentFactory_ = []() { return std::make_unique<SelectWorldScreen>(); };
    }
}


std::string CreateWorldScreen::getWorldSaveName(WorldStorageSource* storageSource, std::string worldName)
{
    if (storageSource == nullptr) {
        return worldName;
    }
    while (storageSource->getWorldProperties(worldName).has_value()) {
        worldName += '-';
    }
    return worldName;
}

void CreateWorldScreen::init()
{
    input::InputSystem::instance().setKeyboardRepeat(true);
    buttons_.clear();
    createButton_ = &addActionButton(layout::centerBtnX(width()), layout::formPrimaryBtnY(height()),
        resource::language::I18n::getTranslation("selectWorld.create"),
        [this] { createWorld(); });
    addActionButton(layout::centerBtnX(width()), layout::formCancelBtnY(height()),
        resource::language::I18n::getTranslation("gui.cancel"),
        [this] { navigateTo(parentFactory_); });
    addCenteredActionButton(height() / 4 + 72 + 12, "Find Seeds",
        [this] {
            const std::string worldName =
                worldNameField_ != nullptr ? worldNameField_->getText() : initialWorldName_;
            const std::string seedText = seedField_ != nullptr ? seedField_->getText() : initialSeedText_;
            const screen::ScreenFactory parent = parentFactory_;
            navigateTo(std::make_unique<SeedfinderScreen>(
                [parent, worldName, seedText]() {
                    return std::make_unique<CreateWorldScreen>(parent, worldName, seedText);
                },
                worldName,
                seedText));
        });

    if (textRenderer() != nullptr) {
        std::string defaultName = resource::language::I18n::getTranslation("selectWorld.newWorld");
        if (!initialWorldName_.empty()) {
            defaultName = initialWorldName_;
        }
        worldNameField_ = std::make_unique<widget::TextFieldWidget>(
            this, textRenderer(), width() / 2 - 100, 60, 200, 20, defaultName);
        worldNameField_->focused = true;
        worldNameField_->setMaxLength(32);
        seedField_ = std::make_unique<widget::TextFieldWidget>(
            this, textRenderer(), width() / 2 - 100, 116, 200, 20, initialSeedText_);
    }
    getSaveDirectoryNames();
    updateCreateButtonState();
}

void CreateWorldScreen::updateCreateButtonState()
{
    if (createButton_ != nullptr) {
        createButton_->active = worldNameField_ != nullptr && !worldNameField_->getText().empty();
    }
}

void CreateWorldScreen::tick()
{
    if (worldNameField_ != nullptr) {
        worldNameField_->tick();
    }
    if (seedField_ != nullptr) {
        seedField_->tick();
    }
}

void CreateWorldScreen::removed()
{
    input::InputSystem::instance().setKeyboardRepeat(false);
}

void CreateWorldScreen::getSaveDirectoryNames()
{
    if (worldNameField_ == nullptr) {
        return;
    }
    worldSaveName_ = worldNameField_->getText();
    const std::size_t start = worldSaveName_.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        worldSaveName_.clear();
    } else {
        const std::size_t end = worldSaveName_.find_last_not_of(" \t\r\n");
        worldSaveName_ = worldSaveName_.substr(start, end - start + 1);
    }
    for (const char invalid : CharacterUtils::invalidCharsWorldName) {
        for (std::size_t i = 0; i < worldSaveName_.size(); ++i) {
            if (worldSaveName_[i] == invalid) {
                worldSaveName_[i] = '_';
            }
        }
    }
    if (MathHelper::isNullOrEmpty(worldSaveName_)) {
        worldSaveName_ = "World";
    }
    if (minecraft() != nullptr) {
        worldSaveName_ = getWorldSaveName(minecraft()->getWorldStorageSource(), worldSaveName_);
    }
}

void CreateWorldScreen::createWorld()
{
    if (createButton_ == nullptr || !createButton_->active || minecraft() == nullptr || creatingLevel_) {
        return;
    }
    creatingLevel_ = true;

    JavaRandom seedRandom;
    std::int64_t seed = seedRandom.nextLong();
    if (seedField_ != nullptr) {
        const std::string seedText = seedField_->getText();
        if (!MathHelper::isNullOrEmpty(seedText)) {
            try {
                const std::int64_t parsed = std::stoll(seedText);
                if (parsed != 0LL) {
                    seed = parsed;
                }
            } catch (const std::exception&) {
                seed = static_cast<std::int64_t>(seedfinder::javaStringHashCode(seedText));
            }
        }
    }

    minecraft()->interactionManager = std::make_unique<SingleplayerInteractionManager>(minecraft());
    const std::string displayName =
        worldNameField_ != nullptr ? worldNameField_->getText() : worldSaveName_;
    minecraft()->startGame(worldSaveName_, displayName, seed);
    closeScreen();
}

void CreateWorldScreen::keyPressed(char character, int keyCode)
{
    if (worldNameField_ != nullptr && worldNameField_->focused) {
        worldNameField_->keyPressed(character, keyCode);
    } else if (seedField_ != nullptr) {
        seedField_->keyPressed(character, keyCode);
    }
    if (character == '\r') {
        createWorld();
    }
    updateCreateButtonState();
    getSaveDirectoryNames();
}

void CreateWorldScreen::mouseClicked(int mouseX, int mouseY, int button)
{
    Screen::mouseClicked(mouseX, mouseY, button);
    if (worldNameField_ != nullptr) {
        worldNameField_->mouseClicked(mouseX, mouseY, button);
    }
    if (seedField_ != nullptr) {
        seedField_->mouseClicked(mouseX, mouseY, button);
    }
}

void CreateWorldScreen::render(int mouseX, int mouseY, float tickDelta)
{
    renderBackground();
    if (textRenderer() != nullptr) {
        drawCenteredTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("selectWorld.create"),
            width() / 2,
            height() / 4 - 60 + 20,
            0xFFFFFF);
        drawTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("selectWorld.enterName"),
            width() / 2 - 100,
            47,
            0xA0A0A0);
        drawTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("selectWorld.resultFolder") + " " + worldSaveName_,
            width() / 2 - 100,
            85,
            0xA0A0A0);
        drawTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("selectWorld.enterSeed"),
            width() / 2 - 100,
            104,
            0xA0A0A0);
        drawTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("selectWorld.seedInfo"),
            width() / 2 - 100,
            140,
            0xA0A0A0);
    }
    if (worldNameField_ != nullptr) {
        worldNameField_->render();
    }
    if (seedField_ != nullptr) {
        seedField_->render();
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

void CreateWorldScreen::handleTab()
{
    if (worldNameField_ == nullptr || seedField_ == nullptr) {
        return;
    }
    if (worldNameField_->focused) {
        worldNameField_->setFocused(false);
        seedField_->setFocused(true);
    } else {
        worldNameField_->setFocused(true);
        seedField_->setFocused(false);
    }
}

} // namespace net::minecraft::client::gui::screen::world
