#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/MinecraftApplet.hpp"
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/client/debug/ClientProfilerOverlay.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/lifecycle/ClientInitializer.hpp"
#include "net/minecraft/client/lifecycle/ClientLaunch.hpp"
#include "net/minecraft/client/lifecycle/ClientShutdown.hpp"
#include "net/minecraft/client/render/LoadingScreenRenderer.hpp"
#include "net/minecraft/client/session/SessionValidator.hpp"
#include "net/minecraft/client/util/DisplayManager.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include "net/minecraft/client/util/TimerHackThread.hpp"

#include <cctype>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Screenshot.hpp"
#include "net/minecraft/client/SingleplayerInteractionManager.hpp"
#include "net/minecraft/client/TestInteractionManager.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gui/screen/DeathScreen.hpp"
#include "net/minecraft/client/gui/screen/FatalErrorScreen.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/gui/screen/OutOfMemoryScreen.hpp"
#include "net/minecraft/client/gui/screen/world/WorldSaveConflictScreen.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/ProgressRenderError.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/resource/language/TranslationStorage.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/crash/CrashReport.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/dimension/PortalForcer.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
#include "net/minecraft/world/storage/exception/SessionLockException.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace net::minecraft::client {

namespace option = net::minecraft::client::option;

namespace {

std::int64_t currentTimeMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::int64_t nanoTime()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace

Minecraft::Minecraft(void* component, void* canvasIn, net::minecraft::MinecraftApplet* appletIn, int width, int height, bool fullscreenIn)
    : fullscreen(fullscreenIn),
      displayWidth(width),
      displayHeight(height),
      canvas(canvasIn),
      applet(appletIn),
      initWidth(width),
      initHeight(height)
{
    (void)component;
    initializeBlocks();
    timerHackThread_ = std::make_unique<util::TimerHackThread>(this, "Timer hack thread");
    if (applet == nullptr) {
        isApplet = false;
    }
    INSTANCE = this;
}

Minecraft::~Minecraft() = default;

void Minecraft::gameCrashed(const net::minecraft::util::crash::CrashReport& crashReport)
{
    crashed = true;
    handleCrash(crashReport);
}

void Minecraft::setStartupServer(const std::string& address, int port)
{
    startupServerAddress_ = address;
    startupServerPort = port;
}

void Minecraft::init()
{
    initializeBlocks();
    util::DisplayManager::setupAndCreateDisplay(*this);
    lifecycle::ClientInitializer::bootstrap(*this);
}

void Minecraft::draw(int x, int y, int u, int v, int width, int height)
{
    render::LoadingScreenRenderer::draw(*this, x, y, u, v, width, height);
}

std::atomic<std::int64_t>& Minecraft::failedSessionCheckTime() noexcept
{
    return session::SessionValidator::failedSessionCheckTime;
}

std::filesystem::path Minecraft::getRunDirectory()
{
    return util::MinecraftDirectories::getRunDirectory();
}

std::filesystem::path Minecraft::getApplicationDirectory(const std::string& name)
{
    return util::MinecraftDirectories::getApplicationDirectory(name);
}

net::minecraft::WorldStorageSource* Minecraft::getWorldStorageSource()
{
    return worldStorageSource.get();
}

void Minecraft::setScreen(std::unique_ptr<gui::screen::Screen> screen)
{
    screenStack_.setScreen(std::move(screen));
}

void Minecraft::ClientHostAdapter::lockMouse()
{
    if (!input::InputSystem::instance().acceptsInput()) {
        return;
    }
    if (minecraft_.focused.load()) {
        return;
    }
    minecraft_.focused = true;
    input::InputSystem::instance().lockCursor();
}

void Minecraft::stop()
{
    lifecycle::ClientShutdown::stop(*this);
}

void Minecraft::cleanHeap()
{
    lifecycle::ClientShutdown::cleanHeap(*this);
}

void Minecraft::handleScreenshotKey()
{
    if (input::InputSystem::instance().isKeyDown(60)) {
        if (!screenshotKeyDown) {
            screenshotKeyDown = true;
            inGameHud.addChatMessage(Screenshot::take(getRunDirectory(), displayWidth, displayHeight));
        }
    } else {
        screenshotKeyDown = false;
    }
}

void Minecraft::scheduleStop()
{
    running = false;
}

void Minecraft::lockMouse()
{
    if (!input::InputSystem::instance().acceptsInput()) {
        return;
    }
    if (focused.load()) {
        return;
    }
    focused = true;
    input::InputSystem::instance().lockCursor();
    screenStack_.setScreen(nullptr);
    attackCooldown = 10000;
    lastClickTicks = ticksPlayed + 10000;
}

void Minecraft::unlockMouse()
{
    if (!focused.load()) {
        return;
    }
    input::InputSystem::instance().resetBindings();
    focused = false;
    input::InputSystem::instance().unlockCursor();
}

void Minecraft::pauseGame()
{
    if (screenStack_.hasScreen()) {
        return;
    }
    setScreen(std::make_unique<gui::screen::GameMenuScreen>());
}

void Minecraft::handleMouseDown(int button, bool holdingAttack)
{
    if (interactionManager == nullptr || interactionManager->noTick) {
        return;
    }
    if (!holdingAttack) {
        attackCooldown = 0;
    }
    if (button == 0 && attackCooldown > 0) {
        return;
    }
    if (holdingAttack && crosshairTarget.has_value() && crosshairTarget->type == HitResultType::BLOCK && button == 0) {
        interactionManager->processBlockBreakingAction(crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ, crosshairTarget->side);
        particleManager.addBlockBreakingParticles(
            crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ, crosshairTarget->side);
    } else {
        interactionManager->cancelBlockBreaking();
    }
}

void Minecraft::handleMouseClick(int button)
{
    if (player == nullptr || interactionManager == nullptr) {
        return;
    }
    if (button == 0 && attackCooldown > 0) {
        return;
    }
    if (button == 0) {
        player->swingHand();
    }
    bool useSelectedItem = true;
    if (!crosshairTarget.has_value()) {
        if (button == 0 && dynamic_cast<TestInteractionManager*>(interactionManager.get()) == nullptr) {
            attackCooldown = 10;
        }
    } else if (crosshairTarget->type == HitResultType::ENTITY) {
        if (button == 0) {
            interactionManager->attackEntity(player, crosshairTarget->entity);
        }
        if (button == 1) {
            interactionManager->interactEntity(player, crosshairTarget->entity);
        }
    } else if (crosshairTarget->type == HitResultType::BLOCK) {
        if (button == 0) {
            interactionManager->attackBlock(crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ, crosshairTarget->side);
        } else {
            ItemStack* itemStack = player->inventory.getSelectedItem();
            const int previousCount = itemStack != nullptr ? itemStack->count : 0;
            if (interactionManager->interactBlock(player, world, itemStack, crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ, crosshairTarget->side)) {
                useSelectedItem = false;
                player->swingHand();
            }
            if (itemStack == nullptr) {
                return;
            }
            if (itemStack->count == 0) {
                player->inventory.main[player->inventory.selectedSlot] = ItemStack {};
            } else if (itemStack->count != previousCount && gameRenderer != nullptr && gameRenderer->heldItemRendererPtr() != nullptr) {
                gameRenderer->heldItemRendererPtr()->place();
            }
        }
    }
    ItemStack* selected = player->inventory.getSelectedItem();
    if (useSelectedItem && button == 1 && selected != nullptr && interactionManager->interactItem(player, world, selected)) {
        if (gameRenderer != nullptr && gameRenderer->heldItemRendererPtr() != nullptr) {
            gameRenderer->heldItemRendererPtr()->use();
        }
    }
}

void Minecraft::toggleFullscreen()
{
    util::DisplayManager::toggleFullscreen(*this);
}

void Minecraft::scheduleScreenResize()
{
    util::DisplayManager::scheduleScreenResize(*this);
}

void Minecraft::resize(int width, int height)
{
    util::DisplayManager::resize(*this, width, height);
}

void Minecraft::handlePickBlock()
{
    if (!crosshairTarget.has_value() || world == nullptr || player == nullptr) {
        return;
    }
    int blockId = world->getBlockId(crosshairTarget->blockX, crosshairTarget->blockY, crosshairTarget->blockZ);
    if (blockId == 2) {
        blockId = 3;
    }
    if (blockId == 43) {
        blockId = 44;
    }
    if (blockId == 7) {
        blockId = 1;
    }
    player->inventory.setHeldItem(blockId, dynamic_cast<TestInteractionManager*>(interactionManager.get()) != nullptr);
}

void Minecraft::startSessionCheck()
{
    session::SessionValidator::startSessionCheck(*this);
}

void Minecraft::runWorldSimulation()
{
    if (world == nullptr) {
        return;
    }
    if (player != nullptr) {
        worldSession_.tickJoinPlayerCounter(*this);
    }
    world->difficulty = options.difficulty;
    if (world->isRemote()) {
        world->difficulty = 3;
    }
    if (!paused.load()) {
        if (gameRenderer != nullptr) {
            gameRenderer->updateCamera();
        }
        if (atmosphereRenderer != nullptr) {
            atmosphereRenderer->tick();
        }
        if (world->lightningTicksLeft > 0) {
            --world->lightningTicksLeft;
        }
        world->tickEntities();
    }
    if (!paused.load() || isWorldRemote()) {
        world->allowSpawning(options.difficulty > 0, true);
        world->tick();
    }
    if (!paused.load() && player != nullptr) {
        world->displayTick(MathHelper::floor(player->x), MathHelper::floor(player->y), MathHelper::floor(player->z));
    }
    if (!paused.load()) {
        particleManager.removeDeadParticles();
    }
}

void Minecraft::tick()
{
    if (ticksPlayed == 6000) {
        startSessionCheck();
    }
    if (stats != nullptr) {
        stats->tick();
    }
    inGameHud.tick();
    if (gameRenderer != nullptr) {
        gameRenderer->updateTargetedEntity(1.0f);
    }
    if (player != nullptr && world != nullptr) {
        if (ChunkSource* chunkSource = world->getChunkSource()) {
            if (auto* legacyCache = dynamic_cast<LegacyChunkCache*>(chunkSource)) {
                const int chunkX = MathHelper::floor(player->x) >> 4;
                const int chunkZ = MathHelper::floor(player->z) >> 4;
                legacyCache->setSpawnPoint(chunkX, chunkZ);
            }
        }
    }
    if (!paused.load() && world != nullptr && interactionManager != nullptr) {
        interactionManager->tick();
        if (worldRenderer != nullptr) {
            worldRenderer->miningProgress = interactionManager->getBlockBreakingProgress(timer.partialTick);
        }
    }
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureManager.getTextureId("/terrain.png"));
    if (!paused.load()) {
        textureManager.tick();
    }
    // INVARIANT: Tick order — do not reorder.
    // InputSystem::beginFrame → screenStack.tickScreens (Screen::tickInput) → pollGame.
    // runWorldSimulation runs after input is settled.
    input::InputSystem::instance().beginFrame(*this);
    screenStack_.tickScreens(*this);
    if (pendingScreenResize_) {
        pendingScreenResize_ = false;
        resize(displayWidth, displayHeight);
    }
    input::InputSystem::instance().pollGame(*this);
    runWorldSimulation();
    lastTickTime = currentTimeMillis();
}

