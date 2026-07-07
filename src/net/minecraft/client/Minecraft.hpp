#pragma once
// Faithful port of net.minecraft.client.Minecraft (beta 1.7.3 MCP).
#include "net/minecraft/client/core/ScreenStack.hpp"
#include "net/minecraft/client/core/WorldSession.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerSession.hpp"
#include "net/minecraft/client/InteractionManager.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/hud/InGameHud.hpp"
#include "net/minecraft/client/gui/hud/toast/AchievementToast.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/particle/ParticleManager.hpp"
#include "net/minecraft/client/render/ProgressRenderer.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
#include "net/minecraft/client/render/texture/ClockSprite.hpp"
#include "net/minecraft/client/render/texture/CompassSprite.hpp"
#include "net/minecraft/client/render/texture/FireSprite.hpp"
#include "net/minecraft/client/render/texture/LavaSideSprite.hpp"
#include "net/minecraft/client/render/texture/LavaSprite.hpp"
#include "net/minecraft/client/render/texture/NetherPortalSprite.hpp"
#include "net/minecraft/client/render/texture/WaterSideSprite.hpp"
#include "net/minecraft/client/render/texture/WaterSprite.hpp"
#include "net/minecraft/client/resource/pack/TexturePacks.hpp"
#include "net/minecraft/client/platform/audio/AudioEngine.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/client/util/Timer.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
namespace net::minecraft {
class World;
class WorldStorage;
class WorldStorageSource;
class MinecraftApplet;
} // namespace net::minecraft
namespace net::minecraft::entity::player {
class ClientPlayerEntity;
}
namespace net::minecraft::client::resource {
class ResourceDownloadThread;
}
namespace net::minecraft::client::host {
class LanHostCoordinator;
}
namespace net::minecraft::client::resource::language {
class TranslationStorage;
}
namespace net::minecraft::stat {
class PlayerStats;
}
namespace net::minecraft::util::crash {
class CrashReport;
}
namespace net::minecraft::client::render {
class GameRenderer;
}
namespace net::minecraft::client::sound {
class WorldSoundListener;
}
namespace net::minecraft::client::session {
class SessionValidator;
}
namespace net::minecraft::client::util {
class TimerHackThread;
class DisplayManager;
} // namespace net::minecraft::client::util
namespace net::minecraft::client::lifecycle {
class ClientShutdown;
class ClientInitializer;
class ClientLaunch;
} // namespace net::minecraft::client::lifecycle
namespace net::minecraft::client::input {
class InputSystem;
}
namespace net::minecraft::client {
class Minecraft {
public:
  inline static std::vector<std::byte> MEMORY_RESERVED_FOR_CRASH =
      std::vector<std::byte>(0xA00000);
  inline static Minecraft* INSTANCE = nullptr;
  [[nodiscard]] static std::atomic<std::int64_t>& failedSessionCheckTime() noexcept;
  Minecraft(void* component, void* canvas, net::minecraft::MinecraftApplet* applet, int width, int height, bool fullscreen);
  virtual ~Minecraft();
  void gameCrashed(const net::minecraft::util::crash::CrashReport& crashReport);
  virtual void handleCrash(const net::minecraft::util::crash::CrashReport& crashReport) = 0;
  void setStartupServer(const std::string& address, int port);
  void init();
  [[nodiscard]] static std::filesystem::path getRunDirectory();
  [[nodiscard]] static std::filesystem::path getApplicationDirectory(const std::string& name);
  [[nodiscard]] net::minecraft::WorldStorageSource* getWorldStorageSource();
  void setScreen(std::unique_ptr<gui::screen::Screen> screen);
  void stop();
  void run();
  void cleanHeap();
  void scheduleStop();
  void lockMouse();
  void unlockMouse();
  void pauseGame();
  void toggleFullscreen();
  void resize(int width, int height);
  void scheduleScreenResize();
  void tick();
  void forceResourceReload();
  [[nodiscard]] bool isWorldRemote() const;
  void startGame(const std::string& worldName, const std::string& name, std::int64_t seed,
                 const std::unordered_map<std::string, std::string>& creationOptions = {});
  void changeDimension();
  // General dimension travel: move the player into any dimension by id, handling
  // coordinate scaling, world swap, and portal placement. changeDimension() is the
  // overworld<->Nether toggle built on top of this.
  void travelToDimension(int dimensionId);
  void setWorld(World* world);
  void setWorld(World* world, const std::string& message);
  void setWorld(World* world, const std::string& message, PlayerEntity* player);
  void loadResource(const std::string& path, const std::filesystem::path& file);
  void prepareWorld(const std::string& worldName);
  void convertAndSaveWorld(const std::string& worldName, const std::string& name);
  [[nodiscard]] std::string getRenderChunkDebugInfo() const;
  [[nodiscard]] std::string getRenderEntityDebugInfo() const;
  [[nodiscard]] std::string getChunkSourceDebugInfo() const;
  [[nodiscard]] std::string getWorldDebugInfo() const;
  void respawnPlayer(bool worldSpawn, int dimension);
  static void start(const std::string& username, const std::string& sessionId);
  static void startAndConnect(const std::string& username, const std::string& sessionId, const std::string* server);
  static int main(int argc, char** argv);
  [[nodiscard]] static bool isDisplayGui();
  [[nodiscard]] static bool isFancyGraphicsEnabled();
  [[nodiscard]] static bool isAmbientOcclusionEnabled();
  [[nodiscard]] static bool isDebugProfilerEnabled();
  [[nodiscard]] bool isCommand(const std::string& message) const;
  [[nodiscard]] gui::screen::Screen* currentScreen() { return screenStack_.current(); }
  [[nodiscard]] const gui::screen::Screen* currentScreen() const { return screenStack_.current(); }
  [[nodiscard]] core::WorldSession& worldSession() noexcept { return worldSession_; }
  [[nodiscard]] const core::WorldSession& worldSession() const noexcept { return worldSession_; }
  [[nodiscard]] multiplayer::MultiplayerSession& multiplayerSession() noexcept { return multiplayerSession_; }
  [[nodiscard]] const multiplayer::MultiplayerSession& multiplayerSession() const noexcept { return multiplayerSession_; }
  [[nodiscard]] host::LanHostCoordinator& lanHostCoordinator() noexcept { return *lanHostCoordinator_; }
  [[nodiscard]] const host::LanHostCoordinator& lanHostCoordinator() const noexcept { return *lanHostCoordinator_; }
  // Java public fields.
  std::unique_ptr<InteractionManager> interactionManager;
  bool fullscreen = false;
  bool crashed = false;
  int displayWidth = 0;
  int displayHeight = 0;
  util::Timer timer{20.0f};
  World* world = nullptr;
  std::unique_ptr<render::WorldRenderer> worldRenderer;
  std::unique_ptr<sound::WorldSoundListener> worldSoundListener;
  entity::player::ClientPlayerEntity* player = nullptr;
  LivingEntity* camera = nullptr;
  particle::ParticleManager particleManager{};
  util::Session session{};
  std::string hostAddress;
  void* canvas = nullptr;
  bool isApplet = true;
  std::atomic<bool> paused{false};
  option::GameOptions options{};
  texture::TextureManager textureManager{&options};
  std::unique_ptr<font::TextRenderer> textRenderer;
  render::ProgressRenderer progressRenderer{this};
  std::unique_ptr<render::GameRenderer> gameRenderer;
  int ticksPlayed = 0;
  gui::hud::toast::AchievementToast toast{};
  gui::hud::InGameHud inGameHud{};
  bool skipGameRender = false;
  render::entity::model::BipedEntityModel bipedModel{0.0f};
  std::optional<HitResult> crosshairTarget;
  net::minecraft::MinecraftApplet* applet = nullptr;
  platform::audio::AudioEngine audio{};
  resource::pack::TexturePacks* texturePacks = nullptr;
  std::string debugText;
  bool screenshotKeyDown = false;
  std::int64_t timeAfterLastTick = -1;
  std::atomic<bool> focused{false};
  int lastClickTicks = 0;
  bool raining = false;
  std::int64_t lastTickTime = 0;
  stat::PlayerStats* stats = nullptr;
  std::atomic<bool> running{true};
  core::ScreenStack screenStack_{*this};
  core::WorldSession worldSession_;
  multiplayer::MultiplayerSession multiplayerSession_;

private:
  void handleScreenshotKey();
  void handleMouseDown(int button, bool holdingAttack);
  void handleMouseClick(int button);
  void handlePickBlock();
  void notifyWorldChanged(World* world);
  void startSessionCheck();
  void runRenderPhase(std::int64_t tickDuration, int& frames, std::int64_t& fpsWindowStart);
  void runWorldSimulation();
  friend class core::WorldSession;
  friend class core::ScreenStack;
  friend class input::InputSystem;
  friend class lifecycle::ClientShutdown;
  friend class lifecycle::ClientInitializer;
  friend class util::DisplayManager;
  int attackCooldown = 0;
  int initWidth = 0;
  int initHeight = 0;
  bool pendingScreenResize_ = false;
  std::unique_ptr<resource::ResourceDownloadThread> resourceDownloadThread;
  std::filesystem::path runDirectory_;
  std::unique_ptr<net::minecraft::WorldStorageSource> worldStorageSource;
  std::unique_ptr<resource::pack::TexturePacks> texturePacksStorage_;
  std::unique_ptr<resource::language::TranslationStorage> translationStorage_;
  std::unique_ptr<stat::PlayerStats> statsStorage_;
  std::unique_ptr<host::LanHostCoordinator> lanHostCoordinator_;
  std::string startupServerAddress_;
  int startupServerPort = 0;
  render::texture::WaterSprite waterSprite_{};
  render::texture::LavaSprite lavaSprite_{};
  std::unique_ptr<util::TimerHackThread> timerHackThread_;
};
} // namespace net::minecraft::client
