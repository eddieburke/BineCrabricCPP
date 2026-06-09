#pragma once

#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"

#include <cstddef>
#include <istream>
#include <ostream>
#include <string>

namespace net::minecraft {

class ChatMessagePacket : public Packet {
public:
    std::string chatMessage;

    void read(std::istream& input) override
    {
        chatMessage = Packet::readString(input, 119);
    }

    void write(std::ostream& output) const override
    {
        Packet::writeString(chatMessage, output);
    }

    void apply(NetworkHandler& networkHandler) const override
    {
        networkHandler.onChatMessage(*this);
    }

    [[nodiscard]] std::size_t size() const override
    {
        return chatMessage.size();
    }
};

} // namespace net::minecraft
