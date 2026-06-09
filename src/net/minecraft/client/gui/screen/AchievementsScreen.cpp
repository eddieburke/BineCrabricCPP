#include "net/minecraft/client/gui/screen/AchievementsScreen.hpp"

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"

#include <chrono>
#include <cmath>

namespace net::minecraft::client::gui::screen {

namespace {

constexpr int kMinRow = achievement::Achievements::minRow * 24 - 112;
constexpr int kMaxRow = achievement::Achievements::maxRow * 24 - 77;
constexpr int kDefaultScrollPixelX =
    achievement::Achievements::OPEN_INVENTORY.column * 24 - 141 / 2 - 12;
constexpr int kDefaultTileColumn = (kDefaultScrollPixelX + 288) >> 4;

int blockTextureId(int blockId)
{
    if (blockId <= 0 || blockId >= block::Block::BLOCK_COUNT) {
        return block::Block::STONE != nullptr ? block::Block::STONE->textureId : 1;
    }
    block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if (block == nullptr) {
        return block::Block::STONE != nullptr ? block::Block::STONE->textureId : 1;
    }
    return block->textureId;
}

int proceduralTerrainTexture(int tileColumn, int tileRow, int row, net::minecraft::JavaRandom& random)
{
    random.setSeed(1234 + tileColumn + row);
    [[maybe_unused]] const int discard = random.nextInt();
    int texture = block::Block::SAND != nullptr ? block::Block::SAND->textureId : 0;
    const int noise = random.nextInt(1 + tileRow + row) + (tileRow + row) / 2;
    if (noise > 37 || tileRow + row == 35) {
        texture = block::Block::BEDROCK != nullptr ? block::Block::BEDROCK->textureId : texture;
    } else if (noise == 22) {
        texture = random.nextInt(2) == 0
            ? (block::Block::DIAMOND_ORE != nullptr ? block::Block::DIAMOND_ORE->textureId : texture)
            : (block::Block::REDSTONE_ORE != nullptr ? block::Block::REDSTONE_ORE->textureId : texture);
    } else if (noise == 10) {
        texture = block::Block::IRON_ORE != nullptr ? block::Block::IRON_ORE->textureId : texture;
    } else if (noise == 8) {
        texture = block::Block::COAL_ORE != nullptr ? block::Block::COAL_ORE->textureId : texture;
    } else if (noise > 4) {
        texture = block::Block::STONE != nullptr ? block::Block::STONE->textureId : texture;
    } else if (noise > 0) {
        texture = block::Block::DIRT != nullptr ? block::Block::DIRT->textureId : texture;
    }
    return texture;
}

std::int64_t nowMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace

AchievementsScreen::AchievementsScreen(stat::PlayerStats* stats) : stats_(stats)
{
    constexpr int centerX = 141;
    constexpr int centerY = 141;
    scaledMouseDx_ = scrollX_ = static_cast<double>(achievement::Achievements::OPEN_INVENTORY.column * 24 - centerX / 2 - 12);
    mouseX_ = scrollX_;
    scaledMouseDy_ = scrollY_ = static_cast<double>(achievement::Achievements::OPEN_INVENTORY.row * 24 - centerY / 2);
    mouseY_ = scrollY_;
}

void AchievementsScreen::init()
{
    buttons_.clear();
    addActionButton(width_ / 2 + 24, height_ / 2 + 74, 80, 20,
        resource::language::I18n::getTranslation("gui.done"),
        [this] {
            if (minecraft() != nullptr) {
                closeScreen();
                minecraft()->lockMouse();
            }
        });
}

void AchievementsScreen::tick()
{
    mouseX_ = scaledMouseDx_;
    mouseY_ = scaledMouseDy_;
    const double dx = scrollX_ - scaledMouseDx_;
    const double dy = scrollY_ - scaledMouseDy_;
    if (dx * dx + dy * dy < 4.0) {
        scaledMouseDx_ += dx;
        scaledMouseDy_ += dy;
    } else {
        scaledMouseDx_ += dx * 0.85;
        scaledMouseDy_ += dy * 0.85;
    }
}

void AchievementsScreen::drawHorizontalLine(int x1, int x2, int y, int color)
{
    if (x2 < x1) {
        std::swap(x1, x2);
    }
    fill(x1, y, x2 + 1, y + 1, static_cast<std::uint32_t>(color));
}

void AchievementsScreen::drawVerticalLine(int x, int y1, int y2, int color)
{
    if (y2 < y1) {
        std::swap(y1, y2);
    }
    fill(x, y1, x + 1, y2 + 1, static_cast<std::uint32_t>(color));
}

void AchievementsScreen::setTitle()
{
    const int frameX = (width_ - iconWidth_) / 2;
    const int frameY = (height_ - iconHeight_) / 2;
    if (textRenderer() != nullptr) {
        textRenderer()->draw("Achievements", frameX + 15, frameY + 5, 0x404040);
    }
}

void AchievementsScreen::renderIcons(int mouseX, int mouseY, float tickDelta)
{
    if (minecraft() == nullptr || stats_ == nullptr || textRenderer() == nullptr) {
        return;
    }

    int scrollPixelX = net::minecraft::util::math::MathHelper::floor(
        static_cast<float>(mouseX_ + (scaledMouseDx_ - mouseX_) * static_cast<double>(tickDelta)));
    int scrollPixelY = net::minecraft::util::math::MathHelper::floor(
        static_cast<float>(mouseY_ + (scaledMouseDy_ - mouseY_) * static_cast<double>(tickDelta)));
    scrollPixelY = std::max(kMinRow, std::min(scrollPixelY, kMaxRow - 1));

    zOffset = 0.0f;
    const int terrainTexture = minecraft()->textureManager.getTextureId("/terrain.png");
    const int achievementTexture = minecraft()->textureManager.getTextureId("/achievement/bg.png");
    const int frameX = (width_ - iconWidth_) / 2;
    const int frameY = (height_ - iconHeight_) / 2;
    const int contentX = frameX + 16;
    const int contentY = frameY + 17;

    gl::GL11::glDepthFunc(gl::GL11::GL_GEQUAL);
    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(0.0f, 0.0f, -200.0f);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
    gl::GL11::glEnable(32826);
    gl::GL11::glEnable(2903);
    minecraft()->textureManager.bindTexture(terrainTexture);

    const int tileColumn = (scrollPixelX + 288) >> 4;
    const int tileRow = (scrollPixelY + 288) >> 4;
    const int offsetX = (scrollPixelX + 288) % 16;
    const int offsetY = (scrollPixelY + 288) % 16;
    net::minecraft::JavaRandom random;

    World* world = minecraft()->world;
    entity::player::ClientPlayerEntity* player = minecraft()->player;
    const bool useWorldTerrain = world != nullptr && player != nullptr;
    const int playerBlockX = useWorldTerrain
        ? net::minecraft::util::math::MathHelper::floor(static_cast<float>(player->x))
        : 0;
    const int playerBlockZ = useWorldTerrain
        ? net::minecraft::util::math::MathHelper::floor(static_cast<float>(player->z))
        : 0;

    for (int row = 0; row * 16 - offsetY < 155; ++row) {
        for (int column = 0; column * 16 - offsetX < 224; ++column) {
            int texture = 0;
            if (useWorldTerrain) {
                const int worldX = playerBlockX + tileColumn + column - kDefaultTileColumn;
                const int depth = tileRow + row;
                int surfaceY = 63;
                if (Chunk* chunk = world->getChunkIfLoaded(worldX, playerBlockZ)) {
                    surfaceY = chunk->getHeight(worldX & 0xF, playerBlockZ & 0xF);
                }
                const int worldY = surfaceY - depth;
                if (worldY < 0 || worldY >= Chunk::height) {
                    texture = block::Block::BEDROCK != nullptr ? block::Block::BEDROCK->textureId : 1;
                } else {
                    const int blockId = world->getBlockId(worldX, worldY, playerBlockZ);
                    if (blockId == 0) {
                        continue;
                    }
                    texture = blockTextureId(blockId);
                }
                const float shade = 0.6f - static_cast<float>(depth) / 25.0f * 0.3f;
                gl::GL11::glColor4f(shade, shade, shade, 1.0f);
            } else {
                const float shade = 0.6f - static_cast<float>(tileRow + row) / 25.0f * 0.3f;
                gl::GL11::glColor4f(shade, shade, shade, 1.0f);
                texture = proceduralTerrainTexture(tileColumn + column, tileRow, row, random);
            }
            drawTexture(
                contentX + column * 16 - offsetX,
                contentY + row * 16 - offsetY,
                (texture % 16) << 4,
                (texture >> 4) << 4,
                16,
                16);
        }
    }

    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glDepthFunc(gl::GL11::GL_LEQUAL);
    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);

