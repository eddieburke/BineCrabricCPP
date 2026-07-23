#include "net/minecraft/client/gui/screen/mod/ModsScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/gui/screen/pack/PackScreen.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#ifdef _WIN32
#include <shellapi.h>
#endif
namespace net::minecraft::client::gui::screen::mod {
namespace {
std::string sourceLabel(const net::minecraft::mod::runtime::ModPackage& mod) {
 using net::minecraft::mod::runtime::ModPackageSource;
 switch(mod.source) {
 case ModPackageSource::Directory:
  return "Folder";
 case ModPackageSource::Zip:
  return "Zip";
 }
 return "Unknown";
}
std::string entryStatus(const net::minecraft::mod::runtime::ModPackage& mod) {
 std::string text = mod.configuredEnabled ? "Enabled next launch" : "Disabled next launch";
 text += mod.active ? " | Active now" : " | Inactive now";
 if(mod.runtimeScript) {
  text += " | Lua";
 } else if(mod.resourceOverlay) {
  text += " | Resources";
 }
 return text;
}
std::string detailLine(const net::minecraft::mod::runtime::ModPackage& mod) {
 if(!mod.error.empty()) {
  return mod.error;
 }
 if(!mod.description.empty()) {
  return mod.description;
 }
 std::string text = sourceLabel(mod);
 if(!mod.version.empty()) {
  text += " | v" + mod.version;
 }
 return text;
}
} // namespace
class ModsScreen::ModListWidget : public widget::EntryListWidget {
 public:
 ModListWidget(ModsScreen& owner, Minecraft& minecraft, int width, int height)
     : EntryListWidget(minecraft, width, height, 42, height - 96, 44), owner_(owner) {
 }

 protected:
 [[nodiscard]] int getEntryCount() const override {
  return static_cast<int>(owner_.mods_.size());
 }
 void entryClicked(int index, bool /*doubleClick*/) override {
  if(index < 0 || index >= static_cast<int>(owner_.mods_.size())) {
   return;
  }
  owner_.selectedIndex_ = index;
  owner_.updateToggleButton();
 }
 [[nodiscard]] bool isSelectedEntry(int index) const override {
  return owner_.selectedIndex_ == index;
 }
 [[nodiscard]] int getEntriesHeight() const override {
  return getEntryCount() * 44;
 }
 void renderBackground() override {
  owner_.renderBackground();
 }
 void renderEntry(int index, int x, int y, int height, render::Tessellator& tessellator) override {
  (void)height;
  (void)tessellator;
  if(owner_.textRenderer() == nullptr || index < 0 || index >= static_cast<int>(owner_.mods_.size())) {
   return;
  }
  const net::minecraft::mod::runtime::ModPackage& mod = owner_.mods_[static_cast<std::size_t>(index)];
  const int titleColor = mod.error.empty() ? 0xFFFFFF : 0xFF8080;
  owner_.drawTextWithShadow(*owner_.textRenderer(), mod.name, x, y + 1, titleColor);
  owner_.drawTextWithShadow(*owner_.textRenderer(), entryStatus(mod), x, y + 13, 0xA0A0A0);
  owner_.drawTextWithShadow(
      *owner_.textRenderer(), detailLine(mod), x, y + 25, mod.error.empty() ? 0x808080 : 0xFFB0B0);
 }

