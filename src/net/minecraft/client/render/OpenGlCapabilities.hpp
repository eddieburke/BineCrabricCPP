#pragma once

namespace net::minecraft::client::render {

// Faithful port of net.minecraft.client.render.OpenGlCapabilities (beta 1.7.3 MCP).
// Occlusion queries removed (never used; chunk culling via frustum only).
class OpenGlCapabilities {
};

} // namespace net::minecraft::client::render
