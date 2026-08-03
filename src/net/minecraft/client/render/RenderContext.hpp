#pragma once
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
namespace net::minecraft::client::render {
struct RenderContext {
    const option::RenderSettings& settings;
    util::UiScale uiScale;
    float tickDelta = 0.0f;
};
} // namespace net::minecraft::client::render
