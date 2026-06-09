#pragma once

#include "net/minecraft/client/render/entity/model/QuadrupedEntityModel.hpp"

namespace net::minecraft::client::render::entity::model {

class PigEntityModel : public QuadrupedEntityModel {
public:
    PigEntityModel() : QuadrupedEntityModel(6, 0.0f)
    {
    }

    explicit PigEntityModel(float dilation) : QuadrupedEntityModel(6, dilation)
    {
    }
};

} // namespace net::minecraft::client::render::entity::model