void Minecraft::runRenderPhase(std::int64_t tickDuration, int& frames, std::int64_t& fpsWindowStart)
{
    util::DisplayManager::logGlError(*this, "Pre render");
    render::block::BlockRenderManager::fancyGraphics = options.fancyGraphics;
    audio.updateListener(player, timer.partialTick);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    if (world != nullptr) {
        world->doLightingUpdates();
    }
    // Key 65 (F7) — defer present until after render when held.
    if (!input::InputSystem::instance().isKeyDown(65)) {
        util::DisplayManager::pumpAndPresent();
    }
    if (!skipGameRender) {
        if (interactionManager != nullptr) {
            interactionManager->update(timer.partialTick);
        }
        if (gameRenderer != nullptr) {
            gameRenderer->onFrameUpdate(timer.partialTick);
        }
    }
    if (player != nullptr && player->isInsideWall()) {
        options.thirdPerson = false;
    }
#ifdef _WIN32
    if (!util::DisplayManager::isActive()) {
        if (fullscreen) {
            toggleFullscreen();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
    if (options.debugHud && !options.ofFastDebugInfo) {
        debug::ClientProfilerOverlay::renderProfilerChart(*this, tickDuration);
    } else {
        debug::ClientProfilerOverlay::recordFrameTime(*this);
    }
    toast.tick();
    std::this_thread::yield();
    handleScreenshotKey();
    if (input::InputSystem::instance().isKeyDown(65)) {
        util::DisplayManager::pumpAndPresent();
    }
    util::DisplayManager::logGlError(*this, "Post render");
    ++frames;
    paused = !isWorldRemote() && currentScreen() != nullptr && currentScreen()->shouldPause();
    while (currentTimeMillis() >= fpsWindowStart + 1000) {
        if (options.ofFastDebugInfo) {
            debugText = std::to_string(frames) + " fps";
        } else {
            debugText = std::to_string(frames) + " fps, "
                + std::to_string(render::chunk::ChunkBuilder::chunkUpdates) + " chunk updates";
        }
        render::chunk::ChunkBuilder::chunkUpdates = 0;
        fpsWindowStart += 1000;
        frames = 0;
    }
}

void Minecraft::run()
{
    running = true;
    try {
        init();
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        gameCrashed(net::minecraft::util::crash::CrashReport("Failed to start game", exception.what()));
        return;
    }
    try {
        std::int64_t fpsWindowStart = currentTimeMillis();
        int frames = 0;
        while (running.load()) {
            try {
                screenStack_.flushRetired();
                if (applet != nullptr && !applet->isActive()) {
                    break;
                }
#ifdef _WIN32
                if (canvas == nullptr && util::DisplayManager::isCloseRequested()) {
                    scheduleStop();
                }
#endif
                if (paused.load() && world != nullptr) {
                    const float savedPartial = timer.partialTick;
                    timer.advance();
                    timer.partialTick = savedPartial;
                } else {
                    timer.advance();
                }
                const std::int64_t tickStart = nanoTime();
                for (int i = 0; i < timer.ticksThisFrame; ++i) {
                    ++ticksPlayed;
                    try {
                        tick();
                    } catch (const net::minecraft::SessionLockException&) {
                        world = nullptr;
                        setWorld(nullptr);
                        setScreen(std::make_unique<gui::screen::world::WorldSaveConflictScreen>());
                    }
                }
                runRenderPhase(nanoTime() - tickStart, frames, fpsWindowStart);
            } catch (const net::minecraft::SessionLockException&) {
                world = nullptr;
                setWorld(nullptr);
                setScreen(std::make_unique<gui::screen::world::WorldSaveConflictScreen>());
            } catch (const std::bad_alloc& exception) {
                std::cerr << exception.what() << '\n';
                cleanHeap();
                setScreen(std::make_unique<gui::screen::OutOfMemoryScreen>());
                break;
            }
        }
    } catch (const render::ProgressRenderError&) {
    } catch (const std::exception& exception) {
        cleanHeap();
        std::cerr << exception.what() << std::endl;
        gameCrashed(net::minecraft::util::crash::CrashReport("Unexpected error", exception.what()));
    }
    stop();
}

void Minecraft::forceResourceReload()
{
    std::cout << "FORCING RELOAD!" << std::endl;
    audio.reset();
    audio.start(&options);
    if (resourceDownloadThread != nullptr) {
        resourceDownloadThread->reload();
    }
}

bool Minecraft::isWorldRemote() const
{
    return world != nullptr && world->isRemote();
}

void Minecraft::setWorld(World* worldIn)
{
    worldSession_.setWorld(*this, worldIn);
}

void Minecraft::setWorld(World* worldIn, const std::string& message)
{
    worldSession_.setWorld(*this, worldIn, message);
}

void Minecraft::setWorld(World* worldIn, const std::string& message, PlayerEntity* existingPlayer)
{
    worldSession_.setWorld(*this, worldIn, message, existingPlayer);
}

void Minecraft::prepareWorld(const std::string& worldName)
{
    worldSession_.prepareWorld(*this, worldName);
}

void Minecraft::convertAndSaveWorld(const std::string& worldName, const std::string& name)
{
    worldSession_.convertAndSaveWorld(*this, worldName, name);
}

void Minecraft::startGame(const std::string& worldName, const std::string& name, std::int64_t seed)
{
    setWorld(nullptr);
    if (worldStorageSource == nullptr) {
        progressRenderer.progressStart("Generating level");
        progressRenderer.progressStage("Preparing world");
        worldSession_.ownedWorldStorageMut().reset();
        worldSession_.ownedWorldMut() = std::make_unique<World>(name, static_cast<std::uint64_t>(seed));
        if (stats != nullptr) {
            stats->increment(stat::Stats::CREATE_WORLD, 1);
            stats->increment(stat::Stats::START_GAME, 1);
        }
        setWorld(worldSession_.ownedWorld(), "Generating level");
        return;
    }
    if (worldStorageSource->needsConversion(worldName)) {
        convertAndSaveWorld(worldName, name);
        return;
    }
    worldSession_.ownedWorldStorageMut() = worldStorageSource->getSaveLoader(worldName, false);
    if (worldSession_.ownedWorldStorage() == nullptr) {
        throw std::runtime_error("Failed to create world storage for save '" + worldName + "'");
    }
    progressRenderer.progressStart("Generating level");
    progressRenderer.progressStage("Preparing world");
    worldSession_.ownedWorldMut() = std::make_unique<World>(worldSession_.ownedWorldStorage(), name, seed);
    if (worldSession_.ownedWorld()->newWorld) {
        if (stats != nullptr) {
            stats->increment(stat::Stats::CREATE_WORLD, 1);
            stats->increment(stat::Stats::START_GAME, 1);
        }
        setWorld(worldSession_.ownedWorld(), "Generating level");
    } else {
        if (stats != nullptr) {
            stats->increment(stat::Stats::LOAD_WORLD, 1);
            stats->increment(stat::Stats::START_GAME, 1);
        }
        setWorld(worldSession_.ownedWorld(), "Loading level");
    }
}

void Minecraft::changeDimension()
{
    if (player == nullptr || world == nullptr) {
        return;
    }
    std::cout << "Toggling dimension!!" << std::endl;
    player->dimensionId = player->dimensionId == -1 ? 0 : -1;
    world->remove(player);
    player->dead = false;
    double px = player->x;
    double pz = player->z;
    constexpr double scale = 8.0;
    std::string message;
    std::unique_ptr<World> newWorld;
    if (player->dimensionId == -1) {
        px /= scale;
        pz /= scale;
        player->setPositionAndAnglesKeepPrevAngles(px, player->y, pz, player->yaw, player->pitch);
        if (player->isAlive()) {
            world->updateEntity(player, false);
        }
        newWorld = std::make_unique<World>(world, Dimension::fromId(-1));
        message = "Entering the Nether";
    } else {
        px *= scale;
        pz *= scale;
        player->setPositionAndAnglesKeepPrevAngles(px, player->y, pz, player->yaw, player->pitch);
        if (player->isAlive()) {
            world->updateEntity(player, false);
        }
        newWorld = std::make_unique<World>(world, Dimension::fromId(0));
        message = "Leaving the Nether";
    }
    // setWorld must run while the old world is still alive (it saves player data and
    // tears down listeners). Java keeps both worlds alive until setWorld replaces the
    // reference; mirror that by deferring ownedWorld_ replacement until after setWorld.
    setWorld(newWorld.get(), message, player);
    worldSession_.ownedWorldMut() = std::move(newWorld);
    player->world = world;
    if (player->isAlive()) {
        player->setPositionAndAnglesKeepPrevAngles(px, player->y, pz, player->yaw, player->pitch);
        world->updateEntity(player, false);
        PortalForcer forcer;
        forcer.moveToPortal(world, player);
    }
}

void Minecraft::loadResource(const std::string& path, const std::filesystem::path& file)
{
    const std::size_t slash = path.find('/');
    if (slash == std::string::npos) {
        return;
    }
    const std::string prefix = path.substr(0, slash);
    const std::string remainder = path.substr(slash + 1);
    auto equalsIgnoreCase = [](const std::string& a, const std::string& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i]))
                != std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    };
    if (equalsIgnoreCase(prefix, "sound") || equalsIgnoreCase(prefix, "newsound")) {
        audio.registerEffect(remainder, file);
    } else if (equalsIgnoreCase(prefix, "streaming")) {
        audio.registerStreaming(remainder, file);
    } else if (equalsIgnoreCase(prefix, "music") || equalsIgnoreCase(prefix, "newmusic")) {
        audio.registerMusic(remainder, file);
    }
}

