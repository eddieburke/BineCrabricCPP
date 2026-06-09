#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtDouble : public NbtElement {
public:
    double value = 0.0;

    NbtDouble() = default;
    explicit NbtDouble(double value) : value(value) {}

    [[nodiscard]] std::uint8_t getType() const override { return 6; }
    void write(std::ostream& output) const override;
    void read(std::istream& input) override;
    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
