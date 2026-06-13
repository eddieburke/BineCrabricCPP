#include "net/minecraft/client/gui/screen/world/SelectWorldScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/SingleplayerInteractionManager.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/ConfirmScreen.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/gui/screen/world/CreateWorldScreen.hpp"
#include "net/minecraft/client/gui/screen/world/EditWorldScreen.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace net::minecraft::client::gui::screen::world {

namespace {

std::string formatLastPlayed(std::int64_t millis)
{
    const std::time_t seconds = static_cast<std::time_t>(millis / 1000);
    std::tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &seconds);
#else
    localtime_r(&seconds, &localTime);
#endif
    std::ostringstream out;
    out << std::put_time(&localTime, "%x %X");
    return out.str();
}

} // namespace

class SelectWorldScreen::WorldListWidget : public widget::EntryListWidget {
public:
    WorldListWidget(SelectWorldScreen& owner, Minecraft& minecraft, int width, int height)
        : EntryListWidget(minecraft, width, height, 32, height - 64, 36),
          owner_(owner)
    {
    }

protected:
    [[nodiscard]] int getEntryCount() const override
    {
        return static_cast<int>(owner_.saves_.size());
    }

    void entryClicked(int index, bool doubleClick) override
    {
        owner_.selectedWorldId_ = index;
        const bool enabled = owner_.selectedWorldId_ >= 0 && owner_.selectedWorldId_ < getEntryCount();
        if (owner_.playSelectedWorldButton_ != nullptr) {
            owner_.playSelectedWorldButton_->active = enabled;
        }
        if (owner_.renameWorldButton_ != nullptr) {
            owner_.renameWorldButton_->active = enabled;
        }
        if (owner_.deleteWorldButton_ != nullptr) {
            owner_.deleteWorldButton_->active = enabled;
        }
        if (doubleClick && enabled) {
            owner_.selectWorld(index);
        }
    }

    [[nodiscard]] bool isSelectedEntry(int index) const override
    {
        return index == owner_.selectedWorldId_;
    }

    [[nodiscard]] int getEntriesHeight() const override
    {
        return static_cast<int>(owner_.saves_.size()) * 36;
    }

    void renderBackground() override
    {
        owner_.renderBackground();
    }

    void renderEntry(int index, int x, int y, int height, render::Tessellator& tessellator) override
    {
        (void)height;
        (void)tessellator;
        if (index < 0 || index >= static_cast<int>(owner_.saves_.size()) || owner_.textRenderer() == nullptr) {
            return;
        }
        const WorldSaveInfo& info = owner_.saves_[static_cast<std::size_t>(index)];
        std::string title = info.getName();
        if (title.empty() || MathHelper::isNullOrEmpty(title)) {
            title = owner_.worldText_ + " " + std::to_string(index + 1);
        }
        std::string subtitle = info.getSaveName();
        subtitle += " (" + formatLastPlayed(info.getLastPlayed());
        const float sizeMb =
            static_cast<float>(info.getSize() / 1024LL * 100LL / 1024LL) / 100.0f;
        subtitle += ", " + std::to_string(sizeMb) + " MB)";
        std::string conversion;
        if (info.isSameVersion()) {
            conversion = owner_.conversionText_ + " " + conversion;
        }
        owner_.drawTextWithShadow(*owner_.textRenderer(), title, x + 2, y + 1, 0xFFFFFF);
        owner_.drawTextWithShadow(*owner_.textRenderer(), subtitle, x + 2, y + 12, 0x808080);
        if (!conversion.empty()) {
            owner_.drawTextWithShadow(*owner_.textRenderer(), conversion, x + 2, y + 22, 0x808080);
        }
    }

private:
    SelectWorldScreen& owner_;
};

SelectWorldScreen::~SelectWorldScreen() = default;

SelectWorldScreen::SelectWorldScreen(screen::ScreenFactory parentFactory)
    : parentFactory_(std::move(parentFactory))
{
    if (!parentFactory_) {
        parentFactory_ = []() { return std::make_unique<screen::TitleScreen>(); };
    }
}

void SelectWorldScreen::getSaves()
{
    saves_.clear();
    if (minecraft() == nullptr) {
        selectedWorldId_ = -1;
        return;
    }
    WorldStorageSource* storage = minecraft()->getWorldStorageSource();
    if (storage == nullptr) {
        selectedWorldId_ = -1;
        return;
    }
    saves_ = storage->getAll();
    std::sort(saves_.begin(), saves_.end(), [](const WorldSaveInfo& a, const WorldSaveInfo& b) {
        return a.compareTo(b) < 0;
    });
    selectedWorldId_ = -1;
}

