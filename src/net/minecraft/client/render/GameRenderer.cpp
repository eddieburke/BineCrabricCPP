#include "net/minecraft/client/render/GameRenderer.hpp"

#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/StereoSession.hpp"

#include "net/minecraft/client/gui/screen/ChatScreen.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/TestInteractionManager.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/particle/FireSmokeParticle.hpp"
#include "net/minecraft/client/particle/RainSplashParticle.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/culling/FrustumCuller.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"
#include "net/minecraft/client/render/platform/GuiGlState.hpp"
#include "net/minecraft/client/render/platform/StereoRendering.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biomes.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"

#ifdef _WIN32
#include "net/minecraft/client/util/DisplayManager.hpp"
#endif

#ifdef MINECRAFT_GL_REAL
#include <GL/glu.h>
#endif

#include <chrono>
#include <cmath>
#include <cstring>
#include <optional>
#include <thread>
#include <vector>

namespace net::minecraft::client::render {

namespace option = net::minecraft::client::option;

namespace {

constexpr int kClearColorAndDepth = 0x00004000 | 0x00000100;
constexpr int kBedBlockId = 26;
constexpr float kPiF = 3.14159265f;

void gluPerspectiveFov(float fovyDeg, float aspect, float zNear, float zFar)
{
#ifdef MINECRAFT_GL_REAL
    ::gluPerspective(static_cast<GLdouble>(fovyDeg), static_cast<GLdouble>(aspect),
        static_cast<GLdouble>(zNear), static_cast<GLdouble>(zFar));
#else
    (void)fovyDeg;
    (void)aspect;
    (void)zNear;
    (void)zFar;
#endif
}

[[nodiscard]] double vec3Distance(const Vec3d& a, const Vec3d& b)
{
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    const double dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

[[nodiscard]] bool boxContains(const Box& box, const Vec3d& point)
{
    return point.x >= box.minX && point.x <= box.maxX
        && point.y >= box.minY && point.y <= box.maxY
        && point.z >= box.minZ && point.z <= box.maxZ;
}

[[nodiscard]] std::optional<HitResult> boxRaycast(const Box& box, const Vec3d& start, const Vec3d& end)
{
    Vec3d dir {end.x - start.x, end.y - start.y, end.z - start.z};
    double tMin = 0.0;
    double tMax = 1.0;

    for (int axis = 0; axis < 3; ++axis) {
        const double s = axis == 0 ? start.x : (axis == 1 ? start.y : start.z);
        const double d = axis == 0 ? dir.x : (axis == 1 ? dir.y : dir.z);
        const double bMin = axis == 0 ? box.minX : (axis == 1 ? box.minY : box.minZ);
        const double bMax = axis == 0 ? box.maxX : (axis == 1 ? box.maxY : box.maxZ);

        if (std::abs(d) < 1.0e-7) {
            if (s < bMin || s > bMax) {
                return std::nullopt;
            }
            continue;
        }

        double t1 = (bMin - s) / d;
        double t2 = (bMax - s) / d;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) {
            return std::nullopt;
        }
    }

    const double tHit = tMin >= 0.0 ? tMin : tMax;
    if (tHit < 0.0 || tHit > 1.0) {
        return std::nullopt;
    }

    return HitResult {
        MathHelper::floor(start.x + dir.x * tHit),
        MathHelper::floor(start.y + dir.y * tHit),
        MathHelper::floor(start.z + dir.z * tHit),
        0,
        {start.x + dir.x * tHit, start.y + dir.y * tHit, start.z + dir.z * tHit}};
}

[[nodiscard]] float worldBrightness(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return 1.0f;
    }
    // GameRenderer.updateCamera — World.getLightBrightness (0..1 via lightLevelToLuminance).
    return world->getLightBrightness(x, y, z);
}

[[nodiscard]] float worldRainGradient(Minecraft* client, World* world, float tickDelta)
{
    if (client == nullptr) {
        return world != nullptr ? world->getRainGradient(tickDelta) : 0.0f;
    }
    return client::option::rainGradient(client::option::resolve(client->options), world, tickDelta);
}

[[nodiscard]] std::optional<HitResult> entityRaycast(World* world, LivingEntity* camera, double reach,
    float tickDelta)
{
    if (camera == nullptr || world == nullptr) {
        return std::nullopt;
    }
    const Vec3d start = camera->getPosition(tickDelta);
    const Vec3d look = camera->getLookVector(tickDelta);
    return world->raycast(start, start + Vec3d{look.x * reach, look.y * reach, look.z * reach});
}

[[nodiscard]] std::optional<HitResult> worldRaycast(World* world, const Vec3d& start, const Vec3d& end)
{
    if (world == nullptr) {
        return std::nullopt;
    }
    return world->raycast(start, end);
}

[[nodiscard]] std::int64_t nowMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

[[nodiscard]] std::int64_t nowNanos()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void sleepMillis(std::int64_t ms)
{
    if (ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

} // namespace

GameRenderer::GameRenderer(net::minecraft::client::Minecraft* clientIn)
    : client(clientIn)
    , heldItemRenderer(std::make_unique<item::HeldItemRenderer>(clientIn))
    , lastInactiveTime(nowMillis())
{
}

void GameRenderer::updateCamera()
{
    if (client == nullptr) {
        return;
    }

    lastViewBob = viewBob;
    prevThirdPersonDistance = thirdPersonDistance;
    prevThirdPersonYaw = thirdPersonYaw;
    prevThirdPersonPitch = thirdPersonPitch;
    prevCameraRoll = cameraRoll;
    prevCameraRollAmount = cameraRollAmount;

    if (client->camera == nullptr) {
        client->camera = client->player;
    }

    float ambient = 1.0f;
    if (client->world != nullptr && client->camera != nullptr) {
        ambient = worldBrightness(client->world,
            MathHelper::floor(client->camera->x),
            MathHelper::floor(client->camera->y),
            MathHelper::floor(client->camera->z));
    }
    const float viewDistBlend = static_cast<float>(3 - client->options.viewDistance) / 3.0f;
    const float blended = ambient * (1.0f - viewDistBlend) + viewDistBlend;
    viewBob += (blended - viewBob) * 0.1f;

    ++ticks;
    if (heldItemRenderer != nullptr) {
        heldItemRenderer->tick();
    }
    renderRain();
}

void GameRenderer::updateTargetedEntity(float tickDelta)
{
    if (client == nullptr || client->camera == nullptr || client->world == nullptr
        || client->interactionManager == nullptr) {
        return;
    }

    auto* livingCamera = dynamic_cast<LivingEntity*>(client->camera);
    if (livingCamera == nullptr) {
        return;
    }

    double reach = static_cast<double>(client->interactionManager->getReachDistance());
    client->crosshairTarget = entityRaycast(client->world, livingCamera, reach, tickDelta);

    double reachAlongLook = reach;
    const Vec3d eyePos = livingCamera->getPosition(tickDelta);
    if (client->crosshairTarget.has_value()) {
        reachAlongLook = vec3Distance(client->crosshairTarget->pos, eyePos);
    }

    if (dynamic_cast<TestInteractionManager*>(client->interactionManager.get()) != nullptr) {
        reach = 32.0;
        reachAlongLook = 32.0;
    } else {
        if (reachAlongLook > 3.0) {
            reachAlongLook = 3.0;
        }
        reach = reachAlongLook;
    }

    const Vec3d look = livingCamera->getLookVector(tickDelta);
    const Vec3d end = eyePos + look * reach;

    targetedEntity = nullptr;
    double closest = 0.0;

    const Box queryBox = client->camera->boundingBox
        .stretch(look.x * reach, look.y * reach, look.z * reach)
        .expand(1.0);
    const std::vector<Entity*> entities = client->world->getEntities(client->camera, queryBox);

    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isCollidable()) {
            continue;
        }
        const float margin = entity->getTargetingMargin();
        const Box hitBox = entity->boundingBox.expand(margin);
        const std::optional<HitResult> hit = boxRaycast(hitBox, eyePos, end);
        if (boxContains(hitBox, eyePos)) {
            if (!(0.0 < closest) && closest != 0.0) {
                continue;
            }
            targetedEntity = entity;
            closest = 0.0;
            continue;
        }
        if (!hit.has_value()) {
            continue;
        }
        const double dist = vec3Distance(eyePos, hit->pos);
        if (!(dist < closest) && closest != 0.0) {
            continue;
        }
        targetedEntity = entity;
        closest = dist;
    }

