#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::debug {
/// F3 debug profiler chart and debug info string builders.
/// Anchor: Minecraft.cpp L1280–1351, L1184–1205.
class ClientProfilerOverlay {
 public:
 inline static std::array<std::int64_t, 512> frameTimes{};
 inline static std::array<std::int64_t, 512> tickTimes{};
 inline static int frameTimeIndex = 0;
 static void renderProfilerChart(Minecraft& client, std::int64_t tickTime);
 static void recordFrameTime(Minecraft& client);
 [[nodiscard]] static std::string getRenderChunkDebugInfo(const Minecraft& client);
 [[nodiscard]] static std::string getRenderEntityDebugInfo(const Minecraft& client);
 [[nodiscard]] static std::string getChunkSourceDebugInfo(const Minecraft& client);
 [[nodiscard]] static std::string getWorldDebugInfo(const Minecraft& client);
};
} // namespace net::minecraft::client::debug
