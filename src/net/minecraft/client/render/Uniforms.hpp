#pragma once
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render::shaderpack {
struct ShaderUniformValues {
 float frameTimeCounter = 0.0f;
 float frameTime = 0.0f;
 float viewWidth = 1.0f;
 float viewHeight = 1.0f;
 float aspectRatio = 1.0f;
 float nearPlane = 0.05f;
 float farPlane = 256.0f;
 float shadowMapResolution = 0.0f;
 float cameraPosition[3] = {0.0f, 0.0f, 0.0f};
 float cameraPositionFract[3] = {0.0f, 0.0f, 0.0f};
 int cameraPositionInt[3] = {0, 0, 0};
 float previousCameraPosition[3] = {0.0f, 0.0f, 0.0f};
 float previousCameraPositionFract[3] = {0.0f, 0.0f, 0.0f};
 int previousCameraPositionInt[3] = {0, 0, 0};
 float sunPosition[3] = {0.0f, 1.0f, 0.0f};
 float moonPosition[3] = {0.0f, -1.0f, 0.0f};
 float shadowLightPosition[3] = {0.0f, 1.0f, 0.0f};
 float upPosition[3] = {0.0f, 1.0f, 0.0f};
 float gbufferProjection[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f};
 float gbufferProjectionInverse[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f};
 float gbufferPreviousProjection[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                        0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f,
                                        0.0f, 0.0f, 0.0f, 1.0f};
 float gbufferModelView[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f};
 float gbufferModelViewInverse[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 1.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 1.0f};
 float gbufferPreviousModelView[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f};
 float shadowModelView[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 1.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 1.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f};
 float shadowModelViewInverse[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 1.0f};
 float shadowProjection[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f};
 float shadowProjectionInverse[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 1.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 1.0f};
 float sunColor[3] = {1.0f, 1.0f, 1.0f};
 float sunIntensity = 0.0f;
 float fogColor[3] = {0.0f, 0.0f, 0.0f};
 float fogDensity = 0.0f;
 float fogStart = 0.0f;
 float fogEnd = 0.0f;
 int fogMode = 0;
 int fogShape = 0;
 float skyColor[3] = {0.0f, 0.0f, 0.0f};
 float lightningBoltPosition[4]{};
 float thunderStrength = 0.0f;
 float currentPlayerHealth = 0.0f;
 float maxPlayerHealth = 20.0f;
 float currentPlayerAir = 0.0f;
 float maxPlayerAir = 300.0f;
 float currentPlayerHunger = 1.0f;
 float maxPlayerHunger = 20.0f;
 float currentPlayerArmor = 0.0f;
 float maxPlayerArmor = 20.0f;
 float eyePosition[3]{};
 float relativeEyePosition[3]{};
 float playerLookVector[3] = {0.0f, 0.0f, -1.0f};
 float playerBodyVector[3] = {0.0f, 0.0f, -1.0f};
 float vehicleLookVector[3]{};
 float relativeVehiclePosition[3]{};
 float ambientLight = 0.0f;
 float cloudHeight = 0.0f;
 float rainfall = 0.0f;
 float temperature = 0.5f;
 float rainStrength = 0.0f;
 float wetness = 0.0f;
 float eyeAltitude = 0.0f;
 int eyeBrightness[2] = {0, 0};
 int eyeBrightnessSmooth[2] = {0, 0};
 float centerDepthSmooth = 0.0f;
 float nightVision = 0.0f;
 float blindness = 0.0f;
 float darknessFactor = 0.0f;
 float darknessLightFactor = 0.0f;
 float playerMood = 0.0f;
 float constantMood = 0.0f;
 float screenBrightness = 0.0f;
 float sunAngle = 0.0f;
 float shadowAngle = 0.0f;
 float endFlashPosition[3]{};
 float endFlashIntensity = 0.0f;
 float previousEndFlashIntensity = 0.0f;
 int firstPersonCamera = 1;
 int isSpectator = 0;
 int isRightHanded = 1;
 int bedrockLevel = 0;
 int heightLimit = 128;
 int hasCeiling = 0;
 int hasSkylight = 1;
 int logicalHeightLimit = 128;
 int isSneaking = 0;
 int isSprinting = 0;
 int isHurt = 0;
 int isInvisible = 0;
 int isBurning = 0;
 int isOnGround = 0;
 int isRiding = 0;
 int vehicleInWater = 0;
 int inSwimmingAnimation = 0;
 int feetInWater = 0;
 int isElytraFlying = 0;
 int hideGUI = 0;
 int currentColorSpace = 0;
 int textureFilteringMode = 0;
 int biome = 0;
 int biomeCategory = 0;
 int biomePrecipitation = 0;
 int currentDate[3]{};
 int currentTime[3]{};
 int currentYearTime[2]{};
 int currentRenderedItemId = 0;
 int currentSelectedBlockId = 0;
 float currentSelectedBlockPos[3]{};
 int heldItemId = 0;
 int heldItemId2 = 0;
 int heldBlockLightValue = 0;
 int heldBlockLightValue2 = 0;
 int handLightPackedLR = 0;
 int vehicleId = 0;
 int worldTime = 0;
 int worldDay = 0;
 int moonPhase = 0;
 int frameCounter = 0;
 int isEyeInWater = 0;
 int shadowAvailable = 0;
 int normalAvailable = 0;
};
void uploadShaderUniforms(const gl::ShaderProgram& program, const ShaderUniformValues& values,
                          bool uploadGbufferCamera = true);
void uploadGeometryDrawMatrices(const gl::ShaderProgram& program, const float* modelView, const float* projection,
                                const float* modelViewInverse, const float* projectionInverse);
void uploadIdentityDrawMatrices(const gl::ShaderProgram& program);
using FrameUniformSet = ShaderUniformValues;
} // namespace net::minecraft::client::render::shaderpack

namespace net::minecraft::client::render {
using FrameUniformSet = shaderpack::FrameUniformSet;
using ShaderUniformValues = shaderpack::ShaderUniformValues;
} // namespace net::minecraft::client::render