    if (targetedEntity != nullptr
        && dynamic_cast<TestInteractionManager*>(client->interactionManager.get()) == nullptr) {
        client->crosshairTarget = HitResult(targetedEntity, targetedEntity->position());
    }
}

float GameRenderer::getFov(float tickDelta) const
{
    if (client == nullptr || client->camera == nullptr) {
        return 70.0f;
    }
    const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
    if (living == nullptr) {
        return 70.0f;
    }

    float fov = 70.0f;
    if (living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
        fov = 60.0f;
    }
    if (living->health <= 0) {
        const float death = static_cast<float>(living->deathTime) + tickDelta;
        fov /= (1.0f - 500.0f / (death + 500.0f)) * 2.0f + 1.0f;
    }
    fov = option::adjustFieldOfView(fov, option::resolve(client->options));
    return fov + prevCameraRoll + (cameraRoll - prevCameraRoll) * tickDelta;
}

void GameRenderer::applyDamageTiltEffect(float tickDelta)
{
    if (client == nullptr || client->camera == nullptr) {
        return;
    }
    const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
    if (living == nullptr) {
        return;
    }

    float hurt = static_cast<float>(living->hurtTime) - tickDelta;
    if (living->health <= 0) {
        const float death = static_cast<float>(living->deathTime) + tickDelta;
        gl::GL11::glRotatef(40.0f - 8000.0f / (death + 200.0f), 0.0f, 0.0f, 1.0f);
    }
    if (hurt < 0.0f) {
        return;
    }
    hurt /= static_cast<float>(living->damagedTime);
    hurt = MathHelper::sin(hurt * hurt * hurt * hurt * kPiF);
    const float swing = living->damagedSwingDir;
    gl::GL11::glRotatef(-swing, 0.0f, 1.0f, 0.0f);
    gl::GL11::glRotatef(-hurt * 14.0f, 0.0f, 0.0f, 1.0f);
    gl::GL11::glRotatef(swing, 0.0f, 1.0f, 0.0f);
}

