#pragma once

#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::client::render::entity::model {

class EntityModel {
public:
    virtual ~EntityModel() = default;

    float handSwingProgress = 0.0f;
    bool riding = false;

    virtual void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale)
    {
        (void)limbAngle;
        (void)limbDistance;
        (void)animationProgress;
        (void)headYaw;
        (void)headPitch;
        (void)scale;
    }

    virtual void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale)
    {
        (void)limbAngle;
        (void)limbDistance;
        (void)animationProgress;
        (void)headYaw;
        (void)headPitch;
        (void)scale;
    }

    virtual void animateModel(::net::minecraft::entity::LivingEntity& entity, float limbAngle, float limbDistance, float tickDelta)
    {
        (void)entity;
        (void)limbAngle;
        (void)limbDistance;
        (void)tickDelta;
    }
};

} // namespace net::minecraft::client::render::entity::model
