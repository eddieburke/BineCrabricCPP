#include "net/minecraft/client/gui/screen/pack/PackScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/resource/pack/TexturePack.hpp"
#include "net/minecraft/client/resource/pack/TexturePacks.hpp"

#ifdef _WIN32
#include <shellapi.h>
#endif

namespace net::minecraft::client::gui::screen::pack {

class PackScreen::PackListWidget : public widget::EntryListWidget {
public:
    PackListWidget(PackScreen& owner, Minecraft& minecraft, int width, int height)
        : EntryListWidget(minecraft, width, height, 32, height - 55 + 4, 36),
          owner_(owner)
    {
    }

protected:
    [[nodiscard]] int getEntryCount() const override
    {
        return minecraft_.texturePacks != nullptr
            ? static_cast<int>(minecraft_.texturePacks->getAvailable().size())
            : 0;
    }

    void entryClicked(int index, bool /*doubleClick*/) override
    {
        if (minecraft_.texturePacks == nullptr) {
            return;
        }
        const std::vector<resource::pack::TexturePack*> packs = minecraft_.texturePacks->getAvailable();
        if (index < 0 || index >= static_cast<int>(packs.size())) {
            return;
        }
        if (minecraft_.texturePacks->select(packs[static_cast<std::size_t>(index)])) {
            minecraft_.textureManager.reload();
        }
    }

    [[nodiscard]] bool isSelectedEntry(int index) const override
    {
        if (minecraft_.texturePacks == nullptr) {
            return false;
        }
        const std::vector<resource::pack::TexturePack*> packs = minecraft_.texturePacks->getAvailable();
        if (index < 0 || index >= static_cast<int>(packs.size())) {
            return false;
        }
        return minecraft_.texturePacks->selected == packs[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] int getEntriesHeight() const override
    {
        return getEntryCount() * 36;
    }

    void renderBackground() override
    {
        owner_.renderBackground();
    }

    void renderEntry(int index, int x, int y, int height, render::Tessellator& tessellator) override
    {
        if (minecraft_.texturePacks == nullptr || owner_.textRenderer() == nullptr) {
            return;
        }
        const std::vector<resource::pack::TexturePack*> packs = minecraft_.texturePacks->getAvailable();
        if (index < 0 || index >= static_cast<int>(packs.size())) {
            return;
        }
        resource::pack::TexturePack* pack = packs[static_cast<std::size_t>(index)];
        if (pack == nullptr) {
            return;
        }

        pack->bindIcon(minecraft_.textureManager);
        gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        tessellator.startQuads();
        tessellator.color(0xFFFFFF);
        tessellator.vertex(static_cast<double>(x), static_cast<double>(y + height), 0.0, 0.0, 1.0);
        tessellator.vertex(static_cast<double>(x + 32), static_cast<double>(y + height), 0.0, 1.0, 1.0);
        tessellator.vertex(static_cast<double>(x + 32), static_cast<double>(y), 0.0, 1.0, 0.0);
        tessellator.vertex(static_cast<double>(x), static_cast<double>(y), 0.0, 0.0, 0.0);
        tessellator.draw();

        owner_.drawTextWithShadow(*owner_.textRenderer(), pack->name, x + 34, y + 1, 0xFFFFFF);
        owner_.drawTextWithShadow(*owner_.textRenderer(), pack->descriptionLine1, x + 34, y + 12, 0x808080);
        owner_.drawTextWithShadow(*owner_.textRenderer(), pack->descriptionLine2, x + 34, y + 22, 0x808080);
    }

private:
    PackScreen& owner_;
};

PackScreen::PackScreen(screen::ScreenFactory parentFactory) : parentFactory_(std::move(parentFactory))
{
    if (!parentFactory_) {
        parentFactory_ = []() { return std::make_unique<TitleScreen>(); };
    }
}

PackScreen::~PackScreen() = default;

void PackScreen::init()
{
    buttons_.clear();
    if (minecraft() != nullptr && minecraft()->texturePacks != nullptr) {
        minecraft()->texturePacks->reload();
        texturePacksDir_ = Minecraft::getRunDirectory() / "texturepacks";
    }
    addActionButton(layout::listFooterLeftX(width()), height() - 48,
        resource::language::I18n::getTranslation("texturePack.openFolder"),
        [this] { openTexturePacksFolder(); });
    addActionButton(layout::listFooterRightX(width()), height() - 48,
        resource::language::I18n::getTranslation("gui.done"),
        [this] {
            if (minecraft() != nullptr) {
                minecraft()->textureManager.reload();
            }
            navigateTo(parentFactory_);
        });
    if (minecraft() != nullptr) {
        packList_ = std::make_unique<PackListWidget>(*this, *minecraft(), width(), height());
        std::vector<widget::ButtonWidget> scrollButtons;
        packList_->registerButtons(scrollButtons, 7, 8);
    }
}

void PackScreen::render(int mouseX, int mouseY, float tickDelta)
{
    if (packList_ != nullptr) {
        packList_->render(mouseX, mouseY, tickDelta);
    }
    if (reloadCooldown_ <= 0 && minecraft() != nullptr && minecraft()->texturePacks != nullptr) {
        minecraft()->texturePacks->reload();
        reloadCooldown_ += 20;
    }
    if (textRenderer() != nullptr) {
        drawCenteredTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("texturePack.title"),
            width() / 2,
            16,
            0xFFFFFF);
        drawCenteredTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("texturePack.folderInfo"),
            width() / 2 - 77,
            height() - 26,
            0x808080);
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

void PackScreen::tick()
{
    Screen::tick();
    --reloadCooldown_;
}

void PackScreen::openTexturePacksFolder()
{
#ifdef _WIN32
    if (!texturePacksDir_.empty()) {
        ShellExecuteW(
            nullptr,
            L"open",
            texturePacksDir_.wstring().c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL);
    }
#endif
}

void PackScreen::buttonClicked(widget::ButtonWidget& button)
{
    if (packList_ != nullptr) {
        packList_->buttonClicked(button);
    }
}

} // namespace net::minecraft::client::gui::screen::pack