void GameRenderer::applyViewBobbing(float tickDelta)
{
    if (client == nullptr) {
        return;
    }
    auto* player = dynamic_cast<PlayerEntity*>(client->camera);
    if (player == nullptr) {
        return;
    }

    const float speedDelta = player->horizontalSpeed - player->prevHorizontalSpeed;
    const float phase = -(player->horizontalSpeed + speedDelta * tickDelta);
    const float stepBob = player->prevStepBobbingAmount
        + (player->stepBobbingAmount - player->prevStepBobbingAmount) * tickDelta;
    const float tiltBob = player->prevTilt + (player->tilt - player->prevTilt) * tickDelta;

    gl::GL11::glTranslatef(MathHelper::sin(phase * kPiF) * stepBob * 0.5f,
        -std::abs(MathHelper::cos(phase * kPiF) * stepBob),
        0.0f);
    gl::GL11::glRotatef(MathHelper::sin(phase * kPiF) * stepBob * 3.0f, 0.0f, 0.0f, 1.0f);
    gl::GL11::glRotatef(std::abs(MathHelper::cos(phase * kPiF - 0.2f) * stepBob) * 5.0f, 1.0f, 0.0f, 0.0f);
    gl::GL11::glRotatef(tiltBob, 1.0f, 0.0f, 0.0f);
}

