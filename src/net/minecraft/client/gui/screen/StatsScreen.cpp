#include "net/minecraft/client/gui/screen/StatsScreen.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"

#include <algorithm>
#include <vector>

namespace net::minecraft::client::gui::screen {

namespace {

class GeneralStatsListWidget : public widget::EntryListWidget {
public:
    GeneralStatsListWidget(Minecraft& minecraft, int width, int height, stat::PlayerStats* stats)
        : EntryListWidget(minecraft, width, height, 32, height - 64, 10), stats_(stats)
    {
        setRenderSelectionHighlight(false);
    }

protected:
    [[nodiscard]] int getEntryCount() const override { return static_cast<int>(stat::Stats::GENERAL_STATS.size()); }
    void entryClicked(int /*index*/, bool /*doubleClick*/) override {}
    [[nodiscard]] bool isSelectedEntry(int /*index*/) const override { return false; }
    void renderBackground() override
    {
        if (minecraft_.currentScreen() != nullptr) {
            minecraft_.currentScreen()->renderBackground();
        }
    }
    void renderEntry(int index, int x, int y, int /*height*/, render::Tessellator& /*tessellator*/) override
    {
        if (stats_ == nullptr || minecraft_.textRenderer == nullptr) {
            return;
        }
        stat::StatId* stat = stat::Stats::GENERAL_STATS[static_cast<std::size_t>(index)];
        const std::string label = resource::language::I18n::getTranslation(stat->name);
        const std::string value = stat->format(stats_->get(*stat));
        const int color = index % 2 == 0 ? 0xFFFFFF : 0x909090;
        minecraft_.textRenderer->drawWithShadow(label, x + 2, y + 1, color);
        minecraft_.textRenderer->drawWithShadow(value, x + 2 + 213 - minecraft_.textRenderer->getWidth(value), y + 1, color);
    }

private:
    stat::PlayerStats* stats_ = nullptr;
};

class AbstractStatsListWidget : public widget::EntryListWidget {
public:
    AbstractStatsListWidget(Minecraft& minecraft, int width, int height, stat::PlayerStats* stats)
        : EntryListWidget(minecraft, width, height, 32, height - 64, 20), stats_(stats)
    {
        setRenderSelectionHighlight(false);
        setHeader(true, 20);
    }

protected:
    [[nodiscard]] int getEntryCount() const override { return static_cast<int>(entries_.size()); }
    void entryClicked(int /*index*/, bool /*doubleClick*/) override {}
    [[nodiscard]] bool isSelectedEntry(int /*index*/) const override { return false; }
    void renderBackground() override
    {
        if (minecraft_.currentScreen() != nullptr) {
            minecraft_.currentScreen()->renderBackground();
        }
    }

    void headerClicked(int x, int y) override
    {
        clickedIconId_ = -1;
        if (x >= 79 && x < 115) {
            clickedIconId_ = 0;
        } else if (x >= 129 && x < 165) {
            clickedIconId_ = 1;
        } else if (x >= 179 && x < 215) {
            clickedIconId_ = 2;
        }
        if (clickedIconId_ >= 0) {
            clickHeader(clickedIconId_);
            minecraft_.audio.play("random.click", 1.0f, 1.0f);
        }
        EntryListWidget::headerClicked(x, y);
    }

    void renderStat(stat::StatId* stat, int x, int y, bool evenRow) const
    {
        if (minecraft_.textRenderer == nullptr || stats_ == nullptr) {
            return;
        }
        const std::string value = stat != nullptr ? stat->format(stats_->getById(stat->id)) : "-";
        const int color = evenRow ? 0xFFFFFF : 0x909090;
        minecraft_.textRenderer->drawWithShadow(value, x - minecraft_.textRenderer->getWidth(value), y + 5, color);
    }

    [[nodiscard]] stat::StatId* getEntry(int index) const { return entries_[static_cast<std::size_t>(index)]; }

    std::vector<stat::StatId*> entries_;
    int clickedIconId_ = -1;
    int selectedTab_ = -1;
    int statSortOrder_ = 0;
    stat::PlayerStats* stats_ = nullptr;

