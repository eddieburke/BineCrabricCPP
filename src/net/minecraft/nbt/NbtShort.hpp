#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtShort : public NbtElement {
public:
    std::int16_t value = 0;

    NbtShort() = default;
    explicit NbtShort(std::int16_t value) : value(value) {}

    [[nodiscard]] std::uint8_t getType() const override { return 2; }
    void write(std::ostream& output) const override;
    void read(std::istream& input) override;
    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
