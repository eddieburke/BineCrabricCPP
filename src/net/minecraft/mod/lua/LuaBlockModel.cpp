#include "net/minecraft/mod/lua/LuaBlockModel.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/item/BlockItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/ModClient.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemModel.hpp"
#include "net/minecraft/mod/lua/LuaModNaming.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::mod::lua {
namespace detail {
using net::minecraft::BlockSoundGroup;
using net::minecraft::BlockView;
using net::minecraft::Box;
using net::minecraft::World;
using net::minecraft::block::Block;
using net::minecraft::block::material::Material;
BlockSoundGroup kModBlockSound("stone", 1.0f, 1.0f);

Material* materialFromName(const std::string& name) {
    std::string lowered = name;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    static const std::unordered_map<std::string, Material*> kMaterials = {
        {"stone", &Material::STONE},
        {"metal", &Material::METAL},
        {"wood", &Material::WOOD},
        {"glass", &Material::GLASS},
    };
    const auto it = kMaterials.find(lowered);
    return it != kMaterials.end() ? it->second : &Material::STONE;
}

void registerModBlockDisplayName(const BlockRegistrationSpec& spec) {
    std::string keyToken = spec.translationKey;
    if (keyToken.empty()) {
        keyToken = "block" + std::to_string(spec.blockId);
    }
    const std::string i18nKey = "tile." + keyToken + ".name";
    std::string label = spec.displayName;
    if (label.empty()) {
        label = humanizeTranslationKey(keyToken);
    }
    if (label.empty()) {
        label = "Block " + std::to_string(spec.blockId);
    }
    client::resource::language::I18n::addTranslation(i18nKey, std::move(label));
}

Box offsetBox(const Box& local, int x, int y, int z) {
    return {static_cast<double>(x) + local.minX,
            static_cast<double>(y) + local.minY,
            static_cast<double>(z) + local.minZ,
            static_cast<double>(x) + local.maxX,
            static_cast<double>(y) + local.maxY,
            static_cast<double>(z) + local.maxZ};
}

Box coordinateVariedBounds(const LuaBlockModelSpec& model, int x, int y, int z) {
    const std::int64_t seed = static_cast<std::int64_t>(x) * 3129871LL ^ static_cast<std::int64_t>(y) * 116129781LL ^
                              static_cast<std::int64_t>(z);
    JavaRandom random(static_cast<std::uint64_t>(seed));
    const float halfOffset = model.boundsOffset * 0.5f;
    const float xOffset = random.nextFloat() * model.boundsOffset - halfOffset;
    const float yOffset = random.nextFloat() * model.boundsOffset - halfOffset;
    const float zOffset = random.nextFloat() * model.boundsOffset - halfOffset;
    const float minScale = std::min(model.minScale, model.maxScale);
    const float maxScale = std::max(model.minScale, model.maxScale);
    const float scale = minScale + random.nextFloat() * (maxScale - minScale);
    const float padding = std::clamp(model.boundsPadding, 0.0f, 0.49f);
    float minX = 0.5f - (0.5f - padding) * scale;
    float maxX = 0.5f + (0.5f - padding) * scale;
    float minY = 0.5f - (0.5f - padding) * scale;
    float maxY = 0.5f + (0.5f - padding) * scale;
    float minZ = 0.5f - (0.5f - padding) * scale;
    float maxZ = 0.5f + (0.5f - padding) * scale;
    minX = std::clamp(minX + xOffset, 0.0f, 1.0f);
    maxX = std::clamp(maxX + xOffset, 0.0f, 1.0f);
    minY = std::clamp(minY + yOffset, 0.0f, 1.0f);
    maxY = std::clamp(maxY + yOffset, 0.0f, 1.0f);
    minZ = std::clamp(minZ + zOffset, 0.0f, 1.0f);
    maxZ = std::clamp(maxZ + zOffset, 0.0f, 1.0f);
    return {minX, minY, minZ, maxX, maxY, maxZ};
}

int coordinateColor(int x, int y, int z) {
    const std::int64_t hash = static_cast<std::int64_t>(x) * x * 3187961LL + static_cast<std::int64_t>(x) * 987243LL +
                              static_cast<std::int64_t>(y) * y * 43297126LL + static_cast<std::int64_t>(y) * 987121LL +
                              static_cast<std::int64_t>(z) * z * 927469861LL + static_cast<std::int64_t>(z) * 1861LL;
    return static_cast<int>(hash & 0xFFFFFF);
}

class LuaModBlock : public Block {
   public:
    LuaModBlock(int id, int textureId, Material& material, const LuaBlockModelSpec& modelSpec)
        : Block(id, textureId, material), modelSpec_(modelSpec) {
    }