 private:
 ModsScreen& owner_;
};
ModsScreen::ModsScreen(screen::ScreenFactory parentFactory) : parentFactory_(std::move(parentFactory)) {
 if(!parentFactory_) {
  parentFactory_ = []() { return std::make_unique<TitleScreen>(); };
 }
}
ModsScreen::~ModsScreen() = default;
void ModsScreen::init() {
 buttons_.clear();
 toggleButton_ = nullptr;
 refreshMods();
 toggleButton_ = &addActionButton(layout::listFooterLeftX(width()),
                                  height() - 76,
                                  layout::kConfirmButtonWidth,
                                  layout::kDefaultButtonHeight,
                                  "",
                                  [this] { toggleSelected(); });
 addActionButton(layout::listFooterRightX(width()),
                 height() - 76,
                 layout::kConfirmButtonWidth,
                 layout::kDefaultButtonHeight,
                 "Reload List",
                 [this] { refreshMods(); });
 addActionButton(layout::listFooterLeftX(width()),
                 height() - 52,
                 layout::kConfirmButtonWidth,
                 layout::kDefaultButtonHeight,
                 "Open mods folder",
                 [this] { openModsFolder(); });
 addActionButton(layout::listFooterRightX(width()),
                 height() - 52,
                 layout::kConfirmButtonWidth,
                 layout::kDefaultButtonHeight,
                 "Texture Packs...",
                 [this] { openTexturePacks(); });
 addCenteredActionButton(height() - 28,
                         layout::kConfirmButtonWidth,
                         layout::kDefaultButtonHeight,
                         resource::language::I18n::getTranslation("gui.done"),
                         [this] { navigateTo(parentFactory_); });
 if(minecraft() != nullptr) {
  modList_ = std::make_unique<ModListWidget>(*this, *minecraft(), width(), height());
 }
 updateToggleButton();
}
void ModsScreen::render(int mouseX, int mouseY, float tickDelta) {
 if(modList_ != nullptr) {
  modList_->render(mouseX, mouseY, tickDelta);
 }
 if(textRenderer() != nullptr) {
  drawCenteredTextWithShadow(*textRenderer(), "Mods", width() / 2, 14, 0xFFFFFF);
  drawCenteredTextWithShadow(
      *textRenderer(), "Zip and folder mods. Changes apply on restart.", width() / 2, 28, 0xA0A0A0);
  if(!footerText_.empty()) {
   drawCenteredTextWithShadow(*textRenderer(), footerText_, width() / 2, height() - 88, 0xD0D0D0);
  }
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
void ModsScreen::tick() {
 Screen::tick();
}
void ModsScreen::refreshMods() {
 net::minecraft::mod::runtime::host().rescan();
 mods_ = net::minecraft::mod::runtime::host().packageMods();
 modsDir_ = net::minecraft::mod::runtime::host().modsDirectory();
 if(mods_.empty()) {
  selectedIndex_ = -1;
 } else if(selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(mods_.size())) {
  selectedIndex_ = 0;
 }
 footerText_ = "Changes take effect after restarting the game.";
 updateToggleButton();
}
void ModsScreen::toggleSelected() {
 if(selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(mods_.size())) {
  return;
 }
 net::minecraft::mod::runtime::ModPackage& mod = mods_[static_cast<std::size_t>(selectedIndex_)];
 mod.configuredEnabled = !mod.configuredEnabled;
 net::minecraft::mod::runtime::host().setEnabled(mod.id, mod.configuredEnabled);
 footerText_ = "Saved. Restart the game to apply the new mod state.";
 updateToggleButton();
}
void ModsScreen::updateToggleButton() {
 if(toggleButton_ == nullptr) {
  return;
 }
 if(selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(mods_.size())) {
  toggleButton_->active = false;
  toggleButton_->text = "Select a mod";
  return;
 }
 toggleButton_->active = true;
 const net::minecraft::mod::runtime::ModPackage& mod = mods_[static_cast<std::size_t>(selectedIndex_)];
 toggleButton_->text = mod.configuredEnabled ? "Disable" : "Enable";
}
void ModsScreen::openModsFolder() {
#ifdef _WIN32
 if(!modsDir_.empty()) {
  ShellExecuteW(nullptr, L"open", modsDir_.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
 }
#endif
}
void ModsScreen::openTexturePacks() {
 screen::ScreenFactory returnToMods = [parentFactory = parentFactory_] {
  return std::make_unique<ModsScreen>(parentFactory);
 };
 navigateTo(std::make_unique<pack::PackScreen>(returnToMods));
}
} // namespace net::minecraft::client::gui::screen::mod
