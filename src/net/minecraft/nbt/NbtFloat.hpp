#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtFloat : public NbtElement {
public:
    float value = 0.0f;

    NbtFloat() = default;
    explicit NbtFloat(float value) : value(value) {}

    [[nodiscard]] std::uint8_t getType() const override { return 5; }
    void write(std::ostream& output) const override;
    void read(std::istream& input) override;
    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
