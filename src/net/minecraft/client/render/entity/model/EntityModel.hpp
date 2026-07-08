#pragma once
#include <cmath>
#include <string>
#include <unordered_map>

#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::client::render::entity::model {
// Per-part rotation override (radians). NaN components mean "leave as-is".
struct ModelPartPose {
    float yaw = std::numeric_limits<float>::quiet_NaN();
    float pitch = std::numeric_limits<float>::quiet_NaN();
    float roll = std::numeric_limits<float>::quiet_NaN();
};

class EntityModel {
   public:
    virtual ~EntityModel() = default;
    float handSwingProgress = 0.0f;
    bool riding = false;
    // Lua entity_render named-part overrides (render-only).
    bool poseActive = false;
    std::unordered_map<std::string, ModelPartPose> partOverrides;

    // Models register their named parts here so Lua can address them by name
    // (e.g. "rightArm", "head", "body"). Called by applyPartOverrides().
    virtual void collectNamedParts(std::unordered_map<std::string, net::minecraft::client::model::ModelPart*>& parts) {
        (void) parts;
    }

    // Applies pose.partOverrides onto the parts returned by collectNamedParts.
    // Call at the end of your setAngles() implementation (radians).
    void applyPartOverrides() {
        if (!poseActive || partOverrides.empty()) {
            return;
        }
        std::unordered_map<std::string, net::minecraft::client::model::ModelPart*> parts;
        collectNamedParts(parts);
        for (const auto& [name, pose] : partOverrides) {
            const auto it = parts.find(name);
            if (it == parts.end() || it->second == nullptr) {
                continue;
            }
            net::minecraft::client::model::ModelPart& part = *it->second;
            if (!std::isnan(pose.yaw)) {
                part.yaw = pose.yaw;
            }
            if (!std::isnan(pose.pitch)) {
                part.pitch = pose.pitch;
            }
            if (!std::isnan(pose.roll)) {
                part.roll = pose.roll;
            }
        }
    }

    virtual void render(
        float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) = 0;

    virtual void setAngles(
        float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        (void) limbAngle;
        (void) limbDistance;
        (void) animationProgress;
        (void) headYaw;
        (void) headPitch;
        (void) scale;
    }

    virtual void animateModel(::net::minecraft::entity::LivingEntity& entity,
                              float limbAngle,
                              float limbDistance,
                              float tickDelta) {
        (void) entity;
        (void) limbAngle;
        (void) limbDistance;
        (void) tickDelta;
    }
};
}  // namespace net::minecraft::client::render::entity::model
