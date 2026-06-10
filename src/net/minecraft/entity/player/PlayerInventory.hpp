#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ArmorItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtList.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace net::minecraft::entity::player {

class PlayerEntity;

class PlayerInventory : public Inventory {
public:
    static constexpr std::size_t mainSize = 36;
    static constexpr std::size_t armorSize = 4;

    explicit PlayerInventory(PlayerEntity* player = nullptr)
        : player_(player)
    {
    }

    [[nodiscard]] PlayerEntity* player() const noexcept
    {
        return player_;
    }

    std::array<ItemStack, mainSize> main {};
    std::array<ItemStack, armorSize> armor {};
    int selectedSlot = 0;
    bool dirty = false;

    [[nodiscard]] ItemStack* getSelectedItem()
    {
        if (selectedSlot < 0 || selectedSlot >= 9) {
            return nullptr;
        }
        ItemStack& stack = main[static_cast<std::size_t>(selectedSlot)];
        return stack.empty() ? nullptr : &stack;
    }

    [[nodiscard]] const ItemStack* getSelectedItem() const
    {
        if (selectedSlot < 0 || selectedSlot >= 9) {
            return nullptr;
        }
        const ItemStack& stack = main[static_cast<std::size_t>(selectedSlot)];
        return stack.empty() ? nullptr : &stack;
    }

    static int getHotbarSize()
    {
        return 9;
    }

    void scrollInHotbar(int direction)
    {
        if (direction > 0) {
            direction = 1;
        }
        if (direction < 0) {
            direction = -1;
        }
        selectedSlot -= direction;
        while (selectedSlot < 0) {
            selectedSlot += 9;
        }
        while (selectedSlot >= 9) {
            selectedSlot -= 9;
        }
    }

    void setHeldItem(int itemId, bool /*testInteractionManager*/)
    {
        const int slot = indexOf(itemId);
        if (slot >= 0 && slot < 9) {
            selectedSlot = slot;
        }
    }

    void inventoryTick();

    bool remove(int itemId)
    {
        const int slot = indexOf(itemId);
        if (slot < 0) {
            return false;
        }
        ItemStack& stack = main[static_cast<std::size_t>(slot)];
        if (--stack.count <= 0) {
            stack = {};
        }
        return true;
    }

    bool addStack(ItemStack& stack)
    {
        if (stack.empty()) {
            return false;
        }
        if (!stack.isDamaged()) {
            int previousCount = 0;
            do {
                previousCount = stack.count;
                stack.count = combineStacks(stack);
            } while (stack.count > 0 && stack.count < previousCount);
            return stack.count < previousCount;
        }
        const int emptySlot = getEmptySlot();
        if (emptySlot >= 0) {
            main[static_cast<std::size_t>(emptySlot)] = stack.copy();
            main[static_cast<std::size_t>(emptySlot)].bobbingAnimationTime = 5;
            stack.count = 0;
            return true;
        }
        return false;
    }

    [[nodiscard]] ItemStack removeStack(std::size_t slot, int amount) override
    {
        ItemStack* stack = nullptr;
        if (slot < main.size()) {
            stack = &main[slot];
        } else if (slot < size()) {
            stack = &armor[slot - main.size()];
        }
        if (stack == nullptr || stack->empty()) {
            return {};
        }
        if (stack->count <= amount) {
            ItemStack removed = *stack;
            *stack = {};
            return removed;
        }
        ItemStack removed = stack->split(amount);
        if (stack->count == 0) {
            *stack = {};
        }
        return removed;
    }

    void setStack(std::size_t slot, ItemStack stack) override
    {
        if (slot < main.size()) {
            main[slot] = stack;
        } else if (slot < size()) {
            armor[slot - main.size()] = stack;
        }
    }

    [[nodiscard]] ItemStack getStack(std::size_t slot) const override
    {
        if (slot < main.size()) {
            return main[slot];
        }
        if (slot < size()) {
            return armor[slot - main.size()];
        }
        return {};
    }

    [[nodiscard]] std::size_t size() const override
    {
        return main.size() + armor.size();
    }

    [[nodiscard]] std::string getName() const override
    {
        return "Inventory";
    }

    [[nodiscard]] int getMaxCountPerStack() const override
    {
        return 64;
    }

    void markDirty() override
    {
        dirty = true;
    }

    [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override;

    [[nodiscard]] float getStrengthOnBlock(int blockId) const
    {
        float strength = 1.0f;
        if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
            return strength;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        const ItemStack stack = getStack(static_cast<std::size_t>(selectedSlot));
        if (!stack.empty() && block != nullptr) {
            strength *= stack.getMiningSpeedMultiplier(block);
        }
        return strength;
    }

    [[nodiscard]] int getAttackDamage(Entity* entity) const
    {
        const ItemStack stack = getStack(static_cast<std::size_t>(selectedSlot));
        if (stack.empty()) {
            return 1;
        }
        return stack.getAttackDamage(entity);
    }

    [[nodiscard]] bool isUsingEffectiveTool(int blockId) const
    {
        if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
            return false;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block == nullptr) {
            return false;
        }
        if (block->material.isHandHarvestable()) {
            return true;
        }
        const ItemStack stack = getStack(static_cast<std::size_t>(selectedSlot));
        if (!stack.empty()) {
            return stack.isSuitableFor(block);
        }
        return false;
    }

