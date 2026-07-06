#include "net/minecraft/client/gui/screen/world/EditWorldScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/world/SelectWorldScreen.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"
#include <optional>
namespace net::minecraft::client::gui::screen::world {
EditWorldScreen::EditWorldScreen(screen::ScreenFactory parentFactory, std::string worldSaveName)
    : parentFactory_(std::move(parentFactory)), worldSaveName_(std::move(worldSaveName)) {
  if(!parentFactory_) {
    parentFactory_ = []() { return std::make_unique<SelectWorldScreen>(); };
  }
}
void EditWorldScreen::init() {
  enableTextInput();
  buttons_.clear();
  renameButton_ = &addActionButton(layout::centerBtnX(width()), layout::formPrimaryBtnY(height()),
                                   resource::language::I18n::getTranslation("selectWorld.renameButton"),
                                   [this] { renameWorld(); });
  addActionButton(layout::centerBtnX(width()), layout::formCancelBtnY(height()),
                  resource::language::I18n::getTranslation("gui.cancel"), [this] { navigateTo(parentFactory_); });
  std::string initialName;
  if(minecraft() != nullptr) {
    if(WorldStorageSource* storage = minecraft()->getWorldStorageSource()) {
      if(const std::optional<WorldProperties> props = storage->getWorldProperties(worldSaveName_);
         props.has_value()) {
        initialName = props->getName();
      }
    }
  }
  if(textRenderer() != nullptr) {
    levelNameField_ = std::make_unique<widget::TextFieldWidget>(this, textRenderer(), width() / 2 - 100, 60, 200,
                                                                20, initialName);
    levelNameField_->focused = true;
    levelNameField_->setMaxLength(32);
  }
  updateRenameButtonState();
}
void EditWorldScreen::tick() {
  tickTextFields({levelNameField_.get()});
}
void EditWorldScreen::removed() {
  disableTextInput();
}
void EditWorldScreen::updateRenameButtonState() {
  if(renameButton_ != nullptr) {
    renameButton_->active = levelNameField_ != nullptr && !levelNameField_->getText().empty();
  }
}
void EditWorldScreen::renameWorld() {
  if(renameButton_ == nullptr || !renameButton_->active || minecraft() == nullptr || levelNameField_ == nullptr) {
    return;
  }
  if(WorldStorageSource* storage = minecraft()->getWorldStorageSource()) {
    storage->rename(worldSaveName_, levelNameField_->getText());
  }
  navigateTo(parentFactory_());
}
void EditWorldScreen::keyPressed(char character, int keyCode) {
  handleFormKeyPress(character, keyCode, {levelNameField_.get()}, [this] { renameWorld(); });
  updateRenameButtonState();
}
void EditWorldScreen::mouseClicked(int mouseX, int mouseY, int button) {
  Screen::mouseClicked(mouseX, mouseY, button);
  clickTextFields(mouseX, mouseY, button, {levelNameField_.get()});
}
void EditWorldScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr) {
    drawCenteredTextWithShadow(*textRenderer(), resource::language::I18n::getTranslation("selectWorld.renameTitle"),
                               width() / 2, layout::formTitleY(height()), 0xFFFFFF);
    drawTextWithShadow(*textRenderer(), resource::language::I18n::getTranslation("selectWorld.enterName"),
                       width() / 2 - 100, 47, 0xA0A0A0);
  }
  if(levelNameField_ != nullptr) {
    levelNameField_->render();
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::gui::screen::world
