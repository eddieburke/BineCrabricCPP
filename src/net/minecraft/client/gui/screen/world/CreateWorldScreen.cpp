#include "net/minecraft/client/gui/screen/world/CreateWorldScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/SingleplayerInteractionManager.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
#include "net/minecraft/client/gui/screen/world/SelectWorldScreen.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/util/SeedText.hpp"
#include "net/minecraft/mod/HostScreenRegistry.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
namespace net::minecraft::client::gui::screen::world {
namespace {
constexpr int kFieldWidth = 200;
constexpr int kFieldHeight = 20;
constexpr int kNameLabelY = 47;
constexpr int kNameFieldY = 60;
constexpr int kResultFolderY = 85;
constexpr int kSeedLabelY = 104;
constexpr int kSeedFieldY = 116;
constexpr int kSeedInfoY = 140;
constexpr int kFooterBottomMargin = 4;
constexpr int kTextFooterClearance = 14;
constexpr int kFieldFooterClearanceY = kSeedFieldY + kFieldHeight + 6;
constexpr int kMaxCompactShift = 35;
struct CreateWorldFormLayout {
  int titleY = 0;
  int fieldX = 0;
  int nameLabelY = 0;
  int nameFieldY = 0;
  int resultFolderY = 0;
  int seedLabelY = 0;
  int seedFieldY = 0;
  int seedInfoY = 0;
  bool showSeedInfo = true;
};
[[nodiscard]] int footerStartY(int screenHeight) noexcept {
  return screenHeight / 4 + 72 + 12;
}
[[nodiscard]] CreateWorldFormLayout formLayout(int screenWidth, int screenHeight, int firstFooterY) noexcept {
  const int compactShift =
      firstFooterY > 0 ? std::min(kMaxCompactShift, std::max(0, kFieldFooterClearanceY - firstFooterY)) : 0;
  CreateWorldFormLayout layout{};
  layout.fieldX = screenWidth / 2 - kFieldWidth / 2;
  layout.nameLabelY = kNameLabelY - compactShift;
  layout.nameFieldY = kNameFieldY - compactShift;
  layout.resultFolderY = kResultFolderY - compactShift;
  layout.seedLabelY = kSeedLabelY - compactShift;
  layout.seedFieldY = kSeedFieldY - compactShift;
  layout.seedInfoY = kSeedInfoY - compactShift;
  layout.titleY = std::max(8, std::min(layout::formTitleY(screenHeight), layout.nameLabelY - 16));
  layout.showSeedInfo = firstFooterY <= 0 || layout.seedInfoY + kTextFooterClearance <= firstFooterY;
  return layout;
}
void openCreateWorldHostScreen(const std::unordered_map<std::string, std::string>& fields) {
  if(client::Minecraft::INSTANCE == nullptr) {
    return;
  }
  std::string worldName;
  std::string seedText;
  const auto worldNameIt = fields.find("world_name");
  if(worldNameIt != fields.end()) {
    worldName = worldNameIt->second;
  }
  const auto seedIt = fields.find("seed");
  if(seedIt != fields.end()) {
    seedText = seedIt->second;
  } else if(const auto seedTextIt = fields.find("seed_text"); seedTextIt != fields.end()) {
    seedText = seedTextIt->second;
  }
  client::Minecraft::INSTANCE->setScreen(std::make_unique<CreateWorldScreen>(
      screen::ScreenFactory{}, std::move(worldName), std::move(seedText)));
}
const bool kRegisterCreateWorldHostScreen = []() {
  mod::registerHostScreen(mod::screen_ids::kCreateWorld, openCreateWorldHostScreen);
  return true;
}();
} // namespace
CreateWorldScreen::CreateWorldScreen(
    screen::ScreenFactory parentFactory,
    std::string initialWorldName,
    std::string initialSeedText)
    : parentFactory_(std::move(parentFactory)),
      initialWorldName_(std::move(initialWorldName)),
      initialSeedText_(std::move(initialSeedText)) {
  if(!parentFactory_) {
    parentFactory_ = []() { return std::make_unique<SelectWorldScreen>(); };
  }
}
std::string CreateWorldScreen::getWorldSaveName(WorldStorageSource* storageSource, std::string worldName) {
  if(storageSource == nullptr) {
    return worldName;
  }
  while(storageSource->getWorldProperties(worldName).has_value()) {
    worldName += '-';
  }
  return worldName;
}
void CreateWorldScreen::init() {
  enableTextInput();
  buttons_.clear();
  createButton_ = nullptr;
  const std::size_t footerFirstButton = buttons_.size();
  int nextButtonY = footerStartY(height());
  publishScreenUi(mod::screen_regions::kFooter, &nextButtonY);
  nextButtonY = std::max(nextButtonY, layout::formPrimaryBtnY(height()));
  createButton_ = &addActionButton(layout::centerBtnX(width()), nextButtonY,
                                   resource::language::I18n::getTranslation("selectWorld.create"),
                                   [this] { createWorld(); });
  nextButtonY += layout::kRowSpacing;
  addActionButton(layout::centerBtnX(width()), nextButtonY,
                  resource::language::I18n::getTranslation("gui.cancel"),
                  [this] { navigateTo(parentFactory_); });
  firstFooterButtonY_ = fitFooterButtons(footerFirstButton);
  initTextFields();
  getSaveDirectoryNames();
  updateCreateButtonState();
}
void CreateWorldScreen::initTextFields() {
  if(textRenderer() == nullptr) {
    return;
  }
  const CreateWorldFormLayout layout = formLayout(width(), height(), firstFooterButtonY_);
  if(worldNameField_ == nullptr) {
    std::string defaultName = resource::language::I18n::getTranslation("selectWorld.newWorld");
    if(!initialWorldName_.empty()) {
      defaultName = initialWorldName_;
    }
    worldNameField_ = std::make_unique<widget::TextFieldWidget>(
        this, textRenderer(), layout.fieldX, layout.nameFieldY, kFieldWidth, kFieldHeight, defaultName);
    worldNameField_->focused = true;
    worldNameField_->setMaxLength(32);
  } else {
    worldNameField_->setBounds(layout.fieldX, layout.nameFieldY, kFieldWidth, kFieldHeight);
  }
  if(seedField_ == nullptr) {
    seedField_ = std::make_unique<widget::TextFieldWidget>(
        this, textRenderer(), layout.fieldX, layout.seedFieldY, kFieldWidth, kFieldHeight, initialSeedText_);
  } else {
    seedField_->setBounds(layout.fieldX, layout.seedFieldY, kFieldWidth, kFieldHeight);
  }
}
int CreateWorldScreen::fitFooterButtons(std::size_t firstFooterButton) {
  if(firstFooterButton >= buttons_.size()) {
    return 0;
  }
  int firstY = buttons_[firstFooterButton]->y;
  int bottomY = buttons_[firstFooterButton]->y + buttons_[firstFooterButton]->height;
  for(std::size_t i = firstFooterButton + 1; i < buttons_.size(); ++i) {
    if(buttons_[i] == nullptr || !buttons_[i]->visible) {
      continue;
    }
    firstY = std::min(firstY, buttons_[i]->y);
    bottomY = std::max(bottomY, buttons_[i]->y + buttons_[i]->height);
  }
  const int overflow = bottomY - (height() - kFooterBottomMargin);
  if(overflow > 0) {
    for(std::size_t i = firstFooterButton; i < buttons_.size(); ++i) {
      if(buttons_[i] != nullptr) {
        buttons_[i]->y -= overflow;
      }
    }
    firstY -= overflow;
  }
  return firstY;
}
void CreateWorldScreen::updateCreateButtonState() {
  if(createButton_ != nullptr) {
    createButton_->active = worldNameField_ != nullptr && !worldNameField_->getText().empty();
  }
}
void CreateWorldScreen::tick() {
  tickTextFields({worldNameField_.get(), seedField_.get()});
}
void CreateWorldScreen::removed() {
  disableTextInput();
}
void CreateWorldScreen::getSaveDirectoryNames() {
  if(worldNameField_ == nullptr) {
    return;
  }
  worldSaveName_ = worldNameField_->getText();
  const std::size_t start = worldSaveName_.find_first_not_of(" \t\r\n");
  if(start == std::string::npos) {
    worldSaveName_.clear();
  } else {
    const std::size_t end = worldSaveName_.find_last_not_of(" \t\r\n");
    worldSaveName_ = worldSaveName_.substr(start, end - start + 1);
  }
  for(const char invalid : CharacterUtils::invalidCharsWorldName) {
    for(std::size_t i = 0; i < worldSaveName_.size(); ++i) {
      if(worldSaveName_[i] == invalid) {
        worldSaveName_[i] = '_';
      }
    }
  }
  if(MathHelper::isNullOrEmpty(worldSaveName_)) {
    worldSaveName_ = "World";
  }
  if(minecraft() != nullptr) {
    worldSaveName_ = getWorldSaveName(minecraft()->getWorldStorageSource(), worldSaveName_);
  }
}
void CreateWorldScreen::createWorld() {
  if(createButton_ == nullptr || !createButton_->active || minecraft() == nullptr || creatingLevel_) {
    return;
  }
  creatingLevel_ = true;
  JavaRandom seedRandom;
  std::int64_t seed = seedRandom.nextLong();
  if(seedField_ != nullptr) {
    const std::string seedText = seedField_->getText();
    if(!MathHelper::isNullOrEmpty(seedText)) {
      const std::uint64_t resolved = net::minecraft::util::resolveSeedText(seedText);
      if(resolved != 0ULL) {
        seed = static_cast<std::int64_t>(resolved);
      }
    }
  }
  getSaveDirectoryNames();
  mod::CreateWorldEvent createEvent{&worldSaveName_, seed, false, {}};
  mod::hooks().publish(createEvent);
  if(createEvent.canceled) {
    creatingLevel_ = false;
    return;
  }
  minecraft()->interactionManager = std::make_unique<SingleplayerInteractionManager>(minecraft());
  const std::string displayName =
      worldNameField_ != nullptr ? worldNameField_->getText() : worldSaveName_;
  minecraft()->startGame(worldSaveName_, displayName, seed, createEvent.options);
  closeScreen();
}
void CreateWorldScreen::keyPressed(char character, int keyCode) {
  handleFormKeyPress(character, keyCode, {worldNameField_.get(), seedField_.get()}, [this] { createWorld(); });
  updateCreateButtonState();
  getSaveDirectoryNames();
}
void CreateWorldScreen::mouseClicked(int mouseX, int mouseY, int button) {
  Screen::mouseClicked(mouseX, mouseY, button);
  clickTextFields(mouseX, mouseY, button, {worldNameField_.get(), seedField_.get()});
}
void CreateWorldScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr) {
    const CreateWorldFormLayout layout = formLayout(width(), height(), firstFooterButtonY_);
    drawCenteredTextWithShadow(*textRenderer(),
                               resource::language::I18n::getTranslation("selectWorld.create"),
                               width() / 2,
                               layout.titleY,
                               0xFFFFFF);
    drawTextWithShadow(*textRenderer(),
                       resource::language::I18n::getTranslation("selectWorld.enterName"),
                       layout.fieldX,
                       layout.nameLabelY,
                       0xA0A0A0);
    drawTextWithShadow(*textRenderer(),
                       resource::language::I18n::getTranslation("selectWorld.resultFolder") + " " + worldSaveName_,
                       layout.fieldX,
                       layout.resultFolderY,
                       0xA0A0A0);
    drawTextWithShadow(*textRenderer(),
                       resource::language::I18n::getTranslation("selectWorld.enterSeed"),
                       layout.fieldX,
                       layout.seedLabelY,
                       0xA0A0A0);
    if(layout.showSeedInfo) {
      drawTextWithShadow(*textRenderer(),
                         resource::language::I18n::getTranslation("selectWorld.seedInfo"),
                         layout.fieldX,
                         layout.seedInfoY,
                         0xA0A0A0);
    }
  }
  if(worldNameField_ != nullptr) {
    worldNameField_->render();
  }
  if(seedField_ != nullptr) {
    seedField_->render();
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
void CreateWorldScreen::handleTab() {
  if(worldNameField_ == nullptr || seedField_ == nullptr) {
    return;
  }
  if(worldNameField_->focused) {
    worldNameField_->setFocused(false);
    seedField_->setFocused(true);
  } else {
    worldNameField_->setFocused(true);
    seedField_->setFocused(false);
  }
}
std::string_view CreateWorldScreen::getScreenUiId() const {
  return mod::screen_ids::kCreateWorld;
}
std::string CreateWorldScreen::worldNameFieldText() const {
  if(worldNameField_ != nullptr) {
    return worldNameField_->getText();
  }
  return initialWorldName_;
}
std::string CreateWorldScreen::seedFieldText() const {
  if(seedField_ != nullptr) {
    return seedField_->getText();
  }
  return initialSeedText_;
}
void CreateWorldScreen::setSeedFieldText(std::string text) {
  if(seedField_ != nullptr) {
    seedField_->setText(std::move(text));
    return;
  }
  initialSeedText_ = std::move(text);
}
std::string CreateWorldScreen::hostFieldText(std::string_view name) const {
  if(name == "world_name") {
    return worldNameFieldText();
  }
  if(name == "seed") {
    return seedFieldText();
  }
  return {};
}
bool CreateWorldScreen::setHostFieldText(std::string_view name, std::string value) {
  if(name == "seed") {
    setSeedFieldText(std::move(value));
    return true;
  }
  if(name == "world_name" && worldNameField_ != nullptr) {
    worldNameField_->setText(std::move(value));
    return true;
  }
  return false;
}
void CreateWorldScreen::forEachHostField(const std::function<void(std::string_view, std::string_view)>& fn) const {
  if(fn == nullptr) {
    return;
  }
  fn("world_name", worldNameFieldText());
  fn("seed", seedFieldText());
}
} // namespace net::minecraft::client::gui::screen::world
