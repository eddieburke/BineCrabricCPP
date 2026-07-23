#include "net/minecraft/client/Minecraft.hpp"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/MinecraftApplet.hpp"
#include "net/minecraft/client/Screenshot.hpp"
#include "net/minecraft/client/SingleplayerInteractionManager.hpp"
#include "net/minecraft/client/auth/microsoft/PlayerTextures.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/color/world/FoliageColors.hpp"
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/client/color/world/WaterColors.hpp"
#include "net/minecraft/client/debug/ClientProfilerOverlay.hpp"
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gui/screen/ConfirmScreen.hpp"
#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"
#include "net/minecraft/client/gui/screen/DeathScreen.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/gui/screen/FatalErrorScreen.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/gui/screen/OutOfMemoryScreen.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/gui/screen/world/WorldSaveConflictScreen.hpp"
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/ProgressRenderer.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include "net/minecraft/client/resource/ResourcePack.hpp"
#include "net/minecraft/client/resource/ResourceRoot.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/resource/language/TranslationStorage.hpp"
#include "net/minecraft/client/session/SessionValidator.hpp"
#include "net/minecraft/client/sound/WorldSoundListener.hpp"
#include "net/minecraft/client/util/DisplayManager.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/crash/CrashReport.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/dimension/PortalForcer.hpp"
#include "net/minecraft/world/storage/RegionWorldStorageSource.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"
#include "net/minecraft/world/storage/exception/SessionLockException.hpp"
namespace net::minecraft::client {
namespace {
std::int64_t currentTimeMillis() {
 return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
     .count();
}
std::int64_t nanoTime() {
 return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
     .count();
}
std::string crashReportTimestamp() {
 const std::time_t seconds = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
 std::tm localTime{};
#if defined(_WIN32)
 localtime_s(&localTime, &seconds);
#else
 localtime_r(&seconds, &localTime);
#endif
 char buffer[32] = {};
 std::strftime(buffer, sizeof(buffer), "%m/%d/%y %I:%M %p", &localTime);
 return buffer;
}
std::string crashReportGlString(unsigned int name) {
 const unsigned char* value = ::glGetString(name);
 return value != nullptr ? reinterpret_cast<const char*>(value) : "";
}
std::string crashReportToLower(std::string value) {
 for(char& ch : value) {
  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
 }
 return value;
}
std::string crashReportStackTrace(const std::exception_ptr& exception) {
 if(!exception) {
  return "(no exception)\n";
 }
 try {
  std::rethrow_exception(exception);
 } catch(const std::exception& ex) {
  return std::string(ex.what()) + "\n";
 } catch(...) {
  return "Unknown exception\n";
 }
}
std::string formatCrashReport(const net::minecraft::util::crash::CrashReport& report) {
 const std::string stackTrace = crashReportStackTrace(report.exception);
 const std::string glVendor = crashReportGlString(0x1F00);
 const std::string glRenderer = crashReportGlString(0x1F01);
 const std::string glVersion = crashReportGlString(0x1F02);
 std::string systemSection;
 systemSection += "Generated " + crashReportTimestamp() + "\n\n";
 systemSection += "Minecraft: Minecraft Beta 1.7.3\n";
#if defined(_WIN32)
 systemSection += "OS: Windows\n";
#else
 systemSection += "OS: Unknown\n";
#endif
 systemSection += "OpenGL: " + (glRenderer.empty() ? std::string("(unavailable)") : glRenderer) + " version " +
                  glVersion + ", " + glVendor + "\n";
 systemSection += "\n" + stackTrace;
 std::string body;
 if(stackTrace.find("Pixel format not accelerated") != std::string::npos) {
  body += "      Bad video card drivers!      \n";
  body += "      -----------------------      \n\n";
  body += "Minecraft was unable to start because it failed to find an accelerated OpenGL mode.\n";
  body += "This can usually be fixed by updating the video card drivers.\n";
  const std::string lowerVendor = crashReportToLower(glVendor);
  if(lowerVendor.find("nvidia") != std::string::npos) {
   body += "\nYou might be able to find drivers for your video card here:\n  http://www.nvidia.com/\n";
  } else if(lowerVendor.find("ati") != std::string::npos) {
   body += "\nYou might be able to find drivers for your video card here:\n  http://www.amd.com/\n";
  }
 } else {
  body += "      Minecraft has crashed!      \n";
  body += "      ----------------------      \n\n";
  body += "Minecraft has stopped running because it encountered a problem.\n\n";
  body +=
      "If you wish to report this, please include this entire text and a description of what you did "
      "when the error occured.\n";
 }
 body += "\n\n\n";
 std::ostringstream beginHex;
 beginHex << std::hex << std::hash<std::string>{}(body);
 body += "--- BEGIN ERROR REPORT " + beginHex.str() + " --------\n";
 body += report.description + "\n\n" + systemSection;
 std::ostringstream endHex;
 endHex << std::hex << std::hash<std::string>{}(body);
 body += "--- END ERROR REPORT " + endHex.str() + " ----------\n";
 return body;
}
void renderBootstrapLoadingScreen(Minecraft& client) {
 const util::UiScale scale = util::uiScale(client.options, client.displayWidth, client.displayHeight);
 render::RenderSystem::clear(0x00004000 | 0x00000100);
 render::RenderSystem::matrixMode(0x1701);
 render::RenderSystem::loadIdentity();
 render::RenderSystem::ortho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 1000.0, 3000.0);
 render::RenderSystem::matrixMode(0x1700);
 render::RenderSystem::loadIdentity();
 render::RenderSystem::translate(0.0f, 0.0f, -2000.0f);
 render::RenderSystem::viewport(0, 0, client.displayWidth, client.displayHeight);
 render::RenderSystem::clearColor(0.0f, 0.0f, 0.0f, 0.0f);
 {
  render::RenderPassScope scope(render::RenderType::guiTextured());
  render::RenderSystem::bindTexture(client.textureManager.getTextureId("/title/mojang.png"));
  render::RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  render::Tessellator& tessellator = render::Tessellator::INSTANCE;
  tessellator.startQuads();
  tessellator.color(0xFFFFFF);
  tessellator.vertex(0.0, scale.rawHeight, 0.0, 0.0, 0.0);
  tessellator.vertex(scale.rawWidth, scale.rawHeight, 0.0, 0.0, 0.0);
  tessellator.vertex(scale.rawWidth, 0.0, 0.0, 0.0, 0.0);
  tessellator.vertex(0.0, 0.0, 0.0, 0.0, 0.0);
  constexpr int logoW = 256;
  constexpr int logoH = 256;
  gui::draw::appendAtlasQuad(
      tessellator, (scale.scaledWidth - logoW) / 2, (scale.scaledHeight - logoH) / 2, 0, 0, logoW, logoH, 0.0f);
  tessellator.draw();
 }
 render::RenderSystem::alphaTest(0.1f);
#ifdef _WIN32
 util::DisplayManager::present();
#endif
}
class RunnableMinecraft final : public Minecraft {
 public:
 RunnableMinecraft(
     void* component, void* canvas, net::minecraft::MinecraftApplet* applet, int width, int height, bool fullscreen)
     : Minecraft(component, canvas, applet, width, height, fullscreen) {
 }
 void handleCrash(const net::minecraft::util::crash::CrashReport& crashReport) override {
  const std::string reportText = formatCrashReport(crashReport);
  ClientLog::LOGGER.log(LogLevel::Severe, reportText);
#ifdef _WIN32
  diagnostics::reportFatalError("Minecraft has crashed!", reportText);
  diagnostics::pauseBeforeExit();
#endif
 }
};
} // namespace
Minecraft::Minecraft(void* component,
                     void* canvasIn,
                     net::minecraft::MinecraftApplet* appletIn,
                     int width,
                     int height,
                     bool fullscreenIn)
    : fullscreen(fullscreenIn),
      displayWidth(width),
      displayHeight(height),
      canvas(canvasIn),
      applet(appletIn),
      initWidth(width),
      initHeight(height) {
 (void)component;
 initializeBlocks();
 if(applet == nullptr) {
  isApplet = false;
 }
 INSTANCE = this;
 serverProcessCoordinator_ = std::make_unique<host::ServerProcessCoordinator>(this);
}
Minecraft::~Minecraft() {
 if(serverProcessCoordinator_ != nullptr) {
  serverProcessCoordinator_->shutdown();
 }
}
void Minecraft::gameCrashed(const net::minecraft::util::crash::CrashReport& crashReport) {
 crashed = true;
 handleCrash(crashReport);
}
void Minecraft::setStartupServer(const std::string& address, int port) {
 startupServerAddress_ = address;
 startupServerPort = port;
}
void Minecraft::init() {
 initializeBlocks();
 util::DisplayManager::setupAndCreateDisplay(*this);
 bootstrapAfterDisplay();
}
void Minecraft::bootstrapAfterDisplay() {
 diagnostics::setStartupPhase("init: directories");
 runDirectory_ = getRunDirectory();
 options.optionsFile = runDirectory_ / "options.txt";
 options.bindMinecraft(this);
 option::OptionRegistry::registerAll();
 options.load();
 worldStorageSource = std::make_unique<net::minecraft::RegionWorldStorageSource>(runDirectory_ / "saves");
 const net::minecraft::ResourcePack resources(resource::resourceRoot());
 translationStorage_ = std::make_unique<resource::language::TranslationStorage>(resources);
 resource::language::I18n::setTranslations(translationStorage_.get());
 texturePacksStorage_ = std::make_unique<resource::pack::TexturePacks>(
     resource::resourceRoot(), runDirectory_, &options, &textureManager);
 texturePacks = texturePacksStorage_.get();
 textureManager.setTexturePacks(texturePacks);
 diagnostics::setStartupPhase("init: text renderer");
 textRenderer = font::TextRenderer::create(options, textureManager, "font/default.png");
 color::world::WaterColors::setColorMap(textureManager.getColors("/misc/watercolor.png"));
 color::world::GrassColors::setColorMap(textureManager.getColors("/misc/grasscolor.png"));
 color::world::FoliageColors::setColorMap(textureManager.getColors("/misc/foliagecolor.png"));
 diagnostics::setStartupPhase("init: game renderer");
 gameRenderer = std::make_unique<render::GameRenderer>(this);
 render::entity::EntityRenderDispatcher::instance().setHeldItemRenderer(
     std::make_unique<render::item::HeldItemRenderer>(this));
 diagnostics::setStartupPhase("init: player stats");
 statsStorage_ = std::make_unique<stat::PlayerStats>(session, runDirectory_);
 stats = statsStorage_.get();
 diagnostics::setStartupPhase("init: loading screen");
 util::DisplayManager::logGlError(*this, "Pre startup");
 render::RenderSystem::clearDepth(1.0);
 render::RenderSystem::matrixMode(0x1701);
 render::RenderSystem::loadIdentity();
 render::RenderSystem::matrixMode(0x1700);
 util::DisplayManager::logGlError(*this, "Startup");
 renderBootstrapLoadingScreen(*this);
#ifdef _WIN32
 input::InputSystem::init(util::DisplayManager::hwnd());
#endif
 util::DisplayManager::logGlError(*this, "Post startup");
 audio.start(&options);
 textureManager.addDynamicTexture(&lavaSprite_);
 textureManager.addDynamicTexture(&waterSprite_);
 textureManager.addDynamicTexture(new render::texture::NetherPortalSprite());
 textureManager.addDynamicTexture(new render::texture::CompassSprite(*this));
 textureManager.addDynamicTexture(new render::texture::ClockSprite(*this));
 textureManager.addDynamicTexture(new render::texture::WaterSideSprite());
 textureManager.addDynamicTexture(new render::texture::LavaSideSprite());
 textureManager.addDynamicTexture(new render::texture::FireSprite(0));
 textureManager.addDynamicTexture(new render::texture::FireSprite(1));
 particleManager.setTextureManager(&textureManager);
 worldRenderer = std::make_unique<render::WorldRenderer>(this, &textureManager);
 worldSoundListener = std::make_unique<sound::WorldSoundListener>(this);
 render::RenderSystem::viewport(0, 0, displayWidth, displayHeight);
 resourceDownloadThread = std::make_unique<resource::ResourceDownloadThread>(resource::resourceRoot(), this);
 resourceDownloadThread->start();
 interactionManager = std::make_unique<SingleplayerInteractionManager>(this);
 inGameHud.setClient(this);
 toast.setClient(this);
 diagnostics::setStartupPhase("init: title screen");
 msauth::beginRestoreSavedAccount(*this);
 if(!startupServerAddress_.empty()) {
  setScreen(std::make_unique<gui::screen::ConnectScreen>(this, startupServerAddress_, startupServerPort));
 } else {
  setScreen(std::make_unique<gui::screen::TitleScreen>());
 }
 diagnostics::setStartupPhase("init: complete");
}
std::atomic<std::int64_t>& Minecraft::failedSessionCheckTime() noexcept {
 return session::SessionValidator::failedSessionCheckTime;
}
std::filesystem::path Minecraft::getRunDirectory() {
 return util::MinecraftDirectories::getRunDirectory();
}
std::filesystem::path Minecraft::getApplicationDirectory(const std::string& name) {
 return util::MinecraftDirectories::getApplicationDirectory(name);
}
net::minecraft::WorldStorageSource* Minecraft::getWorldStorageSource() {
 return worldStorageSource.get();
}
void Minecraft::setScreen(std::unique_ptr<gui::screen::Screen> screen) {
 screenStack_.setScreen(std::move(screen));
}
void Minecraft::stop() {
#ifdef _WIN32
 diagnostics::disarmHangWatchdog();
#endif
 try {
  if(applet != nullptr) {
   applet->clearMemory();
  }
  if(resourceDownloadThread != nullptr) {
   try {
    resourceDownloadThread->cancel();
   } catch(...) {
    ClientLog::LOGGER.log(LogLevel::Warning, "resourceDownloadThread->cancel() failed during shutdown");
   }
  }
  try {
   setWorld(nullptr);
  } catch(const std::exception& e) {
   ClientLog::LOGGER.log(LogLevel::Warning, "World teardown failed", &e);
  }
  try {
   RegionIo::flush();
  } catch(...) {
   ClientLog::LOGGER.log(LogLevel::Warning, "RegionIo::flush() failed during shutdown");
  }
  try {
   render::RenderSystem::clearAllocatedTextures();
  } catch(...) {
   ClientLog::LOGGER.log(LogLevel::Warning, "clearAllocatedTextures() failed during shutdown");
  }
  if(serverProcessCoordinator_ != nullptr) {
   serverProcessCoordinator_->shutdown();
  }
  mod::runtime::host().shutdown();
  audio.shutdown();
#ifdef _WIN32
  input::InputSystem::shutdown();
#endif
 } catch(...) {
 }
#ifdef _WIN32
 util::DisplayManager::destroy();
#endif
 if(!crashed) {
  std::cout.flush();
  std::_Exit(0);
 }
}
void Minecraft::cleanHeap() {
 try {
  MEMORY_RESERVED_FOR_CRASH.clear();
  if(worldRenderer != nullptr) {
   worldRenderer->releaseSections();
  }
 } catch(...) {
  ClientLog::LOGGER.log(LogLevel::Warning, "cleanHeap failed during crash recovery");
 }
 try {
  setWorld(nullptr);
 } catch(...) {
  ClientLog::LOGGER.log(LogLevel::Warning, "setWorld(nullptr) failed during crash recovery");
 }
}
void Minecraft::handleScreenshotKey() {
 if(input::InputSystem::instance().isKeyDown(input::keys::kF2)) {
  if(!screenshotKeyDown) {
   screenshotKeyDown = true;
   inGameHud.addChatMessage(Screenshot::take(getRunDirectory(), displayWidth, displayHeight));
  }
 } else {
  screenshotKeyDown = false;
 }
}
void Minecraft::scheduleStop() {
 running = false;
}
void Minecraft::lockMouse() {
 if(!input::InputSystem::instance().acceptsInput()) {
  return;
 }
 if(focused.load()) {
  return;
 }
 focused = true;
 input::InputSystem::instance().lockCursor();
 screenStack_.setScreen(nullptr);
 attackCooldown = 10000;
 lastClickTicks = ticksPlayed + 10000;
}
void Minecraft::unlockMouse() {
 if(!focused.load()) {
  return;
 }
 input::InputSystem::instance().resetBindings();
 if(interactionManager != nullptr) {
  interactionManager->cancelBlockBreaking();
 }
 focused = false;
 input::InputSystem::instance().unlockCursor();
}
void Minecraft::pauseGame() {
 if(screenStack_.hasScreen()) {
  return;
 }
 setScreen(std::make_unique<gui::screen::GameMenuScreen>());
}
void Minecraft::handleMouseDown(int button, bool holdingAttack) {
 if(interactionManager == nullptr || interactionManager->noTick) {
  return;
 }
 if(!holdingAttack) {
  attackCooldown = 0;
 }
 if(button == 0 && attackCooldown > 0) {
  return;
 }
 if(holdingAttack && crosshairTarget.has_value() && crosshairTarget->type == HitResultType::BLOCK && button == 0) {
  interactionManager->processBlockBreakingAction(
      crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ, crosshairTarget->side);
  particleManager.addBlockBreakingParticles(
      crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ, crosshairTarget->side);
 } else {
  interactionManager->cancelBlockBreaking();
 }
}
void Minecraft::handleMouseClick(int button) {
 if(player == nullptr || interactionManager == nullptr) {
  return;
 }
 if(button == 0 && attackCooldown > 0) {
  return;
 }
 if(button == 0) {
  player->swingHand();
 }
 bool useSelectedItem = true;
 if(!crosshairTarget.has_value()) {
  if(button == 0) {
   attackCooldown = 10;
  }
 } else if(crosshairTarget->type == HitResultType::ENTITY) {
  if(button == 0) {
   interactionManager->attackEntity(player, crosshairTarget->entity);
  }
  if(button == 1) {
   interactionManager->interactEntity(player, crosshairTarget->entity);
  }
 } else if(crosshairTarget->type == HitResultType::BLOCK) {
  if(button == 0) {
   interactionManager->attackBlock(
       crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ, crosshairTarget->side);
  } else {
   ItemStack* itemStack = player->inventory.getSelectedItem();
   const int previousCount = itemStack != nullptr ? itemStack->count : 0;
   if(interactionManager->interactBlock(player,
                                        world,
                                        itemStack,
                                        crosshairTarget->blockX,
                                        crosshairTarget->blockY,
                                        crosshairTarget->blockZ,
                                        crosshairTarget->side)) {
    useSelectedItem = false;
    player->swingHand();
   }
   if(itemStack == nullptr) {
    return;
   }
   if(itemStack->count == 0) {
    player->inventory.main[player->inventory.selectedSlot] = ItemStack{};
   } else if(itemStack->count != previousCount && gameRenderer != nullptr &&
             gameRenderer->heldItemRendererPtr() != nullptr) {
    gameRenderer->heldItemRendererPtr()->place();
   }
  }
 }
 ItemStack* selected = player->inventory.getSelectedItem();
 if(useSelectedItem && button == 1 && selected != nullptr &&
    interactionManager->interactItem(player, world, selected)) {
  if(gameRenderer != nullptr && gameRenderer->heldItemRendererPtr() != nullptr) {
   gameRenderer->heldItemRendererPtr()->use();
  }
 }
}
void Minecraft::toggleFullscreen() {
 util::DisplayManager::toggleFullscreen(*this);
}
void Minecraft::scheduleScreenResize() {
 util::DisplayManager::scheduleScreenResize(*this);
}
void Minecraft::resize(int width, int height) {
 util::DisplayManager::resize(*this, width, height);
}
void Minecraft::handlePickBlock() {
 if(!crosshairTarget.has_value() || world == nullptr || player == nullptr) {
  return;
 }
 int blockId = world->getBlockId(crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ);
 if(blockId == 2) {
  blockId = 3;
 }
 if(blockId == 43) {
  blockId = 44;
 }
 if(blockId == 7) {
  blockId = 1;
 }
 player->inventory.setHeldItem(blockId);
}
void Minecraft::startSessionCheck() {
 session::SessionValidator::startSessionCheck(*this);
}
void Minecraft::runWorldSimulation() {
 if(world == nullptr) {
  return;
 }
 // Apply any respawn the network handler deferred out of ClientWorld::tick(); doing
 // it here keeps the heavy prepareWorld()/lighting flush off the world-tick stack.
 if(auto* clientWorld = dynamic_cast<ClientWorld*>(world)) {
  if(multiplayer::ClientNetworkHandler* handler = clientWorld->networkHandler()) {
   handler->applyDeferredRespawn();
  }
 }
 if(player != nullptr) {
  worldSession_.tickJoinPlayerCounter(*this);
 }
 world->difficulty = options.difficulty;
 if(world->isRemote()) {
  world->difficulty = 3;
 }
 if(!paused.load()) {
  if(gameRenderer != nullptr) {
   gameRenderer->updateCamera();
  }
  if(world->weather().lightningTicks > 0) {
   --world->weather().lightningTicks;
  }
  world->tickEntities();
 }
 if(!paused.load() || isWorldRemote()) {
  world->allowSpawning(options.difficulty > 0, true);
  world->tick();
 }
 if(!paused.load() && player != nullptr) {
  world->displayTick(MathHelper::floor(player->x), MathHelper::floor(player->y), MathHelper::floor(player->z));
 }
 if(!paused.load()) {
  particleManager.removeDeadParticles();
 }
}
void Minecraft::tick() {
 if(serverProcessCoordinator_ != nullptr) {
  serverProcessCoordinator_->tick();
 }
 mod::ClientTickEvent beforeClientTick{this, player, world, paused.load(), true, false};
 net::minecraft::mod::runtime::luaHookClientTick(beforeClientTick);
 msauth::tickRestoreSavedAccount(*this);
 audio.tick();
 if(worldSoundListener != nullptr && world != nullptr) {
  worldSoundListener->tickWeather(*this);
 }
 if(ticksPlayed == 6000) {
  startSessionCheck();
 }
 if(stats != nullptr) {
  stats->tick();
 }
 inGameHud.tick();
 if(gameRenderer != nullptr) {
  gameRenderer->updateTargetedEntity(1.0f);
 }
 {
  mod::RaycastEvent raycastEvent{this, player, world};
  if(crosshairTarget.has_value()) {
   const HitResult& target = *crosshairTarget;
   raycastEvent.hasHit = true;
   raycastEvent.type = target.type;
   raycastEvent.hitX = target.pos.x;
   raycastEvent.hitY = target.pos.y;
   raycastEvent.hitZ = target.pos.z;
   if(target.type == HitResultType::BLOCK) {
    raycastEvent.blockX = target.blockX;
    raycastEvent.blockY = target.blockY;
    raycastEvent.blockZ = target.blockZ;
    raycastEvent.side = target.side;
    if(world != nullptr) {
     raycastEvent.blockId = world->getBlockId(target.blockX, target.blockY, target.blockZ);
    }
   } else if(target.type == HitResultType::ENTITY) {
    raycastEvent.entity = target.entity;
   }
  }
  net::minecraft::mod::runtime::luaHookRaycast(raycastEvent);
 }
 if(!paused.load() && world != nullptr && interactionManager != nullptr) {
  interactionManager->tick();
  if(worldRenderer != nullptr) {
   worldRenderer->miningProgress = interactionManager->getBlockBreakingProgress(timer.partialTick);
  }
 }
 render::RenderSystem::bindTexture(textureManager.getTextureId("/terrain.png"));
 if(!paused.load()) {
  textureManager.tick();
 }
 // INVARIANT: Tick order — do not reorder.
 // InputSystem::beginFrame → screenStack.tickScreens (Screen::tickInput) → pollGame.
 // runWorldSimulation runs after input is settled.
 input::InputSystem::instance().beginFrame(*this);
 screenStack_.tickScreens(*this);
 // Pump the multiplayer socket while no ClientWorld is ticking it (ConnectScreen /
 // pre-login). Runs after screen tick so ConnectScreen::poll can adopt a pending bridge
 // before the first pump. Once a remote world is active, ClientWorld::tick owns it.
 if(world == nullptr || !world->isRemote()) {
  multiplayerSession_.tick();
 }
 if(pendingScreenResize_) {
  pendingScreenResize_ = false;
  resize(displayWidth, displayHeight);
 }
 input::InputSystem::instance().pollGame(*this);
 mod::ClientTickEvent preSimClientTick{this, player, world, paused.load(), false, false};
 net::minecraft::mod::runtime::luaHookClientTick(preSimClientTick);
 runWorldSimulation();
 lastTickTime = currentTimeMillis();
 mod::ClientTickEvent afterClientTick{this, player, world, paused.load(), false, true};
 net::minecraft::mod::runtime::luaHookClientTick(afterClientTick);
}
void Minecraft::runRenderPhase(std::int64_t tickDuration, int& frames, std::int64_t& fpsWindowStart) {
 audio.updateListener(player, timer.partialTick);
 if(world != nullptr) {
  world->doLightingUpdates();
  world->pumpChunkPublish();
 }
 // Key 65 (F7) — defer present until after render when held.
 if(!input::InputSystem::instance().isKeyDown(input::keys::kF7)) {
  util::DisplayManager::pumpAndPresent();
 }
 if(player != nullptr && player->isInsideWall()) {
  options.thirdPerson = false;
 }
 if(!skipGameRender) {
  mod::RenderFrameEvent rfEvent{timer.partialTick};
  net::minecraft::mod::runtime::luaHookRenderFrame(rfEvent);
  if(interactionManager != nullptr) {
   interactionManager->update(timer.partialTick);
  }
  if(gameRenderer != nullptr) {
   gameRenderer->onFrameUpdate(timer.partialTick);
  }
 }
#ifdef _WIN32
 if(!util::DisplayManager::isActive()) {
  if(fullscreen) {
   toggleFullscreen();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
#endif
 if(options.debugHud) {
  debug::ClientProfilerOverlay::renderProfilerChart(*this, tickDuration);
 } else {
  debug::ClientProfilerOverlay::recordFrameTime(*this);
 }
 toast.tick();
 std::this_thread::yield();
 if(input::InputSystem::instance().isKeyDown(input::keys::kF7)) {
  util::DisplayManager::pumpAndPresent();
 }
 handleScreenshotKey();
 ++frames;
 paused = !isWorldRemote() && currentScreen() != nullptr && currentScreen()->shouldPause();
 while(currentTimeMillis() >= fpsWindowStart + 1000) {
  debugText = std::to_string(frames) + " fps, " + std::to_string(render::chunk::ChunkBuilder::chunkUpdates) +
              " chunk updates";
  render::chunk::ChunkBuilder::chunkUpdates = 0;
  fpsWindowStart += 1000;
  frames = 0;
 }
}
void Minecraft::run() {
 running = true;
 try {
  init();
 } catch(const std::exception& exception) {
  ClientLog::LOGGER.log(LogLevel::Severe, "Failed to start game", &exception);
  gameCrashed(net::minecraft::util::crash::CrashReport("Failed to start game", exception.what()));
  return;
 }
 try {
  std::int64_t fpsWindowStart = currentTimeMillis();
  int frames = 0;
  while(running.load()) {
   try {
#ifdef _WIN32
    diagnostics::pingMainLoopHeartbeat();
#endif
    screenStack_.flushRetired();
    // Free network bridges retired last iteration, then retire the active one once it
    // has disconnected. Both happen here (no tick on the stack) so a handler whose
    // tick() requested teardown is destroyed only after that stack has unwound.
    multiplayerSession_.flushRetired();
    if(multiplayer::ClientNetworkBridge* bridge = multiplayerSession_.bridge()) {
     multiplayer::ClientNetworkHandler* handler = bridge->handler();
     if(handler == nullptr || handler->disconnected) {
      multiplayerSession_.retireBridge();
     }
    }
    if(applet != nullptr && !applet->isActive()) {
     break;
    }
#ifdef _WIN32
    if(canvas == nullptr && util::DisplayManager::isCloseRequested()) {
     scheduleStop();
    }
#endif
    mod::TickRateEvent tickRateEvent{timer.tps, timer.tpsScale};
    net::minecraft::mod::runtime::luaHookTickRate(tickRateEvent);
    timer.tps = tickRateEvent.targetTps;
    timer.tpsScale = tickRateEvent.tpsScale;
    if(paused.load() && world != nullptr) {
     const float savedPartial = timer.partialTick;
     timer.advance();
     timer.partialTick = savedPartial;
    } else {
     timer.advance();
    }
    const std::int64_t tickStart = nanoTime();
    for(int i = 0; i < timer.ticksThisFrame; ++i) {
     ++ticksPlayed;
     try {
      tick();
     } catch(const net::minecraft::SessionLockException&) {
      ClientLog::LOGGER.log(LogLevel::Warning, "Session lock lost during tick; tearing down world");
      world = nullptr;
      setWorld(nullptr);
      setScreen(std::make_unique<gui::screen::world::WorldSaveConflictScreen>());
     }
    }
    runRenderPhase(nanoTime() - tickStart, frames, fpsWindowStart);
   } catch(const net::minecraft::SessionLockException&) {
    world = nullptr;
    setWorld(nullptr);
    setScreen(std::make_unique<gui::screen::world::WorldSaveConflictScreen>());
   } catch(const std::bad_alloc& exception) {
    ClientLog::LOGGER.log(LogLevel::Severe, "Out of memory", &exception);
    cleanHeap();
    setScreen(std::make_unique<gui::screen::OutOfMemoryScreen>());
    break;
   }
  }
 } catch(const render::ProgressRenderError&) {
 } catch(const std::exception& exception) {
  cleanHeap();
  ClientLog::LOGGER.log(LogLevel::Severe, "Unexpected error", &exception);
  gameCrashed(net::minecraft::util::crash::CrashReport("Unexpected error", exception.what()));
 }
 stop();
}
void Minecraft::forceResourceReload() {
 audio.reset();
 audio.start(&options);
 if(resourceDownloadThread != nullptr) {
  resourceDownloadThread->reload();
 }
}
bool Minecraft::isWorldRemote() const {
 return world != nullptr && world->isRemote();
}
void Minecraft::setWorld(World* worldIn) {
 worldSession_.setWorld(*this, worldIn, "", nullptr);
 notifyWorldChanged(worldIn);
}
void Minecraft::setWorld(World* worldIn, const std::string& message) {
 worldSession_.setWorld(*this, worldIn, message, nullptr);
 notifyWorldChanged(worldIn);
}
void Minecraft::setWorld(World* worldIn, const std::string& message, PlayerEntity* existingPlayer) {
 worldSession_.setWorld(*this, worldIn, message, existingPlayer);
 notifyWorldChanged(worldIn);
}
void Minecraft::notifyWorldChanged(World* worldIn) {
 if(serverProcessCoordinator_ != nullptr) {
  serverProcessCoordinator_->afterWorldChange(worldIn);
 }
}
void Minecraft::startGame(const std::string& worldName,
                          const std::string& name,
                          std::int64_t seed,
                          const std::unordered_map<std::string, std::string>& creationOptions,
                          bool skipModCheck) {
 setWorld(nullptr);
 if(worldStorageSource == nullptr) {
  mod::WorldOpenEvent openEvent{&worldName, true, &creationOptions};
  net::minecraft::mod::runtime::luaHookWorldOpen(openEvent);
  progressRenderer.progressStart("Generating level");
  progressRenderer.progressStage("Preparing world");
  worldSession_.ownedWorldStorageMut().reset();
  worldSession_.ownedWorldMut() =
      std::make_unique<World>(name, static_cast<std::uint64_t>(seed), creationOptions);
  if(stats != nullptr) {
   stats->increment(stat::Stats::CREATE_WORLD, 1);
   stats->increment(stat::Stats::START_GAME, 1);
  }
  mod::WorldStartEvent startEvent{worldSession_.ownedWorld(), &worldName, true};
  net::minecraft::mod::runtime::luaHookWorldStart(startEvent);
  setWorld(worldSession_.ownedWorld(), "Generating level");
  return;
 }
 if(worldStorageSource->needsConversion(worldName)) {
  worldSession_.convertAndSaveWorld(*this, worldName, name);
  return;
 }
 std::unordered_map<std::string, std::string> worldOptions = creationOptions;
 const std::optional<WorldProperties> existingProperties = worldStorageSource->getWorldProperties(worldName);
 const bool newWorld = !existingProperties.has_value();
 if(existingProperties.has_value()) {
  worldOptions = existingProperties->getModOptions();
 }
 mod::WorldOpenEvent openEvent{&worldName, newWorld, &worldOptions};
 net::minecraft::mod::runtime::luaHookWorldOpen(openEvent);
 worldSession_.ownedWorldStorageMut() = worldStorageSource->getSaveLoader(worldName, false);
 if(worldSession_.ownedWorldStorage() == nullptr) {
  throw std::runtime_error("Failed to create world storage for save '" + worldName + "'");
 }
 {
  const std::vector<std::string> missingMods =
      mod::runtime::WorldRequiredMods::missingForDirectory(worldSession_.ownedWorldStorage()->worldDirectory());
  if(!missingMods.empty()) {
   if(!skipModCheck) {
    auto confirmed = std::make_shared<bool>(false);
    setScreen(std::make_unique<gui::screen::ConfirmScreen>(
        [this, confirmed]() -> std::unique_ptr<gui::screen::Screen> {
         if(*confirmed) {
          return nullptr;
         }
         return std::make_unique<gui::screen::TitleScreen>();
        },
        [this, confirmed, worldName, name, seed, creationOptions](bool c) {
         *confirmed = c;
         if(c) {
          startGame(worldName, name, seed, creationOptions, true);
         }
        },
        "This world requires Lua mods that are not loaded:",
        mod::runtime::WorldRequiredMods::joinCsv(missingMods),
        "Load anyway",
        "Cancel"));
    return;
   }
   inGameHud.addChatMessage("World loaded with missing Lua mods. Unknown blocks replaced with air.");
  }
 }
 progressRenderer.progressStart("Generating level");
 progressRenderer.progressStage("Preparing world");
 worldSession_.ownedWorldMut() = std::make_unique<World>(
     worldSession_.ownedWorldStorage(), name, seed, /*deferSpawnInit=*/false, std::move(worldOptions));
 if(worldSession_.ownedWorld()->isNewWorld()) {
  if(stats != nullptr) {
   stats->increment(stat::Stats::CREATE_WORLD, 1);
   stats->increment(stat::Stats::START_GAME, 1);
  }
  mod::WorldStartEvent startEvent{worldSession_.ownedWorld(), &worldName, true};
  net::minecraft::mod::runtime::luaHookWorldStart(startEvent);
  setWorld(worldSession_.ownedWorld(), "Generating level");
 } else {
  if(stats != nullptr) {
   stats->increment(stat::Stats::LOAD_WORLD, 1);
   stats->increment(stat::Stats::START_GAME, 1);
  }
  mod::WorldStartEvent startEvent{worldSession_.ownedWorld(), &worldName, false};
  net::minecraft::mod::runtime::luaHookWorldStart(startEvent);
  setWorld(worldSession_.ownedWorld(), "Loading level");
 }
}
void Minecraft::changeDimension() {
 // Vanilla overworld<->Nether toggle, now a thin call over generic travel.
 if(player == nullptr) {
  return;
 }
 travelToDimension(player->dimensionId == -1 ? 0 : -1);
}
void Minecraft::travelToDimension(int dimensionId) {
 // Generic multi-dimension travel: move the player into ANY registered
 // dimension (vanilla or mod-registered). Coordinate scaling follows the
 // ratio of the two dimensions' movementFactor (overworld 1 : Nether 8 keeps
 // the classic 8:1 compression); other dimensions scale by their own factor.
 if(player == nullptr || world == nullptr || player->dimensionId == dimensionId) {
  return;
 }
 std::unique_ptr<Dimension> destDimension = Dimension::fromId(dimensionId);
 if(destDimension == nullptr) {
  return;
 }
 const double fromFactor = world->dimension != nullptr ? world->dimension->movementFactor() : 1.0;
 const double toFactor = destDimension->movementFactor();
 const double scale = toFactor != 0.0 ? fromFactor / toFactor : 1.0;
 player->dimensionId = dimensionId;
 world->remove(player);
 player->dead = false;
 const double px = player->x * scale;
 const double pz = player->z * scale;
 player->setPositionAndAnglesKeepPrevAngles(px, player->y, pz, player->yaw, player->pitch);
 if(player->isAlive()) {
  world->updateEntity(player, false);
 }
 std::unique_ptr<World> newWorld = std::make_unique<World>(world, std::move(destDimension));
 const std::string message = "Travelling to dimension " + std::to_string(dimensionId);
 // setWorld must run while the old world is still alive (it saves player data and
 // tears down listeners). Java lets the previous dimension linger until GC; keep it
 // in parkedDimensionWorld_ so chunk/render adapters never dangle mid-travel.
 setWorld(newWorld.get(), message, player);
 std::unique_ptr<World> previousWorld = std::move(worldSession_.ownedWorldMut());
 worldSession_.ownedWorldMut() = std::move(newWorld);
 worldSession_.parkedDimensionWorldMut() = std::move(previousWorld);
 player->world = world;
 if(player->isAlive()) {
  player->setPositionAndAnglesKeepPrevAngles(px, player->y, pz, player->yaw, player->pitch);
  world->updateEntity(player, false);
  PortalForcer forcer;
  forcer.moveToPortal(world, player);
 }
}
void Minecraft::loadResource(const std::string& path, const std::filesystem::path& file) {
 const std::size_t slash = path.find('/');
 if(slash == std::string::npos) {
  return;
 }
 const std::string prefix = path.substr(0, slash);
 const std::string remainder = path.substr(slash + 1);
 auto equalsIgnoreCase = [](const std::string& a, const std::string& b) {
  if(a.size() != b.size()) {
   return false;
  }
  for(std::size_t i = 0; i < a.size(); ++i) {
   if(std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
    return false;
   }
  }
  return true;
 };
 if(equalsIgnoreCase(prefix, "sound") || equalsIgnoreCase(prefix, "newsound")) {
  audio.registerEffect(remainder, file);
 } else if(equalsIgnoreCase(prefix, "streaming")) {
  audio.registerStreaming(remainder, file);
 } else if(equalsIgnoreCase(prefix, "music") || equalsIgnoreCase(prefix, "newmusic")) {
  audio.registerMusic(remainder, file);
 }
}
std::string Minecraft::getRenderChunkDebugInfo() const {
 return debug::ClientProfilerOverlay::getRenderChunkDebugInfo(*this);
}
std::string Minecraft::getRenderEntityDebugInfo() const {
 return debug::ClientProfilerOverlay::getRenderEntityDebugInfo(*this);
}
std::string Minecraft::getChunkSourceDebugInfo() const {
 return debug::ClientProfilerOverlay::getChunkSourceDebugInfo(*this);
}
std::string Minecraft::getWorldDebugInfo() const {
 return debug::ClientProfilerOverlay::getWorldDebugInfo(*this);
}
void Minecraft::respawnPlayer(bool worldSpawn, int dimension) {
 if(world == nullptr || interactionManager == nullptr) {
  return;
 }
 if(!world->isRemote() && world->dimension != nullptr && !world->dimension->hasWorldSpawn()) {
  changeDimension();
 }
 std::optional<Vec3i> bedSpawnPos;
 std::optional<Vec3i> respawnPos;
 bool useBedSpawn = true;
 if(player != nullptr && !worldSpawn) {
  bedSpawnPos = player->getSpawnPos();
  if(bedSpawnPos.has_value()) {
   respawnPos = entity::player::PlayerEntity::findRespawnPosition(world, *bedSpawnPos);
   if(!respawnPos.has_value()) {
    player->sendMessage("tile.bed.notValid");
   }
  }
 }
 if(!respawnPos.has_value()) {
  respawnPos = world->getSpawnPos();
  useBedSpawn = false;
 }
 world->setChunkCacheCenterFromBlockPos(respawnPos->x, respawnPos->z);
 world->updateSpawnPosition();
 world->updateEntityLists();
 int playerId = 0;
 if(player != nullptr) {
  playerId = player->id;
  if(player->fishHook != nullptr) {
   player->fishHook->markDead();
   player->fishHook = nullptr;
  }
  world->serverRemove(player);
 }
 camera = nullptr;
 worldSession_.ownedPlayerMut() = std::unique_ptr<entity::player::ClientPlayerEntity>(
     static_cast<entity::player::ClientPlayerEntity*>(interactionManager->createPlayer(world)));
 player = worldSession_.ownedPlayer();
 player->dimensionId = dimension;
 camera = player;
 if(useBedSpawn && bedSpawnPos.has_value()) {
  player->setSpawnPos(bedSpawnPos);
  player->setPositionAndAnglesKeepPrevAngles(
      static_cast<double>(respawnPos->x) + 0.5,
      static_cast<double>(respawnPos->y) + static_cast<double>(player->standingEyeHeight),
      static_cast<double>(respawnPos->z) + 0.5,
      0.0f,
      0.0f);
 }
 player->teleportTop();
 interactionManager->preparePlayer(player);
 world->addPlayer(player);
 player->id = playerId;
 player->spawn();
 interactionManager->preparePlayerRespawn(player);
 worldSession_.prepareWorld(*this, "Respawning");
 msauth::refreshPlayerTextures(*this);
 if(dynamic_cast<gui::screen::DeathScreen*>(currentScreen()) != nullptr) {
  setScreen(nullptr);
 }
}
void Minecraft::start(const std::string& username, const std::string& sessionId) {
 startAndConnect(username, sessionId, nullptr);
}
void Minecraft::startAndConnect(const std::string& username, const std::string& sessionId, const std::string* server) {
 auto client = std::make_unique<RunnableMinecraft>(nullptr, nullptr, nullptr, 854, 480, false);
 client->hostAddress = "www.minecraft.net";
 if(!username.empty() && !sessionId.empty()) {
  client->session = util::Session(username, sessionId);
 } else {
  client->session = util::Session("Player" + std::to_string(currentTimeMillis() % 1000LL), "");
 }
 if(server != nullptr) {
  const std::size_t colon = server->find(':');
  if(colon != std::string::npos) {
   client->setStartupServer(server->substr(0, colon), std::stoi(server->substr(colon + 1)));
  }
 }
 client->run();
}
int Minecraft::main(int argc, char** argv) {
 net::minecraft::block::initializeBlocks();
 std::string username = "Player" + std::to_string(currentTimeMillis() % 1000LL);
 std::string sessionId = "-";
 std::string serverArg;
 const std::string* serverPtr = nullptr;
 std::vector<std::string> positional;
 for(int i = 1; i < argc; ++i) {
  if(argv[i] == nullptr) {
   continue;
  }
  positional.push_back(argv[i]);
 }
 if(!positional.empty()) {
  username = positional[0];
 }
 if(positional.size() > 1) {
  sessionId = positional[1];
 }
 if(positional.size() > 2) {
  serverArg = positional[2];
  if(!serverArg.empty()) {
   serverPtr = &serverArg;
  }
 }
 try {
  startAndConnect(username, sessionId, serverPtr);
 } catch(const std::exception& exception) {
  ClientLog::LOGGER.log(LogLevel::Severe, "Failed to start", &exception);
#ifdef _WIN32
  diagnostics::reportFatalError("Minecraft Native - failed to start", std::string(exception.what()));
  diagnostics::pauseBeforeExit();
#endif
  return 1;
 } catch(...) {
  const char* message = "Unknown exception during startup.";
  ClientLog::LOGGER.log(LogLevel::Severe, message);
#ifdef _WIN32
  diagnostics::reportFatalError("Minecraft Native - failed to start", message);
  diagnostics::pauseBeforeExit();
#endif
  return 1;
 }
 return 0;
}
bool Minecraft::isDisplayGui() {
 return INSTANCE == nullptr || !INSTANCE->options.hideHud;
}
bool Minecraft::isFancyGraphicsEnabled() {
 return INSTANCE != nullptr && INSTANCE->options.fancyGraphics;
}
bool Minecraft::isAmbientOcclusionEnabled() {
 return INSTANCE != nullptr && INSTANCE->options.ao;
}
bool Minecraft::isDebugProfilerEnabled() {
 return INSTANCE != nullptr && INSTANCE->options.debugHud;
}
bool Minecraft::isCommand(const std::string& /*message*/) const {
 // Vanilla b1.7.3 has no client-side commands; slash-prefixed input is sent
 // to the server as a normal chat message. Java Minecraft.isCommand always
 // returns false, so don't suppress the send here.
 return false;
}
} // namespace net::minecraft::client
