#pragma once

#include "net/minecraft/client/render/culling/Culler.hpp"

namespace net::minecraft::client::render {

class FrustumData;

class FrustumCuller : public Culler {
public:
    void prepare(double x, double y, double z) override;
    [[nodiscard]] bool isVisible(const net::minecraft::Box& box) const override;
    [[nodiscard]] const FrustumData* frustumData() const noexcept { return frustum_; }
    [[nodiscard]] double offsetX() const noexcept { return offsetX_; }
    [[nodiscard]] double offsetY() const noexcept { return offsetY_; }
    [[nodiscard]] double offsetZ() const noexcept { return offsetZ_; }

private:
    const FrustumData* frustum_ = nullptr;
    double offsetX_ = 0.0;
    double offsetY_ = 0.0;
    double offsetZ_ = 0.0;
};

} // namespace net::minecraft::client::render