    virtual void clickHeader(int buttonId) = 0;
    virtual void sortEntries() = 0;
};

class ItemStatsListWidget : public AbstractStatsListWidget {
public:
    ItemStatsListWidget(Minecraft& minecraft, int width, int height, stat::PlayerStats* stats)
        : AbstractStatsListWidget(minecraft, width, height, stats)
    {
        for (stat::StatId* stat : stat::Stats::ITEM_STATS) {
            if (stat == nullptr) {
                continue;
            }
            const int itemId = stat->itemOrBlockId;
            bool visible = stats_->getById(stat->id) > 0;
            if (!visible) {
                if (stat::StatId* broken = stat::Stats::getBrokenStat(itemId); broken != nullptr && stats_->getById(broken->id) > 0) {
                    visible = true;
                }
            }
            if (!visible) {
                if (stat::StatId* crafted = stat::Stats::getCraftedStat(itemId); crafted != nullptr && stats_->getById(crafted->id) > 0) {
                    visible = true;
                }
            }
            if (visible) {
                entries_.push_back(stat);
            }
        }
    }

protected:
    void renderHeader(int x, int y, render::Tessellator& tessellator) override
    {
        AbstractStatsListWidget::renderHeader(x, y, tessellator);
        auto* screen = dynamic_cast<StatsScreen*>(minecraft_.currentScreen());
        if (screen == nullptr) {
            return;
        }
        if (clickedIconId_ == 0) {
            screen->renderIcon(x + 115 - 18 + 1, y + 2, 72, 18);
        } else {
            screen->renderIcon(x + 115 - 18, y + 1, 72, 18);
        }
        if (clickedIconId_ == 1) {
            screen->renderIcon(x + 165 - 18 + 1, y + 2, 18, 18);
        } else {
            screen->renderIcon(x + 165 - 18, y + 1, 18, 18);
        }
        if (clickedIconId_ == 2) {
            screen->renderIcon(x + 215 - 18 + 1, y + 2, 36, 18);
        } else {
            screen->renderIcon(x + 215 - 18, y + 1, 36, 18);
        }
    }

    void renderEntry(int index, int x, int y, int /*height*/, render::Tessellator& /*tessellator*/) override
    {
        auto* screen = dynamic_cast<StatsScreen*>(minecraft_.currentScreen());
        stat::StatId* entry = getEntry(index);
        if (screen == nullptr || entry == nullptr) {
            return;
        }
        screen->renderItemIcon(x + 40, y, entry->itemOrBlockId);
        renderStat(stat::Stats::getBrokenStat(entry->itemOrBlockId), x + 115, y, index % 2 == 0);
        renderStat(stat::Stats::getCraftedStat(entry->itemOrBlockId), x + 165, y, index % 2 == 0);
        renderStat(entry, x + 215, y, index % 2 == 0);
    }

    void clickHeader(int buttonId) override
    {
        if (buttonId != selectedTab_) {
            selectedTab_ = buttonId;
            statSortOrder_ = -1;
        } else if (statSortOrder_ == -1) {
            statSortOrder_ = 1;
        } else {
            selectedTab_ = -1;
            statSortOrder_ = 0;
        }
        sortEntries();
    }

