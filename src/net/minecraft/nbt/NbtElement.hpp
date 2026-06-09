#pragma once

#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Nbt.hpp"

#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace net::minecraft {

// Faithful port of net.minecraft.nbt.NbtElement — polymorphic tag base.
class NbtElement {
public:
    virtual ~NbtElement() = default;

    [[nodiscard]] virtual std::uint8_t getType() const = 0;

    [[nodiscard]] std::string getKey() const
    {
        return key_.empty() ? "" : key_;
    }

    NbtElement& setKey(std::string key)
    {
        key_ = std::move(key);
        return *this;
    }

    virtual void write(std::ostream& output) const = 0;
    virtual void read(std::istream& input) = 0;

    [[nodiscard]] static std::unique_ptr<NbtElement> readTag(std::istream& input);
    static void writeTag(const NbtElement& element, std::ostream& output);

    [[nodiscard]] static std::unique_ptr<NbtElement> createTypeFromId(std::uint8_t id);
    [[nodiscard]] static std::string getTypeNameFromId(std::uint8_t id);

    // Bridge to unified storage used by world/chunk/entity save paths.
    [[nodiscard]] virtual Nbt toStorage() const = 0;
    static std::unique_ptr<NbtElement> fromStorage(const Nbt& tag);

protected:
    std::string key_;
};

} // namespace net::minecraft
