#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtString : public NbtElement {
public:
    std::string value;

    NbtString() = default;
    explicit NbtString(std::string value) : value(std::move(value)) {}

    [[nodiscard]] std::uint8_t getType() const override { return 8; }
    void write(std::ostream& output) const override;
    void read(std::istream& input) override;
    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
