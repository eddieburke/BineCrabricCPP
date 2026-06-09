#pragma once

namespace net::minecraft::util {

// Faithful port of net.minecraft.util.Tickable (beta 1.7.3).
class Tickable {
public:
    virtual ~Tickable() = default;

    virtual void tick() = 0;
};

} // namespace net::minecraft::util