void GameRenderer::applyCameraTransform(float tickDelta)
{
    if (client == nullptr || client->camera == nullptr) {
        return;
    }
    auto* living = dynamic_cast<LivingEntity*>(client->camera);
    if (living == nullptr) {
        return;
    }

    float eyeOffset = living->standingEyeHeight - 1.62f;
    double interpX = living->prevX + (living->x - living->prevX) * static_cast<double>(tickDelta);
    double interpY = living->prevY + (living->y - living->prevY) * static_cast<double>(tickDelta) - static_cast<double>(eyeOffset);
    double interpZ = living->prevZ + (living->z - living->prevZ) * static_cast<double>(tickDelta);

    gl::GL11::glRotatef(prevCameraRollAmount + (cameraRollAmount - prevCameraRollAmount) * tickDelta, 0.0f, 0.0f, 1.0f);

    if (living->isSleeping()) {
        eyeOffset += 1.0f;
        gl::GL11::glTranslatef(0.0f, 0.3f, 0.0f);
        if (!client->options.debugCamera && client->world != nullptr) {
            const int blockId = client->world->getBlockId(
                MathHelper::floor(living->x),
                MathHelper::floor(living->y),
                MathHelper::floor(living->z));
            if (blockId == kBedBlockId) {
                const int meta = client->world->getBlockMeta(
                    MathHelper::floor(living->x),
                    MathHelper::floor(living->y),
                    MathHelper::floor(living->z));
                const int facing = static_cast<int>(meta) & 3;
                gl::GL11::glRotatef(static_cast<float>(facing) * 90.0f, 0.0f, 1.0f, 0.0f);
            }
            gl::GL11::glRotatef(living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, -1.0f, 0.0f);
            gl::GL11::glRotatef(living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, -1.0f, 0.0f, 0.0f);
        }
    } else if (client->options.thirdPerson) {
        double camDist = prevThirdPersonDistance + (thirdPersonDistance - prevThirdPersonDistance) * static_cast<double>(tickDelta);
        if (client->options.debugCamera) {
            const float dbgYaw = prevThirdPersonYaw + (thirdPersonYaw - prevThirdPersonYaw) * tickDelta;
            const float dbgPitch = prevThirdPersonPitch + (thirdPersonPitch - prevThirdPersonPitch) * tickDelta;
            gl::GL11::glTranslatef(0.0f, 0.0f, static_cast<float>(-camDist));
            gl::GL11::glRotatef(dbgPitch, 1.0f, 0.0f, 0.0f);
            gl::GL11::glRotatef(dbgYaw, 0.0f, 1.0f, 0.0f);
        } else if (client->world != nullptr) {
            const float baseYaw = living->yaw;
            const float basePitch = living->pitch;
            double offsetX = static_cast<double>(-MathHelper::sin(baseYaw / 180.0f * kPiF) * MathHelper::cos(basePitch / 180.0f * kPiF)) * camDist;
            double offsetZ = static_cast<double>(MathHelper::cos(baseYaw / 180.0f * kPiF) * MathHelper::cos(basePitch / 180.0f * kPiF)) * camDist;
            double offsetY = static_cast<double>(-MathHelper::sin(basePitch / 180.0f * kPiF)) * camDist;

            for (int corner = 0; corner < 8; ++corner) {
                float sx = static_cast<float>((corner & 1) * 2 - 1);
                float sy = static_cast<float>((corner >> 1 & 1) * 2 - 1);
                float sz = static_cast<float>((corner >> 2 & 1) * 2 - 1);
                sx *= 0.1f;
                sy *= 0.1f;
                sz *= 0.1f;

                const Vec3d rayStart {
                    interpX + static_cast<double>(sx),
                    interpY + static_cast<double>(sy),
                    interpZ + static_cast<double>(sz)};
                const Vec3d rayEnd {
                    interpX - offsetX + static_cast<double>(sx) + static_cast<double>(sz),
                    interpY - offsetY + static_cast<double>(sy),
                    interpZ - offsetZ + static_cast<double>(sz)};

                const std::optional<HitResult> hit = worldRaycast(client->world, rayStart, rayEnd);
                if (!hit.has_value()) {
                    continue;
                }
                const double dist = vec3Distance(hit->pos, Vec3d {interpX, interpY, interpZ});
                if (dist < camDist) {
                    camDist = dist;
                }
            }

            gl::GL11::glRotatef(living->pitch - basePitch, 1.0f, 0.0f, 0.0f);
            gl::GL11::glRotatef(living->yaw - baseYaw, 0.0f, 1.0f, 0.0f);
            gl::GL11::glTranslatef(0.0f, 0.0f, static_cast<float>(-camDist));
            gl::GL11::glRotatef(baseYaw - living->yaw, 0.0f, 1.0f, 0.0f);
            gl::GL11::glRotatef(basePitch - living->pitch, 1.0f, 0.0f, 0.0f);
        }
    } else {
        gl::GL11::glTranslatef(0.0f, 0.0f, -0.1f);
    }

    if (!client->options.debugCamera) {
        gl::GL11::glRotatef(living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, 1.0f, 0.0f, 0.0f);
        gl::GL11::glRotatef(living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, 1.0f, 0.0f);
    }

    gl::GL11::glTranslatef(0.0f, eyeOffset, 0.0f);

    interpX = living->prevX + (living->x - living->prevX) * static_cast<double>(tickDelta);
    interpY = living->prevY + (living->y - living->prevY) * static_cast<double>(tickDelta) - static_cast<double>(eyeOffset);
    interpZ = living->prevZ + (living->z - living->prevZ) * static_cast<double>(tickDelta);

}

