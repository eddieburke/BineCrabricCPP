#pragma once

#include "net/minecraft/client/render/culling/Culler.hpp"

namespace net::minecraft::client::render {

class FrustumData;

class FrustumCuller : public Culler {
public:
    void prepare(double x, double y, double z) override;
    [[nodiscard]] bool isVisible(const net::minecraft::Box& box) const override;

private:
    const FrustumData* frustum_ = nullptr;
    double offsetX_ = 0.0;
    double offsetY_ = 0.0;
    double offsetZ_ = 0.0;
};

} // namespace net::minecraft::client::render
