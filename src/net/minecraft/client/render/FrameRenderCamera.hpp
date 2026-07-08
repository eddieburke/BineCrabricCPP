#pragma once

namespace net::minecraft::entity {
class LivingEntity;
}

namespace net::minecraft::mod {
struct CameraSetupEvent;
}

namespace net::minecraft::client::render {
struct FrameRenderCamera {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    bool customView = false;
    bool hideFirstPersonHand = false;
};

class RenderCameraState {
   public:
    static RenderCameraState& instance() noexcept;
    void setFrame(FrameRenderCamera camera) noexcept;

    [[nodiscard]] const FrameRenderCamera& frame() const noexcept {
        return frame_;
    }

    void clearFrame() noexcept;

   private:
    FrameRenderCamera frame_{};
};

void populateCameraSetupDefaults(mod::CameraSetupEvent& event,
                                 const net::minecraft::entity::LivingEntity& camera,
                                 float tickDelta,
                                 float rollDegrees);
[[nodiscard]] FrameRenderCamera frameCameraFromSetup(const mod::CameraSetupEvent& event) noexcept;
}  // namespace net::minecraft::client::render