    void sortEntries() override
    {
        std::sort(entries_.begin(), entries_.end(), [this](stat::StatId* left, stat::StatId* right) {
            const int leftId = left->itemOrBlockId;
            const int rightId = right->itemOrBlockId;
            stat::StatId* leftStat = nullptr;
            stat::StatId* rightStat = nullptr;
            if (selectedTab_ == 0) {
                leftStat = stat::Stats::getBrokenStat(leftId);
                rightStat = stat::Stats::getBrokenStat(rightId);
            } else if (selectedTab_ == 1) {
                leftStat = stat::Stats::getCraftedStat(leftId);
                rightStat = stat::Stats::getCraftedStat(rightId);
            } else if (selectedTab_ == 2) {
                leftStat = left;
                rightStat = right;
            }
            if (leftStat != nullptr || rightStat != nullptr) {
                if (leftStat == nullptr) {
                    return false;
                }
                if (rightStat == nullptr) {
                    return true;
                }
                const int leftValue = stats_->getById(leftStat->id);
                const int rightValue = stats_->getById(rightStat->id);
                if (leftValue != rightValue) {
                    return (leftValue - rightValue) * statSortOrder_ < 0;
                }
            }
            return leftId < rightId;
        });
    }
};

class BlockStatsListWidget : public AbstractStatsListWidget {
public:
    BlockStatsListWidget(Minecraft& minecraft, int width, int height, stat::PlayerStats* stats)
        : AbstractStatsListWidget(minecraft, width, height, stats)
    {
        for (stat::StatId* stat : stat::Stats::BLOCK_MINED_STATS) {
            if (stat == nullptr) {
                continue;
            }
            const int blockId = stat->itemOrBlockId;
            bool visible = stats_->getById(stat->id) > 0;
            if (!visible) {
                if (stat::StatId* used = stat::Stats::getUsedStat(blockId); used != nullptr && stats_->getById(used->id) > 0) {
                    visible = true;
                }
            }
            if (!visible) {
                if (stat::StatId* crafted = stat::Stats::getCraftedStat(blockId); crafted != nullptr && stats_->getById(crafted->id) > 0) {
                    visible = true;
                }
            }
            if (visible) {
                entries_.push_back(stat);
            }
        }
    }

protected:
    void renderHeader(int x, int y, render::Tessellator& tessellator) override
    {
        AbstractStatsListWidget::renderHeader(x, y, tessellator);
        auto* screen = dynamic_cast<StatsScreen*>(minecraft_.currentScreen());
        if (screen == nullptr) {
            return;
        }
        if (clickedIconId_ == 0) {
            screen->renderIcon(x + 115 - 18 + 1, y + 2, 18, 18);
        } else {
            screen->renderIcon(x + 115 - 18, y + 1, 18, 18);
        }
        if (clickedIconId_ == 1) {
            screen->renderIcon(x + 165 - 18 + 1, y + 2, 36, 18);
        } else {
            screen->renderIcon(x + 165 - 18, y + 1, 36, 18);
        }
        if (clickedIconId_ == 2) {
            screen->renderIcon(x + 215 - 18 + 1, y + 2, 54, 18);
        } else {
            screen->renderIcon(x + 215 - 18, y + 1, 54, 18);
        }
    }

    void renderEntry(int index, int x, int y, int /*height*/, render::Tessellator& /*tessellator*/) override
    {
        auto* screen = dynamic_cast<StatsScreen*>(minecraft_.currentScreen());
        stat::StatId* entry = getEntry(index);
        if (screen == nullptr || entry == nullptr) {
            return;
        }
        screen->renderItemIcon(x + 40, y, entry->itemOrBlockId);
        renderStat(stat::Stats::getCraftedStat(entry->itemOrBlockId), x + 115, y, index % 2 == 0);
        renderStat(stat::Stats::getUsedStat(entry->itemOrBlockId), x + 165, y, index % 2 == 0);
        renderStat(entry, x + 215, y, index % 2 == 0);
    }

    void clickHeader(int buttonId) override
    {
        if (buttonId != selectedTab_) {
            selectedTab_ = buttonId;
            statSortOrder_ = -1;
        } else if (statSortOrder_ == -1) {
            statSortOrder_ = 1;
        } else {
            selectedTab_ = -1;
            statSortOrder_ = 0;
        }
        sortEntries();
    }

