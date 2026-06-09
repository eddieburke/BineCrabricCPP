#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtLong : public NbtElement {
public:
    std::int64_t value = 0;

    NbtLong() = default;
    explicit NbtLong(std::int64_t value) : value(value) {}

    [[nodiscard]] std::uint8_t getType() const override { return 4; }
    void write(std::ostream& output) const override;
    void read(std::istream& input) override;
    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