    [[nodiscard]] bool isOpaque() const override {
        return modelSpec_.opaque;
    }

    [[nodiscard]] bool isFullCube() const override {
        return modelSpec_.fullCube;
    }

    [[nodiscard]] int getColorMultiplier(const BlockView* blockView, int x, int y, int z) const override {
        if (modelSpec_.coordinateColor) {
            return coordinateColor(x, y, z);
        }
        return Block::getColorMultiplier(blockView, x, y, z);
    }

    [[nodiscard]] int getRenderType() const override {
        if (modelSpec_.type == LuaBlockModelSpec::Type::FullCube ||
            modelSpec_.type == LuaBlockModelSpec::Type::BoxList) {
            return net::minecraft::client::render::block::BlockRenderType::FULL_CUBE;
        }
        return 31;  // non-side-lit custom type → item renders as sprite
    }

    [[nodiscard]] int getRenderLayer() const override {
        return modelSpec_.opaque ? 0 : 1;
    }

    [[nodiscard]] Box getRenderBounds(const BlockView* /*blockView*/, int x, int y, int z) const override {
        if (modelSpec_.coordinateBounds) {
            return coordinateVariedBounds(modelSpec_, x, y, z);
        }
        return Block::getRenderBounds(nullptr, x, y, z);
    }

    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override {
        setBoundingBox(getRenderBounds(blockView, x, y, z));
    }

