#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtEnd : public NbtElement {
public:
    [[nodiscard]] std::uint8_t getType() const override { return 0; }

    void write(std::ostream& output) const override { (void)output; }
    void read(std::istream& input) override { (void)input; }

    [[nodiscard]] Nbt toStorage() const override { return Nbt(); }
};

} // namespace net::minecraft
