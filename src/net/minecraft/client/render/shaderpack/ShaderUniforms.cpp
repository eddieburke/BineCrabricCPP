#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::render::shaderpack {
namespace {
void uploadGbufferCameraMatrices(const gl::ShaderProgram& program, const float* modelView, const float* projection,
                                 const float* modelViewInverse, const float* projectionInverse) {
 program.setMatrix4At(program.location("gbufferModelView"), modelView);
 program.setMatrix4At(program.location("gbufferProjection"), projection);
 program.setMatrix4At(program.location("gbufferModelViewInverse"), modelViewInverse);
 program.setMatrix4At(program.location("gbufferProjectionInverse"), projectionInverse);
}
void uploadDrawMatrixAliases(const gl::ShaderProgram& program, const float* modelView, const float* projection,
                             const float* modelViewInverse, const float* projectionInverse) {
 program.setMatrix4At(program.location("modelViewMatrix"), modelView);
 program.setMatrix4At(program.location("projectionMatrix"), projection);
 program.setMatrix4At(program.location("modelViewMatrixInverse"), modelViewInverse);
 program.setMatrix4At(program.location("projectionMatrixInverse"), projectionInverse);
 net::minecraft::util::math::Matrix4f mv;
 mv.set(modelView);
 net::minecraft::util::math::Matrix4f proj;
 proj.set(projection);
 const net::minecraft::util::math::Matrix4f mvp = proj * mv;
 net::minecraft::util::math::Matrix4f mvpInverse = mvp;
 mvpInverse.invert();
 program.setMatrix4At(program.location("modelViewProjectionMatrix"), mvp.data());
 program.setMatrix4At(program.location("modelViewProjectionMatrixInverse"), mvpInverse.data());
 const float normal[9] = {modelViewInverse[0], modelViewInverse[4], modelViewInverse[8],
                          modelViewInverse[1], modelViewInverse[5], modelViewInverse[9],
                          modelViewInverse[2], modelViewInverse[6], modelViewInverse[10]};
 program.setMatrix3At(program.location("normalMatrix"), normal);
 static const float kIdentity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
 program.setMatrix4At(program.location("textureMatrix"), kIdentity);
}
} // namespace
void uploadGeometryDrawMatrices(const gl::ShaderProgram& program, const float* modelView, const float* projection,
                                const float* modelViewInverse, const float* projectionInverse) {
 uploadDrawMatrixAliases(program, modelView, projection, modelViewInverse, projectionInverse);
}
void uploadIdentityDrawMatrices(const gl::ShaderProgram& program) {
 static const float kIdentity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
 static const float kIdentityNormal[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
 uploadDrawMatrixAliases(program, kIdentity, kIdentity, kIdentity, kIdentity);
 program.setMatrix3At(program.location("normalMatrix"), kIdentityNormal);
}
void uploadShaderUniforms(const gl::ShaderProgram& program, const ShaderUniformValues& values,
                          bool uploadGbufferCamera) {
 program.set1f("frameTimeCounter", values.frameTimeCounter);
 program.set1f("frameTime", values.frameTime);
 program.set1i("frameCounter", values.frameCounter);
 program.set1f("viewWidth", values.viewWidth);
 program.set1f("viewHeight", values.viewHeight);
 program.set1f("aspectRatio", values.aspectRatio);
 program.set1f("near", values.nearPlane);
 program.set1f("far", values.farPlane);
 program.set1f("shadowMapResolution", values.shadowMapResolution);
 program.set3fAt(program.location("cameraPosition"), values.cameraPosition);
 program.set3fAt(program.location("cameraPositionFract"), values.cameraPositionFract);
 program.set3iAt(program.location("cameraPositionInt"), values.cameraPositionInt);
 program.set3fAt(program.location("previousCameraPosition"), values.previousCameraPosition);
 program.set3fAt(program.location("previousCameraPositionFract"), values.previousCameraPositionFract);
 program.set3iAt(program.location("previousCameraPositionInt"), values.previousCameraPositionInt);
 program.set3fAt(program.location("sunPosition"), values.sunPosition);
 program.set3fAt(program.location("moonPosition"), values.moonPosition);
 program.set3fAt(program.location("shadowLightPosition"), values.shadowLightPosition);
 program.set3fAt(program.location("upPosition"), values.upPosition);
 if(uploadGbufferCamera) {
  uploadGbufferCameraMatrices(program, values.gbufferModelView, values.gbufferProjection,
                              values.gbufferModelViewInverse, values.gbufferProjectionInverse);
  program.setMatrix4At(program.location("gbufferPreviousProjection"), values.gbufferPreviousProjection);
  program.setMatrix4At(program.location("gbufferPreviousModelView"), values.gbufferPreviousModelView);
 }
 program.setMatrix4At(program.location("shadowModelView"), values.shadowModelView);
 program.setMatrix4At(program.location("shadowModelViewInverse"), values.shadowModelViewInverse);
 program.setMatrix4At(program.location("shadowProjection"), values.shadowProjection);
 program.setMatrix4At(program.location("shadowProjectionInverse"), values.shadowProjectionInverse);
 program.set3fAt(program.location("sunColor"), values.sunColor);
 program.set1f("sunIntensity", values.sunIntensity);
 program.set3fAt(program.location("fogColor"), values.fogColor);
 program.set1f("fogDensity", values.fogDensity);
 program.set1f("fogStart", values.fogStart);
 program.set1f("fogEnd", values.fogEnd);
 program.set1i("fogMode", values.fogMode);
 program.set1i("fogShape", values.fogShape);
 program.set3fAt(program.location("skyColor"), values.skyColor);
 program.set4fAt(program.location("lightningBoltPosition"), values.lightningBoltPosition);
 program.set1f("thunderStrength", values.thunderStrength);
 program.set1f("currentPlayerHealth", values.currentPlayerHealth);
 program.set1f("maxPlayerHealth", values.maxPlayerHealth);
 program.set1f("currentPlayerAir", values.currentPlayerAir);
 program.set1f("maxPlayerAir", values.maxPlayerAir);
 program.set1f("currentPlayerHunger", values.currentPlayerHunger);
 program.set1f("maxPlayerHunger", values.maxPlayerHunger);
 program.set1f("currentPlayerArmor", values.currentPlayerArmor);
 program.set1f("maxPlayerArmor", values.maxPlayerArmor);
 program.set3fAt(program.location("eyePosition"), values.eyePosition);
 program.set3fAt(program.location("relativeEyePosition"), values.relativeEyePosition);
 program.set3fAt(program.location("playerLookVector"), values.playerLookVector);
 program.set3fAt(program.location("playerBodyVector"), values.playerBodyVector);
 program.set3fAt(program.location("vehicleLookVector"), values.vehicleLookVector);
 program.set3fAt(program.location("relativeVehiclePosition"), values.relativeVehiclePosition);
 program.set1f("ambientLight", values.ambientLight);
 program.set1f("cloudHeight", values.cloudHeight);
 program.set1f("rainfall", values.rainfall);
 program.set1f("temperature", values.temperature);
 program.set1f("rainStrength", values.rainStrength);
 program.set1f("wetness", values.wetness);
 program.set1f("eyeAltitude", values.eyeAltitude);
 program.set2iAt(program.location("eyeBrightness"), values.eyeBrightness);
 program.set2iAt(program.location("eyeBrightnessSmooth"), values.eyeBrightnessSmooth);
 program.set1f("centerDepthSmooth", values.centerDepthSmooth);
 program.set1f("nightVision", values.nightVision);
 program.set1f("blindness", values.blindness);
 program.set1f("darknessFactor", values.darknessFactor);
 program.set1f("darknessLightFactor", values.darknessLightFactor);
 program.set1f("playerMood", values.playerMood);
 program.set1f("constantMood", values.constantMood);
 program.set1f("screenBrightness", values.screenBrightness);
 program.set1f("sunAngle", values.sunAngle);
 program.set1f("shadowAngle", values.shadowAngle);
 program.set3fAt(program.location("endFlashPosition"), values.endFlashPosition);
 program.set1f("endFlashIntensity", values.endFlashIntensity);
 program.set1f("previousEndFlashIntensity", values.previousEndFlashIntensity);
 program.set1i("firstPersonCamera", values.firstPersonCamera);
 program.set1i("isSpectator", values.isSpectator);
 program.set1i("isRightHanded", values.isRightHanded);
 program.set1i("bedrockLevel", values.bedrockLevel);
 program.set1i("heightLimit", values.heightLimit);
 program.set1i("hasCeiling", values.hasCeiling);
 program.set1i("hasSkylight", values.hasSkylight);
 program.set1i("logicalHeightLimit", values.logicalHeightLimit);
 program.set1i("is_sneaking", values.isSneaking);
 program.set1i("is_sprinting", values.isSprinting);
 program.set1i("is_hurt", values.isHurt);
 program.set1i("is_invisible", values.isInvisible);
 program.set1i("is_burning", values.isBurning);
 program.set1i("is_on_ground", values.isOnGround);
 program.set1i("isRiding", values.isRiding);
 program.set1i("vehicleInWater", values.vehicleInWater);
 program.set1i("inSwimmingAnimation", values.inSwimmingAnimation);
 program.set1i("feetInWater", values.feetInWater);
 program.set1i("isElytraFlying", values.isElytraFlying);
 program.set1i("hideGUI", values.hideGUI);
 program.set1i("currentColorSpace", values.currentColorSpace);
 program.set1i("textureFilteringMode", values.textureFilteringMode);
 program.set1i("biome", values.biome);
 program.set1i("biome_category", values.biomeCategory);
 program.set1i("biome_precipitation", values.biomePrecipitation);
 program.set3iAt(program.location("currentDate"), values.currentDate);
 program.set3iAt(program.location("currentTime"), values.currentTime);
 program.set2iAt(program.location("currentYearTime"), values.currentYearTime);
 program.set1i("currentRenderedItemId", values.currentRenderedItemId);
 program.set1i("currentSelectedBlockId", values.currentSelectedBlockId);
 program.set3fAt(program.location("currentSelectedBlockPos"), values.currentSelectedBlockPos);
 program.set1i("heldItemId", values.heldItemId);
 program.set1i("heldItemId2", values.heldItemId2);
 program.set1i("heldBlockLightValue", values.heldBlockLightValue);
 program.set1i("heldBlockLightValue2", values.heldBlockLightValue2);
 program.set1i("vehicleId", values.vehicleId);
 program.set1i("worldTime", values.worldTime);
 program.set1i("worldDay", values.worldDay);
 program.set1i("moonPhase", values.moonPhase);
 program.set1i("isEyeInWater", values.isEyeInWater);
 program.set1i("shadowAvailable", values.shadowAvailable);
 program.set1i("normalAvailable", values.normalAvailable);
}
} // namespace net::minecraft::client::render::shaderpack