    for (const achievement::AchievementDef& achievement : achievement::Achievements::ALL) {
        if (achievement.parentIndex < 0) {
            continue;
        }
        const int childX = achievement.column * 24 - scrollPixelX + 11 + contentX;
        const int childY = achievement.row * 24 - scrollPixelY + 11 + contentY;
        const achievement::AchievementDef* parent = achievement::Achievements::getByStatId(achievement.parentStatId());
        if (parent == nullptr) {
            continue;
        }
        const int parentX = parent->column * 24 - scrollPixelX + 11 + contentX;
        const int parentY = parent->row * 24 - scrollPixelY + 11 + contentY;
        const bool unlocked = stats_->hasStat(achievement.statId());
        const bool parentUnlocked = stats_->hasParentAchievement(achievement.statId());
        const double pulse = std::sin(static_cast<double>(nowMillis() % 600) / 600.0 * 3.141592653589793 * 2.0);
        const int pulseAlpha = pulse > 0.6 ? 255 : 130;
        const int lineColor = unlocked ? 0xFF6F6968 : (parentUnlocked ? (0xFF00FF00 | (pulseAlpha << 24)) : 0xFF000000);
        drawHorizontalLine(childX, parentX, childY, lineColor);
        drawVerticalLine(parentX, childY, parentY, lineColor);
    }

