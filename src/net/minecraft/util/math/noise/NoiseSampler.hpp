#pragma once

// Faithful port of net.minecraft.util.math.noise.NoiseSampler (beta 1.7.3):
// an empty abstract base class. Concrete samplers keep their own state and do
// not currently rely on virtual dispatch through this base.

namespace net::minecraft {

class NoiseSampler {
public:
    virtual ~NoiseSampler() = default;
};

} // namespace net::minecraft
