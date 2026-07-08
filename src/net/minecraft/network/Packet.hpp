#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/network/packet/PacketTracker.hpp"

namespace net::minecraft {
class NetworkHandler;

class Packet {
   public:
    virtual ~Packet() = default;

    [[nodiscard]] std::uint64_t creationTimeMs() const noexcept {
        return creationTime;
    }

    [[nodiscard]] int rawId() const {
        ensureRegistered();
        const auto& map = typeToId();
        const auto it = map.find(std::type_index(typeid(*this)));
        if (it == map.end()) {
            throw std::runtime_error("Packet type is not registered");
        }
        return it->second;
    }

    template <typename T>
    static void registerPacket(int rawId, bool clientBound, bool serverBound) {
        static_assert(std::is_base_of_v<Packet, T>, "Packet registration requires a Packet subclass");
        auto& ids = typeToId();
        auto& factories = idToFactory();
        const std::type_index type = std::type_index(typeid(T));
        if (factories.contains(rawId)) {
            throw std::runtime_error("Duplicate packet id");
        }
        if (ids.contains(type)) {
            throw std::runtime_error("Duplicate packet type");
        }
        factories.emplace(rawId, []() { return std::make_unique<T>(); });
        ids.emplace(type, rawId);
        if (clientBound) {
            clientBoundIds().insert(rawId);
        }
        if (serverBound) {
            serverBoundIds().insert(rawId);
        }
    }

    [[nodiscard]] static std::unique_ptr<Packet> create(int rawId) {
        ensureRegistered();
        const auto& factories = idToFactory();
        const auto it = factories.find(rawId);
        if (it == factories.end()) {
            return nullptr;
        }
        return it->second();
    }

    [[nodiscard]] static std::unique_ptr<Packet> read(std::istream& input, bool serverSide) {
        ensureRegistered();
        const int rawId = input.get();
        if (rawId == std::char_traits<char>::eof()) {
            return nullptr;
        }
        if ((serverSide && !serverBoundIds().contains(rawId)) || (!serverSide && !clientBoundIds().contains(rawId))) {
            throw std::runtime_error("Bad packet id " + std::to_string(rawId));
        }
        std::unique_ptr<Packet> packet = create(rawId);
        if (packet == nullptr) {
            throw std::runtime_error("Bad packet id " + std::to_string(rawId));
        }
        packet->read(input);
        packetTrackers()[rawId].update(static_cast<int>(packet->size()));
        ++incomingCount();
        return packet;
    }

    static void write(const Packet& packet, std::ostream& output) {
        packetio::writeU8(output, static_cast<std::uint8_t>(packet.rawId()));
        packet.write(output);
    }

    static void writeString(std::string_view value, std::ostream& output) {
        packetio::writeJavaString(output, value);
    }

    [[nodiscard]] static std::string readString(std::istream& input, int maxLength) {
        return packetio::readJavaString(input, static_cast<std::uint16_t>(maxLength));
    }

    const std::uint64_t creationTime = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
    bool worldPacket = false;
    virtual void read(std::istream& input) = 0;
    virtual void write(std::ostream& output) const = 0;
    virtual void apply(NetworkHandler& networkHandler) const = 0;
    [[nodiscard]] virtual std::size_t size() const = 0;
    static void ensureRegistered();

   private:
    using PacketFactory = std::function<std::unique_ptr<Packet>()>;

    [[nodiscard]] static std::unordered_map<int, PacketFactory>& idToFactory() {
        static std::unordered_map<int, PacketFactory> factories;
        return factories;
    }

    [[nodiscard]] static std::unordered_map<std::type_index, int>& typeToId() {
        static std::unordered_map<std::type_index, int> ids;
        return ids;
    }

    [[nodiscard]] static std::unordered_set<int>& clientBoundIds() {
        static std::unordered_set<int> ids;
        return ids;
    }

    [[nodiscard]] static std::unordered_set<int>& serverBoundIds() {
        static std::unordered_set<int> ids;
        return ids;
    }

    [[nodiscard]] static std::unordered_map<int, PacketTracker>& packetTrackers() {
        static std::unordered_map<int, PacketTracker> trackers;
        return trackers;
    }

    [[nodiscard]] static int& incomingCount() {
        static int count = 0;
        return count;
    }
};
}  // namespace net::minecraft