    const achievement::AchievementDef* hovered = nullptr;
    render::item::ItemRenderer itemRenderer;
    gl::GL11::glPushMatrix();
    gl::GL11::glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    render::platform::Lighting::turnOn();
    gl::GL11::glPopMatrix();
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
    gl::GL11::glEnable(32826);
    gl::GL11::glEnable(2903);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);

    for (const achievement::AchievementDef& achievement : achievement::Achievements::ALL) {
        const int iconX = achievement.column * 24 - scrollPixelX;
        const int iconY = achievement.row * 24 - scrollPixelY;
        if (iconX < -24 || iconY < -24 || iconX > 224 || iconY > 155) {
            continue;
        }
        const bool unlocked = stats_->hasStat(achievement.statId());
        const bool parentUnlocked = stats_->hasParentAchievement(achievement.statId());
        float shade = 0.3f;
        if (unlocked) {
            shade = 1.0f;
        } else if (parentUnlocked) {
            const double pulse = std::sin(static_cast<double>(nowMillis() % 600) / 600.0 * 3.141592653589793 * 2.0);
            shade = pulse < 0.6 ? 0.6f : 0.8f;
        }
        gl::GL11::glColor4f(shade, shade, shade, 1.0f);
        minecraft()->textureManager.bindTexture(achievementTexture);
        const int drawX = contentX + iconX;
        const int drawY = contentY + iconY;
        if (achievement.challenge) {
            drawTexture(drawX - 2, drawY - 2, 26, 202, 26, 26);
        } else {
            drawTexture(drawX - 2, drawY - 2, 0, 202, 26, 26);
        }
        if (!parentUnlocked) {
            gl::GL11::glColor4f(0.1f, 0.1f, 0.1f, 1.0f);
            itemRenderer.useCustomDisplayColor = false;
        }
        gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
        gl::GL11::glEnable(2884);
        itemRenderer.renderGuiItem(
            *textRenderer(), minecraft()->textureManager, achievement::Achievements::iconStack(achievement), drawX + 3, drawY + 3);
        gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
        if (!parentUnlocked) {
            itemRenderer.useCustomDisplayColor = true;
        }
        gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        if (mouseX >= contentX && mouseY >= contentY && mouseX < contentX + 224 && mouseY < contentY + 155
            && mouseX >= drawX && mouseX <= drawX + 22 && mouseY >= drawY && mouseY <= drawY + 22) {
            hovered = &achievement;
        }
    }

    gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    minecraft()->textureManager.bindTexture(achievementTexture);
    drawTexture(frameX, frameY, 0, 0, iconWidth_, iconHeight_);
    gl::GL11::glPopMatrix();
    zOffset = 0.0f;
    gl::GL11::glDepthFunc(gl::GL11::GL_LEQUAL);
    gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    Screen::render(mouseX, mouseY, tickDelta);

