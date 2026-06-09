#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

#include <memory>
#include <vector>

namespace net::minecraft {

// Faithful port of net.minecraft.nbt.NbtList.
class NbtList : public NbtElement {
public:
    NbtList() : tag_(Nbt::list()) {}

    explicit NbtList(Nbt tag)
        : tag_(std::move(tag))
    {
        if (!tag_.isList()) {
            tag_ = Nbt::list();
        }
    }

    [[nodiscard]] std::uint8_t getType() const override { return 9; }

    void write(std::ostream& output) const override;
    void read(std::istream& input) override;

    [[nodiscard]] Nbt toStorage() const override { return tag_; }

    [[nodiscard]] Nbt& storage() { return tag_; }
    [[nodiscard]] const Nbt& storage() const { return tag_; }

    void add(std::unique_ptr<NbtElement> element);
    void add(Nbt tag) { tag_.asList().push_back(std::move(tag)); }

    [[nodiscard]] std::size_t size() const { return tag_.asList().size(); }
    [[nodiscard]] const Nbt::List& entries() const { return tag_.asList(); }

private:
    Nbt tag_;
};

} // namespace net::minecraft