void GameRenderer::renderWorld(float tickDelta, int eye)
{
    if (client == nullptr) {
        return;
    }

    viewDistance = net::minecraft::client::option::resolve(client->options).viewDistanceBlocks;
    const int viewportWidth =
        util::uiFramebufferWidth(client->options, client->displayWidth);
    const float aspect =
        static_cast<float>(viewportWidth) / static_cast<float>(client->displayHeight);

    gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
    gl::GL11::glLoadIdentity();

    if (client->options.isStereoActive()) {
        platform::StereoRendering::applyProjectionStereoOffset(eye, client->options);
    }

    if (zoom != 1.0) {
        gl::GL11::glTranslatef(static_cast<float>(zoomX), static_cast<float>(-zoomY), 0.0f);
        gl::GL11::glScaled(zoom, zoom, 1.0);
        gluPerspectiveFov(getFov(tickDelta), aspect, 0.05f, viewDistance * 2.0f);
    } else {
        gluPerspectiveFov(getFov(tickDelta), aspect, 0.05f, viewDistance * 2.0f);
    }

    gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
    gl::GL11::glLoadIdentity();

    if (client->options.isStereoActive()) {
        platform::StereoRendering::applyModelViewStereoOffset(eye, client->options);
    }

    applyDamageTiltEffect(tickDelta);
    if (client->options.bobView) {
        applyViewBobbing(tickDelta);
    }

    if (client->player != nullptr) {
        const float distortion = client->player->lastScreenDistortion
            + (client->player->screenDistortion - client->player->lastScreenDistortion) * tickDelta;
        if (distortion > 0.0f) {
            float scale = 5.0f / (distortion * distortion + 5.0f) - distortion * 0.04f;
            scale *= scale;
            gl::GL11::glRotatef((static_cast<float>(ticks) + tickDelta) * 20.0f, 0.0f, 1.0f, 1.0f);
            gl::GL11::glScalef(1.0f / scale, 1.0f, 1.0f);
            gl::GL11::glRotatef(-((static_cast<float>(ticks) + tickDelta) * 20.0f), 0.0f, 1.0f, 1.0f);
        }
    }

    applyCameraTransform(tickDelta);
}

void GameRenderer::renderFirstPersonHand(float tickDelta, int eye)
{
    if (client == nullptr || heldItemRenderer == nullptr) {
        return;
    }

    gl::GL11::glLoadIdentity();
    if (client->options.isStereoActive()) {
        gl::GL11::glTranslatef(
            static_cast<float>(eye * 2 - 1) * client->options.ofHandStereoSeparation,
            0.0f,
            client->options.ofHandDepth);
        gl::GL11::glTranslatef(
            static_cast<float>(-(eye * 2 - 1)) * client->options.ofHandStereoOffset,
            0.0f,
            0.0f);
    }

    gl::GL11::glPushMatrix();
    applyDamageTiltEffect(tickDelta);
    if (client->options.bobView) {
        applyViewBobbing(tickDelta);
    }

    auto* living = dynamic_cast<LivingEntity*>(client->camera);
    if (living != nullptr && !client->options.thirdPerson && !living->isSleeping() && !client->options.hideHud) {
        heldItemRenderer->render(tickDelta);
    }

    gl::GL11::glPopMatrix();

    if (living != nullptr && !client->options.thirdPerson && !living->isSleeping()) {
        heldItemRenderer->renderScreenOverlays(tickDelta);
        applyDamageTiltEffect(tickDelta);
    }
    if (client->options.bobView) {
        applyViewBobbing(tickDelta);
    }
}