    if (hovered != nullptr) {
        const std::string title = achievement::Achievements::getTranslatedTitle(*hovered);
        const std::string description = achievement::Achievements::getFormattedDescription(
            *hovered, static_cast<int>(minecraft()->options.inventoryKey.code));
        int tooltipX = mouseX + 12;
        int tooltipY = mouseY - 4;
        if (stats_->hasParentAchievement(hovered->statId())) {
            const int width = std::max(textRenderer()->getWidth(title), 120);
            int bodyHeight = textRenderer()->splitAndGetHeight(description, width);
            if (stats_->hasStat(hovered->statId())) {
                bodyHeight += 12;
            }
            fillGradient(tooltipX - 3, tooltipY - 3, tooltipX + width + 3, tooltipY + bodyHeight + 15,
                0xC0000000U, 0xC0000000U);
            textRenderer()->drawSplit(description, tooltipX, tooltipY + 12, width, 0xFFA09FA0);
            if (stats_->hasStat(hovered->statId())) {
                textRenderer()->drawWithShadow(
                    resource::language::I18n::getTranslation("achievement.taken"), tooltipX, tooltipY + bodyHeight + 4, 0xFF90F7DF);
            }
        } else {
            const achievement::AchievementDef* parent = achievement::Achievements::getByStatId(hovered->parentStatId());
            const std::string parentTitle = parent != nullptr
                ? achievement::Achievements::getTranslatedTitle(*parent)
                : "";
            const std::string requirementText =
                resource::language::I18n::getTranslation("achievement.requires", parentTitle);
            const int width = std::max(textRenderer()->getWidth(title), 120);
            const int bodyHeight = textRenderer()->splitAndGetHeight(requirementText, width);
            fillGradient(tooltipX - 3, tooltipY - 3, tooltipX + width + 3, tooltipY + bodyHeight + 15,
                0xC0000000U, 0xC0000000U);
            textRenderer()->drawSplit(requirementText, tooltipX, tooltipY + 12, width, 0xFF6F6E90);
        }
        const int titleColor = stats_->hasParentAchievement(hovered->statId())
            ? (hovered->challenge ? 0xFFFFFF80 : 0xFFFFFFFF)
            : (hovered->challenge ? 0xFF808080 : 0xFF808080);
        textRenderer()->drawWithShadow(title, tooltipX, tooltipY, titleColor);
    }

    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
    render::platform::Lighting::turnOff();
}

void AchievementsScreen::render(int mouseX, int mouseY, float tickDelta)
{
    if (input::InputSystem::instance().isMouseButtonDown(0)) {
        const int frameX = (width_ - iconWidth_) / 2;
        const int frameY = (height_ - iconHeight_) / 2;
        const int contentX = frameX + 8;
        const int contentY = frameY + 17;
        if ((scroll_ == 0 || scroll_ == 1) && mouseX >= contentX && mouseX < contentX + 224 && mouseY >= contentY
            && mouseY < contentY + 155) {
            if (scroll_ == 0) {
                scroll_ = 1;
            } else {
                scaledMouseDx_ -= static_cast<double>(mouseX - prevMouseX_);
                scaledMouseDy_ -= static_cast<double>(mouseY - prevMouseY_);
                scrollX_ = mouseX_ = scaledMouseDx_;
                scrollY_ = mouseY_ = scaledMouseDy_;
            }
            prevMouseX_ = mouseX;
            prevMouseY_ = mouseY;
        }
        scrollY_ = std::max(static_cast<double>(kMinRow), scrollY_);
        scrollY_ = std::min(static_cast<double>(kMaxRow - 1), scrollY_);
    } else {
        scroll_ = 0;
    }

    renderBackground();
    renderIcons(mouseX, mouseY, tickDelta);
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
    gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
    setTitle();
    gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
}

void AchievementsScreen::keyPressed(char character, int keyCode)
{
    (void)character;
    if (minecraft() != nullptr && keyCode == static_cast<int>(minecraft()->options.inventoryKey.code)) {
        minecraft()->setScreen(nullptr);
        minecraft()->lockMouse();
        return;
    }
    Screen::keyPressed(character, keyCode);
}

void AchievementsScreen::keyPressed(int key)
{
    if (minecraft() != nullptr && key == static_cast<int>(minecraft()->options.inventoryKey.code)) {
        minecraft()->setScreen(nullptr);
        minecraft()->lockMouse();
        return;
    }
    Screen::keyPressed(key);
}

} // namespace net::minecraft::client::gui::screen