    void sortEntries() override
    {
        std::sort(entries_.begin(), entries_.end(), [this](stat::StatId* left, stat::StatId* right) {
            const int leftId = left->itemOrBlockId;
            const int rightId = right->itemOrBlockId;
            stat::StatId* leftStat = nullptr;
            stat::StatId* rightStat = nullptr;
            if (selectedTab_ == 2) {
                leftStat = stat::Stats::getMineBlockStat(leftId);
                rightStat = stat::Stats::getMineBlockStat(rightId);
            } else if (selectedTab_ == 0) {
                leftStat = stat::Stats::getCraftedStat(leftId);
                rightStat = stat::Stats::getCraftedStat(rightId);
            } else if (selectedTab_ == 1) {
                leftStat = stat::Stats::getUsedStat(leftId);
                rightStat = stat::Stats::getUsedStat(rightId);
            }
            if (leftStat != nullptr || rightStat != nullptr) {
                if (leftStat == nullptr) {
                    return false;
                }
                if (rightStat == nullptr) {
                    return true;
                }
                const int leftValue = stats_->getById(leftStat->id);
                const int rightValue = stats_->getById(rightStat->id);
                if (leftValue != rightValue) {
                    return (leftValue - rightValue) * statSortOrder_ < 0;
                }
            }
            return leftId < rightId;
        });
    }
};

} // namespace

StatsScreen::StatsScreen(ScreenFactory parentFactory, stat::PlayerStats* stats)
    : parentFactory_(std::move(parentFactory)), stats_(stats)
{
}

void StatsScreen::init()
{
    title_ = resource::language::I18n::getTranslation("gui.stats");
    buttons_.clear();
    if (minecraft() == nullptr) {
        return;
    }
    generalStats_ = std::make_unique<GeneralStatsListWidget>(*minecraft(), width_, height_, stats_);
    itemStats_ = std::make_unique<ItemStatsListWidget>(*minecraft(), width_, height_, stats_);
    blockStats_ = std::make_unique<BlockStatsListWidget>(*minecraft(), width_, height_, stats_);
    selectedStatsList_ = generalStats_.get();
    createButtons();
}

void StatsScreen::createButtons()
{
    addActionButton(width() / 2 + 4, layout::listFooterRow2Y(height()), 150, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("gui.done"),
        [this] {
            if (parentFactory_) {
                navigateTo(parentFactory_);
            }
        });
    addActionButton(layout::listFooterLeftX(width()), layout::listFooterRow1Y(height()), 100, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("stat.generalButton"),
        [this] { selectedStatsList_ = generalStats_.get(); });
    auto& blocksButton = addActionButton(width() / 2 - 46, layout::listFooterRow1Y(height()), 100, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("stat.blocksButton"),
        [this] { selectedStatsList_ = blockStats_.get(); });
    auto& itemsButton = addActionButton(width() / 2 + 62, layout::listFooterRow1Y(height()), 100, layout::kDefaultButtonHeight,
        resource::language::I18n::getTranslation("stat.itemsButton"),
        [this] { selectedStatsList_ = itemStats_.get(); });

    bool hasBlockEntries = false;
    bool hasItemEntries = false;
    if (stats_ != nullptr) {
        for (stat::StatId* stat : stat::Stats::BLOCK_MINED_STATS) {
            if (stat != nullptr && stats_->getById(stat->id) > 0) {
                hasBlockEntries = true;
                break;
            }
        }
        for (stat::StatId* stat : stat::Stats::ITEM_STATS) {
            if (stat != nullptr && stats_->getById(stat->id) > 0) {
                hasItemEntries = true;
                break;
            }
        }
    }
    blocksButton.active = hasBlockEntries;
    itemsButton.active = hasItemEntries;
}

void StatsScreen::renderIcon(int x, int y, int u, int v)
{
    if (minecraft() == nullptr) {
        return;
    }
    const int textureId = minecraft()->textureManager.getTextureId("/gui/slot.png");
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureId);
    drawTexture(x, y, u, v, 18, 18);
}

void StatsScreen::renderItemIcon(int x, int y, int itemOrBlockId)
{
    if (minecraft() == nullptr || minecraft()->textRenderer == nullptr) {
        return;
    }
    renderIcon(x + 1, y + 1);
    gl::GL11::glEnable(32826);
    gl::GL11::glPushMatrix();
    gl::GL11::glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    render::platform::Lighting::turnOn();
    gl::GL11::glPopMatrix();
    static render::item::ItemRenderer itemRenderer;
    itemRenderer.renderGuiItem(*minecraft()->textRenderer, minecraft()->textureManager, ItemStack(itemOrBlockId), x + 2, y + 2);
    render::platform::Lighting::turnOff();
    gl::GL11::glDisable(32826);
}

void StatsScreen::render(int mouseX, int mouseY, float tickDelta)
{
    if (selectedStatsList_ != nullptr) {
        selectedStatsList_->render(mouseX, mouseY, tickDelta);
    }
    if (textRenderer() != nullptr) {
        drawCenteredTextWithShadow(*textRenderer(), title_, width_ / 2, 20, 0xFFFFFF);
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

void StatsScreen::buttonClicked(widget::ButtonWidget& button)
{
    if (selectedStatsList_ != nullptr) {
        selectedStatsList_->buttonClicked(button);
    }
    (void)button;
}

} // namespace net::minecraft::client::gui::screen