void SelectWorldScreen::addButtons()
{
    playSelectedWorldButton_ = &addActionButton(layout::listFooterLeftX(width()), layout::listFooterRow1Y(height()),
        150, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("selectWorld.select"),
        [this] { selectWorld(selectedWorldId_); });
    renameWorldButton_ = &addActionButton(layout::listFooterLeftX(width()), layout::listFooterRow2Y(height()),
        70, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("selectWorld.rename"),
        [this] {
            navigateTo(std::make_unique<EditWorldScreen>(
                [parent = parentFactory_]() { return std::make_unique<SelectWorldScreen>(parent); },
                getSaveFileName(selectedWorldId_)));
        });
    deleteWorldButton_ = &addActionButton(width() / 2 - 74, layout::listFooterRow2Y(height()),
        70, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("selectWorld.delete"),
        [this] { confirmDeleteWorld(); });
    addActionButton(layout::listFooterRightX(width()), layout::listFooterRow1Y(height()),
        150, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("selectWorld.create"),
        [this] {
            navigateTo(std::make_unique<CreateWorldScreen>([parent = parentFactory_]() {
                return std::make_unique<SelectWorldScreen>(parent);
            }));
        });
    addActionButton(layout::listFooterRightX(width()), layout::listFooterRow2Y(height()),
        150, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("gui.cancel"),
        [this] { navigateTo(parentFactory_); });

    if (playSelectedWorldButton_ != nullptr) {
        playSelectedWorldButton_->active = false;
    }
    if (renameWorldButton_ != nullptr) {
        renameWorldButton_->active = false;
    }
    if (deleteWorldButton_ != nullptr) {
        deleteWorldButton_->active = false;
    }
}

void SelectWorldScreen::init()
{
    title_ = resource::language::I18n::getTranslation("selectWorld.title");
    worldText_ = resource::language::I18n::getTranslation("selectWorld.world");
    conversionText_ = resource::language::I18n::getTranslation("selectWorld.conversion");
    getSaves();
    buttons_.clear();
    if (minecraft() != nullptr) {
        worldList_ = std::make_unique<WorldListWidget>(*this, *minecraft(), width(), height());
    }
    addButtons();
}

std::string SelectWorldScreen::getSaveFileName(int index) const
{
    if (index < 0 || index >= static_cast<int>(saves_.size())) {
        return {};
    }
    return saves_[static_cast<std::size_t>(index)].getSaveName();
}

std::string SelectWorldScreen::getWorldName(int index) const
{
    if (index < 0 || index >= static_cast<int>(saves_.size())) {
        return {};
    }
    std::string name = saves_[static_cast<std::size_t>(index)].getName();
    if (name.empty() || MathHelper::isNullOrEmpty(name)) {
        name = worldText_ + " " + std::to_string(index + 1);
    }
    return name;
}

void SelectWorldScreen::selectWorld(int id)
{
    if (minecraft() == nullptr) {
        return;
    }
    minecraft()->setScreen(nullptr);
    if (selected_) {
        return;
    }
    selected_ = true;
    minecraft()->interactionManager = std::make_unique<SingleplayerInteractionManager>(minecraft());
    std::string saveName = getSaveFileName(id);
    if (saveName.empty()) {
        saveName = "World" + std::to_string(id);
    }
    minecraft()->startGame(saveName, getWorldName(id), 0LL);
    minecraft()->setScreen(nullptr);
}

void SelectWorldScreen::render(int mouseX, int mouseY, float tickDelta)
{
    if (worldList_ != nullptr) {
        worldList_->render(mouseX, mouseY, tickDelta);
    }
    if (textRenderer() != nullptr) {
        drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
    }
    screen::Screen::render(mouseX, mouseY, tickDelta);
}

void SelectWorldScreen::confirmDeleteWorld()
{
    if (minecraft() == nullptr || deleteWorldButton_ == nullptr || !deleteWorldButton_->active) {
        return;
    }
    const std::string worldName = getWorldName(selectedWorldId_);
    if (worldName.empty()) {
        return;
    }
    const std::string question = resource::language::I18n::getTranslation("selectWorld.deleteQuestion");
    const std::string warning = "'" + worldName + "' "
        + resource::language::I18n::getTranslation("selectWorld.deleteWarning");
    const std::string deleteButton =
        resource::language::I18n::getTranslation("selectWorld.deleteButton");
    const std::string cancelButton = resource::language::I18n::getTranslation("gui.cancel");
    const std::string saveName = getSaveFileName(selectedWorldId_);
    const screen::ScreenFactory returnFactory = [parent = parentFactory_]() {
        return std::make_unique<SelectWorldScreen>(parent);
    };
    navigateTo(std::make_unique<screen::ConfirmScreen>(
        returnFactory,
        [saveName](bool confirmed) {
            if (!confirmed || Minecraft::INSTANCE == nullptr) {
                return;
            }
            if (WorldStorageSource* storage = Minecraft::INSTANCE->getWorldStorageSource()) {
                storage->flush();
                storage->deleteSave(saveName);
            }
        },
        question,
        warning,
        deleteButton,
        cancelButton));
}

void SelectWorldScreen::confirmed(bool confirmed, int id)
{
    (void)confirmed;
    (void)id;
}

} // namespace net::minecraft::client::gui::screen::world