void GameRenderer::onFrameUpdate(float tickDelta)
{
    if (client == nullptr) {
        return;
    }

#ifdef _WIN32
    if (!util::DisplayManager::isActive()) {
#else
    if (!client->focused.load()) {
#endif
        if (nowMillis() - lastInactiveTime > 500) {
            client->pauseGame();
        }
    } else {
        lastInactiveTime = nowMillis();
    }

    if (client->focused && client->player != nullptr) {
        input::InputSystem::instance().pollMouseLook();
        const float sensitivity = client->options.mouseSensitivity * 0.6f + 0.2f;
        const float scale = sensitivity * sensitivity * sensitivity * 8.0f;
        float deltaYaw = static_cast<float>(input::InputSystem::instance().mouseLookDeltaX()) * scale;
        float deltaPitch = static_cast<float>(input::InputSystem::instance().mouseLookDeltaY()) * scale;
        int invert = 1;
        if (client->options.invertYMouse) {
            invert = -1;
        }
        if (client->options.cinematicMode) {
            deltaYaw = cinematicCameraYawSmoother.smooth(deltaYaw, 0.05f * scale);
            deltaPitch = cinematicCameraPitchSmoother.smooth(deltaPitch, 0.05f * scale);
        } else if (option::resolve(client->options).smoothInput) {
            deltaYaw = yawSmoother.smooth(deltaYaw, 0.15f * scale);
            deltaPitch = pitchSmoother.smooth(deltaPitch, 0.15f * scale);
        }
        client->player->changeLookDirection(deltaYaw, deltaPitch * static_cast<float>(invert));
    }

    if (client->skipGameRender) {
        return;
    }

    int fpsCap = 200;
    if (client->options.fpsLimit == 1) {
        fpsCap = 120;
    } else if (client->options.fpsLimit == 2) {
        fpsCap = 40;
    }

    if (client->world != nullptr) {
        if (client->options.fpsLimit == 0) {
            renderFrame(tickDelta, 0);
        } else {
            renderFrame(tickDelta, lastFrameTime + (1'000'000'000LL / fpsCap));
        }

        throttleAndTimestamp(fpsCap);

        if (!client->options.hideHud || client->currentScreen() != nullptr) {
            const bool chatOpen =
                dynamic_cast<gui::screen::ChatScreen*>(client->currentScreen()) != nullptr;
            platform::StereoUiFrame ui(client->options, client->displayWidth, client->displayHeight);
            ui.forEachEye([&](const platform::StereoUiEyeContext& ctx) {
                ctx.setupHudProjection();
                client->inGameHud.render(tickDelta, chatOpen, ctx.scaledMouseX, ctx.scaledMouseY);
            });
        }
    } else {
        gl::GL11::glViewport(0, 0, client->displayWidth, client->displayHeight);
        gl::GL11::glClear(gl::GL11::GL_COLOR_BUFFER_BIT | gl::GL11::GL_DEPTH_BUFFER_BIT);
        gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
        gl::GL11::glLoadIdentity();
        gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
        gl::GL11::glLoadIdentity();
        setupHudRender();

        throttleAndTimestamp(fpsCap);
    }

    if (client->currentScreen() != nullptr) {
        platform::StereoUiFrame ui(client->options, client->displayWidth, client->displayHeight);
        ui.forEachEye([&](const platform::StereoUiEyeContext& ctx) {
            ctx.setupHudProjection();
            client->currentScreen()->render(ctx.scaledMouseX, ctx.scaledMouseY, tickDelta);
        });
    }
}

void GameRenderer::throttleAndTimestamp(int fpsCap)
{
    if (client->options.fpsLimit == 2) {
        std::int64_t sleepNs = (lastFrameTime + (1'000'000'000LL / fpsCap) - nowNanos()) / 1'000'000LL;
        if (option::resolve(client->options).smoothFps) {
            sleepNs = sleepNs * 3 / 4;
        }
        if (sleepNs < 0) {
            sleepNs += 10;
        }
        if (sleepNs > 0 && sleepNs < 500) {
            sleepMillis(sleepNs);
        }
    }
    lastFrameTime = nowNanos();
}

void GameRenderer::renderFrame(float tickDelta, std::int64_t timeNs)
{
    if (client == nullptr) {
        return;
    }

    pipeline::WorldRenderPass worldRenderPass;
    platform::renderStereoFrame(*this, worldRenderPass, tickDelta, timeNs);
}

void GameRenderer::renderRain()
{
    if (client == nullptr || client->world == nullptr || client->camera == nullptr) {
        return;
    }

    float rain = worldRainGradient(client, client->world, 1.0f);
    if (!client->options.fancyGraphics) {
        rain /= 2.0f;
    }
    if (rain == 0.0f) {
        return;
    }

    random.setSeed(static_cast<std::uint64_t>(ticks) * 312987231ULL);
    const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
    if (living == nullptr) {
        return;
    }

    World* world = client->world;
    const int baseX = MathHelper::floor(living->x);
    const int baseY = MathHelper::floor(living->y);
    const int baseZ = MathHelper::floor(living->z);
    constexpr int radius = 10;

    double soundX = 0.0;
    double soundY = 0.0;
    double soundZ = 0.0;
    int rainSamples = 0;

    BiomeSource* biomeSource = world->getBiomeSource();

    const int particleCount = static_cast<int>(100.0f * rain * rain);
    for (int i = 0; i < particleCount; ++i) {
        const int px = baseX + random.nextInt(radius) - random.nextInt(radius);
        const int pz = baseZ + random.nextInt(radius) - random.nextInt(radius);
        const int topY = world->getTopSolidBlockY(px, pz);
        if (topY < 0) {
            continue;
        }
        const int belowId = world->getBlockId(px, topY - 1, pz);
        if (topY > baseY + radius || topY < baseY - radius) {
            continue;
        }
        if (biomeSource == nullptr || !Biomes::byId(biomeSource->getBiome(px, pz).id).canRain()) {
            continue;
        }
        if (belowId <= 0) {
            continue;
        }

        Block* block = Block::BLOCKS[static_cast<std::size_t>(belowId)];
        if (block == nullptr) {
            continue;
        }

        const float rx = random.nextFloat();
        const float rz = random.nextFloat();
        const double py = static_cast<double>(static_cast<float>(topY) + 0.1f) - block->minY;

        if (&block->material == &::net::minecraft::block::material::Material::LAVA) {
            client->particleManager.addParticle(new client::particle::FireSmokeParticle(
                world, static_cast<float>(px) + rx, py, static_cast<float>(pz) + rz, 0.0, 0.0, 0.0));
            continue;
        }

        if (random.nextInt(++rainSamples) == 0) {
            soundX = static_cast<float>(px) + rx;
            soundY = py;
            soundZ = static_cast<float>(pz) + rz;
        }
        client->particleManager.addParticle(new client::particle::RainSplashParticle(
            world, static_cast<float>(px) + rx, py, static_cast<float>(pz) + rz));
    }

    if (rainSamples > 0 && random.nextInt(3) < rainSoundCounter++) {
        rainSoundCounter = 0;
        if (client->world != nullptr) {
            if (soundY > living->y + 1.0
                && world->getTopSolidBlockY(MathHelper::floor(living->x), MathHelper::floor(living->z))
                    > MathHelper::floor(living->y)) {
                client->world->playSound(soundX, soundY, soundZ, "ambient.weather.rain", 0.1f, 0.5f);
            } else {
                client->world->playSound(soundX, soundY, soundZ, "ambient.weather.rain", 0.2f, 1.0f);
            }
        }
    }
}

void GameRenderer::setupHudRender()
{
    if (client == nullptr) {
        return;
    }
    const util::UiScale scale = util::uiScale(
        client->options,
        util::uiFramebufferWidth(client->options, client->displayWidth),
        client->displayHeight);
    platform::GuiGlState::setupHudProjection(scale.rawWidth, scale.rawHeight);
}

} // namespace net::minecraft::client::render