    [[nodiscard]] int getTotalArmorDurability() const
    {
        int protection = 0;
        int durabilitySum = 0;
        int maxDurabilitySum = 0;
        for (const ItemStack& piece : armor) {
            if (piece.empty()) {
                continue;
            }
            const auto* armorItem = dynamic_cast<const item::ArmorItem*>(piece.getItem());
            if (armorItem == nullptr) {
                continue;
            }
            const int maxDamage = piece.getMaxDamage();
            const int remaining = maxDamage - piece.getDamage2();
            durabilitySum += remaining;
            maxDurabilitySum += maxDamage;
            protection += armorItem->getMaxProtection();
        }
        if (maxDurabilitySum == 0) {
            return 0;
        }
        return (protection - 1) * durabilitySum / maxDurabilitySum + 1;
    }

    void damageArmor(int amount);

    void dropInventory();

    [[nodiscard]] NbtList writeNbt() const
    {
        NbtList list;
        auto& entries = list.storage().asList();
        for (std::size_t slot = 0; slot < main.size(); ++slot) {
            if (main[slot].empty()) {
                continue;
            }
            NbtCompound entry = main[slot].toNbt();
            entry.putByte("Slot", static_cast<std::int8_t>(slot));
            entries.push_back(entry.storage());
        }
        for (std::size_t slot = 0; slot < armor.size(); ++slot) {
            if (armor[slot].empty()) {
                continue;
            }
            NbtCompound entry = armor[slot].toNbt();
            entry.putByte("Slot", static_cast<std::int8_t>(slot + 100U));
            entries.push_back(entry.storage());
        }
        return list;
    }

    void readNbt(const NbtList& nbt)
    {
        main.fill({});
        armor.fill({});
        for (const Nbt& entryTag : nbt.entries()) {
            if (!entryTag.isCompound()) {
                continue;
            }
            const NbtCompound entry(entryTag);
            const int slot = entry.getByte("Slot") & 0xFF;
            const ItemStack stack = ItemStack::fromNbt(entry);
            if (stack.empty()) {
                continue;
            }
            if (slot >= 0 && slot < static_cast<int>(main.size())) {
                main[static_cast<std::size_t>(slot)] = stack;
            } else if (slot >= 100 && slot < 100 + static_cast<int>(armor.size())) {
                armor[static_cast<std::size_t>(slot - 100)] = stack;
            }
        }
        dirty = false;
    }

    void setCursorStack(ItemStack stack);

    [[nodiscard]] ItemStack getCursorStack() const
    {
        return cursorStack_;
    }

    [[nodiscard]] bool contains(const ItemStack& stack) const
    {
        for (const ItemStack& piece : armor) {
            if (!piece.empty() && piece == stack) {
                return true;
            }
        }
        for (const ItemStack& piece : main) {
            if (!piece.empty() && piece == stack) {
                return true;
            }
        }
        return false;
    }

private:
    [[nodiscard]] int indexOf(int itemId) const
    {
        for (std::size_t i = 0; i < main.size(); ++i) {
            if (!main[i].empty() && main[i].itemId == itemId) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    [[nodiscard]] int indexOf(const ItemStack& stack) const
    {
        for (std::size_t i = 0; i < main.size(); ++i) {
            const ItemStack& slot = main[i];
            if (slot.empty() || slot.itemId != stack.itemId || !slot.isStackable()) {
                continue;
            }
            if (slot.count >= slot.getMaxCount() || slot.count >= getMaxCountPerStack()) {
                continue;
            }
            if (slot.hasSubtypes() && slot.damage != stack.damage) {
                continue;
            }
            return static_cast<int>(i);
        }
        return -1;
    }

    [[nodiscard]] int getEmptySlot() const
    {
        for (std::size_t i = 0; i < main.size(); ++i) {
            if (main[i].empty()) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    int combineStacks(ItemStack stack)
    {
        int slot = indexOf(stack);
        if (slot < 0) {
            slot = getEmptySlot();
        }
        if (slot < 0) {
            return stack.count;
        }
        if (main[static_cast<std::size_t>(slot)].empty()) {
            main[static_cast<std::size_t>(slot)] = ItemStack(stack.itemId, 0, stack.damage);
        }
        int moved = stack.count;
        ItemStack& destination = main[static_cast<std::size_t>(slot)];
        if (moved > destination.getMaxCount() - destination.count) {
            moved = destination.getMaxCount() - destination.count;
        }
        if (moved > getMaxCountPerStack() - destination.count) {
            moved = getMaxCountPerStack() - destination.count;
        }
        if (moved == 0) {
            return stack.count;
        }
        destination.count += moved;
        destination.bobbingAnimationTime = 5;
        return stack.count - moved;
    }

    PlayerEntity* player_ = nullptr;
    ItemStack cursorStack_ {};
};

} // namespace net::minecraft::entity::player