render::OpenGlCapabilities* Minecraft::getOpenGlCapabilities()
{
    return openGlCapabilities;
}

std::string Minecraft::getRenderChunkDebugInfo() const
{
    return debug::ClientProfilerOverlay::getRenderChunkDebugInfo(*this);
}

std::string Minecraft::getRenderEntityDebugInfo() const
{
    return debug::ClientProfilerOverlay::getRenderEntityDebugInfo(*this);
}

std::string Minecraft::getChunkSourceDebugInfo() const
{
    return debug::ClientProfilerOverlay::getChunkSourceDebugInfo(*this);
}

std::string Minecraft::getWorldDebugInfo() const
{
    return debug::ClientProfilerOverlay::getWorldDebugInfo(*this);
}

void Minecraft::respawnPlayer(bool worldSpawn, int dimension)
{
    if (world == nullptr || interactionManager == nullptr) {
        return;
    }
    if (!world->isRemote() && world->dimension != nullptr && !world->dimension->hasWorldSpawn()) {
        changeDimension();
    }

    std::optional<Vec3i> bedSpawnPos;
    std::optional<Vec3i> respawnPos;
    bool useBedSpawn = true;
    if (player != nullptr && !worldSpawn) {
        bedSpawnPos = player->getSpawnPos();
        if (bedSpawnPos.has_value()) {
            respawnPos = entity::player::PlayerEntity::findRespawnPosition(world, *bedSpawnPos);
            if (!respawnPos.has_value()) {
                player->sendMessage("tile.bed.notValid");
            }
        }
    }
    if (!respawnPos.has_value()) {
        respawnPos = world->getSpawnPos();
        useBedSpawn = false;
    }

    if (ChunkSource* chunkSource = world->getChunkSource()) {
        if (auto* legacyCache = dynamic_cast<LegacyChunkCache*>(chunkSource)) {
            legacyCache->setSpawnPoint(respawnPos->x >> 4, respawnPos->z >> 4);
        }
    }
    world->updateSpawnPosition();
    world->updateEntityLists();

    int playerId = 0;
    if (player != nullptr) {
        playerId = player->id;
        if (player->fishHook != nullptr) {
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
    if (useBedSpawn && bedSpawnPos.has_value()) {
        player->setSpawnPos(bedSpawnPos);
        player->setPositionAndAnglesKeepPrevAngles(
            static_cast<double>(respawnPos->x) + 0.5,
            static_cast<double>(respawnPos->y) + 0.1,
            static_cast<double>(respawnPos->z) + 0.5,
            0.0f,
            0.0f);
    }
    player->teleportTop();
    interactionManager->preparePlayer(player);
    player->id = playerId;
    interactionManager->preparePlayerRespawn(player);
    prepareWorld("Respawning");
    world->addPlayer(player);
    player->spawn();
    if (dynamic_cast<gui::screen::DeathScreen*>(currentScreen()) != nullptr) {
        setScreen(nullptr);
    }
}

void Minecraft::start(const std::string& username, const std::string& sessionId)
{
    lifecycle::ClientLaunch::start(username, sessionId);
}

void Minecraft::startAndConnect(const std::string& username, const std::string& sessionId, const std::string* server)
{
    lifecycle::ClientLaunch::startAndConnect(username, sessionId, server);
}

int Minecraft::main(int argc, char** argv)
{
    return lifecycle::ClientLaunch::main(argc, argv);
}

bool Minecraft::isDisplayGui()
{
    return INSTANCE == nullptr || !INSTANCE->options.hideHud;
}

bool Minecraft::isFancyGraphicsEnabled()
{
    return INSTANCE != nullptr && INSTANCE->options.fancyGraphics;
}

bool Minecraft::isAmbientOcclusionEnabled()
{
    return INSTANCE != nullptr && option::resolve(INSTANCE->options).ambientOcclusionActive;
}

bool Minecraft::isDebugProfilerEnabled()
{
    return INSTANCE != nullptr && INSTANCE->options.debugHud;
}

bool Minecraft::isCommand(const std::string& message) const
{
    return message.rfind("/", 0) == 0;
}

} // namespace net::minecraft::client
