#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtInt : public NbtElement {
public:
    std::int32_t value = 0;

    NbtInt() = default;
    explicit NbtInt(std::int32_t value) : value(value) {}

    [[nodiscard]] std::uint8_t getType() const override { return 3; }
    void write(std::ostream& output) const override;
    void read(std::istream& input) override;
    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