    [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override {
        if (modelSpec_.coordinateBounds) {
            return true;
        }
        return Block::isSideVisible(blockView, x, y, z, side);
    }

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const override {
        if (world != nullptr && modelSpec_.stackOnSame && world->getBlockId(x, y - 1, z) == id) {
            return true;
        }
        if (world != nullptr && !modelSpec_.stackOnSame && world->getBlockId(x, y - 1, z) == id) {
            return false;
        }
        if (modelSpec_.requiresSolidBelow && world != nullptr && !world->getMaterial(x, y - 1, z).isSolid()) {
            return false;
        }
        return Block::canPlaceAt(world, x, y, z);
    }

    [[nodiscard]] std::optional<Box> getCollisionShape(World* /*world*/, int x, int y, int z) const override {
        if (modelSpec_.coordinateBounds) {
            return Box{static_cast<double>(x),
                       static_cast<double>(y),
                       static_cast<double>(z),
                       static_cast<double>(x + 1),
                       static_cast<double>(y + 1),
                       static_cast<double>(z + 1)};
        }
        if (modelSpec_.collisionHeight <= 1.0f) {
            return Block::getCollisionShape(nullptr, x, y, z);
        }
        return Box{static_cast<double>(x),
                   static_cast<double>(y),
                   static_cast<double>(z),
                   static_cast<double>(x + 1),
                   static_cast<double>(y) + modelSpec_.collisionHeight,
                   static_cast<double>(z + 1)};
    }

    void setItemOverrideTexture(int textureId) {
        itemOverrideTextureId_ = textureId;
    }

    void registerBlockItem() override {
        Block::registerBlockItem();
        if (itemOverrideTextureId_ >= 0) {
            Item* item = Item::ITEMS[static_cast<std::size_t>(id)];
            if (item != nullptr) {
                item->setTextureId(itemOverrideTextureId_);
            }
        }
    }

   private:
    LuaBlockModelSpec modelSpec_;
    int itemOverrideTextureId_ = -1;
};

void registerBlockClass(const BlockRegistrationSpec& spec) {
    const int textureId = spec.terrainTextureId >= 0 ? spec.terrainTextureId : texture(spec.texturePath.c_str());
    Material* material = materialFromName(spec.material);
    std::string translationKey = spec.translationKey;
    if (translationKey.empty()) {
        translationKey = "block" + std::to_string(spec.blockId);
    }
    registerModBlockDisplayName(spec);
    Block* block = (new LuaModBlock(spec.blockId, textureId, *material, spec.model))
                       ->setHardness(spec.hardness)
                       ->setResistance(spec.resistance)
                       ->setSoundGroup(&kModBlockSound)
                       ->setTranslationKey(translationKey.c_str());
    if (spec.luminance > 0.0f) {
        block->setLuminance(std::clamp(spec.luminance, 0.0f, 1.0f));
    }
    if (!spec.model.opaque) {
        block->ignoreMetaUpdates();
    }
    if (spec.model.type != LuaBlockModelSpec::Type::FullCube) {
        registerDraw(spec.blockId, drawLuaBlockWorld, drawLuaBlockInventory);
    }
    if (!spec.itemTexturePath.empty()) {
        const int itemTextureId = spec.itemTextureId >= 0 ? spec.itemTextureId : texture(spec.itemTexturePath.c_str());
        static_cast<LuaModBlock*>(block)->setItemOverrideTexture(itemTextureId);
    }
}
}  // namespace detail

void instantiateLuaModBlock(const BlockRegistrationSpec& spec) {
    detail::registerBlockClass(spec);
}

bool invokeManualBlockModelDraw(
    const BlockRegistrationSpec& spec, bool inventory, int x, int y, int z, float brightness) {
    const int ref = inventory && spec.manualInventoryRef != kLuaNoRef ? spec.manualInventoryRef : spec.manualDrawRef;
    if (ref == kLuaNoRef || spec.ownerModId.empty()) {
        return false;
    }
    LuaApi& api = luaApi();
    if (!api.ready()) {
        return false;
    }
    for (const std::shared_ptr<runtime::ModHost::LoadedLuaMod>& mod : runtime::host().loadedMods()) {
        if (mod == nullptr || mod->modId != spec.ownerModId) {
            continue;
        }
        const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
        if (!mod->active || mod->state == nullptr) {
            return false;
        }
        auto* state = static_cast<lua_State*>(mod->state);
        const int top = api.gettop(state);
        api.rawgeti(state, kLuaRegistryIndex, ref);
        if (api.type(state, -1) != kLuaTFunction) {
            api.settop(state, top);
            return false;
        }
        api.createtable(state, 0, 7);
        api.pushinteger(state, x);
        api.setfield(state, -2, "x");
        api.pushinteger(state, y);
        api.setfield(state, -2, "y");
        api.pushinteger(state, z);
        api.setfield(state, -2, "z");
        api.pushboolean(state, inventory ? 1 : 0);
        api.setfield(state, -2, "inventory");
        api.pushnumber(state, brightness);
        api.setfield(state, -2, "brightness");
        api.pushinteger(state, spec.blockId);
        api.setfield(state, -2, "block_id");
        api.pushstring(state, spec.texturePath.c_str());
        api.setfield(state, -2, "texture");
        const int status = api.pcallk(state, 1, 0, 0, 0, nullptr);
        api.settop(state, top);
        return status == kLuaOk;
    }
    return false;
}

__attribute__((weak)) bool drawLuaBlockWorld(
    client::render::block::BlockRenderManager& /*manager*/, Block& /*block*/, int /*x*/, int /*y*/, int /*z*/) {
    return false;
}

__attribute__((weak)) void drawLuaBlockInventory(client::render::block::BlockRenderManager& /*manager*/,
                                                 Block& /*block*/,
                                                 int /*metadata*/,
                                                 float /*brightness*/) {
}
}  // namespace net::minecraft::mod::lua
