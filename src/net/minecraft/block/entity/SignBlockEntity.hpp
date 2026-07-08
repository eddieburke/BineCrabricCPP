#pragma once
#include <array>
#include <string>

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/network/packet/Packets.hpp"

namespace net::minecraft::block::entity {
class SignBlockEntity : public BlockEntity {
   public:
    void writeNbt(NbtCompound& nbt) const override {
        BlockEntity::writeNbt(nbt);
        for (int i = 0; i < 4; ++i) {
            nbt.putString("Text" + std::to_string(i + 1), texts[static_cast<std::size_t>(i)]);
        }
    }

    void readNbt(const NbtCompound& nbt) override {
        editable_ = false;
        BlockEntity::readNbt(nbt);
        for (int i = 0; i < 4; ++i) {
            std::string text = nbt.getString("Text" + std::to_string(i + 1));
            if (text.size() > 15) {
                text = text.substr(0, 15);
            }
            texts[static_cast<std::size_t>(i)] = std::move(text);
        }
    }

    [[nodiscard]] std::string id() const override {
        return "Sign";
    }

    [[nodiscard]] bool isEditable() const noexcept {
        return editable_;
    }

    void setEditable(bool editable) noexcept {
        editable_ = editable;
    }

    [[nodiscard]] std::unique_ptr<Packet> createUpdatePacket() const override {
        auto packet = std::make_unique<UpdateSignPacket>();
        packet->x = x;
        packet->y = y;
        packet->z = z;
        packet->text = texts;
        return packet;
    }

    std::array<std::string, 4> texts{"", "", "", ""};
    int currentRow = -1;

   private:
    bool editable_ = true;
};
}  // namespace net::minecraft::block::entity
