#include "net/minecraft/client/render/platform/Lighting.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::client::render::platform {

namespace {

// OpenGL lighting constants from net.minecraft.client.render.platform.Lighting.
constexpr int kGlLighting = 0x0B50;
constexpr int kGlLight0 = 0x4000;
constexpr int kGlLight1 = 0x4001;
constexpr int kGlColorMaterial = 0x0B57;
constexpr int kGlFrontAndBack = 0x0408;
constexpr int kGlAmbientAndDiffuse = 0x1602;
constexpr int kGlPosition = 0x1203;
constexpr int kGlDiffuse = 0x1201;
constexpr int kGlAmbient = 0x1200;
constexpr int kGlSpecular = 0x1202;
constexpr int kGlFlat = 0x1D00;
constexpr int kGlLightModelAmbient = 0x0B53;
constexpr int kGlNormalize = 0x0BA1;
constexpr int kGlFront = 0x0404;

thread_local float lightBuffer[4];

void setLight(int light, int pname, float x, float y, float z, float w)
{
    lightBuffer[0] = x;
    lightBuffer[1] = y;
    lightBuffer[2] = z;
    lightBuffer[3] = w;
    gl::GL11::glLightfv(light, pname, lightBuffer);
}

void configureDirectionalLight(int light, float dirX, float dirY, float dirZ)
{
    const Vec3d direction = Vec3d {static_cast<double>(dirX), static_cast<double>(dirY), static_cast<double>(dirZ)}.normalize();
    constexpr float diffuse = 0.6f;
    constexpr float specular = 0.0f;
    setLight(light, kGlPosition, static_cast<float>(direction.x), static_cast<float>(direction.y),
        static_cast<float>(direction.z), 0.0f);
    setLight(light, kGlDiffuse, diffuse, diffuse, diffuse, 1.0f);
    setLight(light, kGlAmbient, 0.0f, 0.0f, 0.0f, 1.0f);
    setLight(light, kGlSpecular, specular, specular, specular, 1.0f);
}

} // namespace

void Lighting::turnOff() noexcept
{
    gl::GL11::glDisable(kGlLighting);
    gl::GL11::glDisable(kGlLight0);
    gl::GL11::glDisable(kGlLight1);
    gl::GL11::glDisable(kGlColorMaterial);
}

void Lighting::turnOn() noexcept
{
    gl::GL11::glEnable(kGlLighting);
    gl::GL11::glEnable(kGlLight0);
    gl::GL11::glEnable(kGlLight1);
    gl::GL11::glEnable(kGlColorMaterial);
    gl::GL11::glColorMaterial(kGlFrontAndBack, kGlAmbientAndDiffuse);
    configureDirectionalLight(kGlLight0, 0.2f, 1.0f, -0.7f);
    configureDirectionalLight(kGlLight1, -0.2f, 1.0f, 0.7f);
    gl::GL11::glShadeModel(kGlFlat);
    constexpr float ambient = 0.4f;
    lightBuffer[0] = ambient;
    lightBuffer[1] = ambient;
    lightBuffer[2] = ambient;
    lightBuffer[3] = 1.0f;
    gl::GL11::glLightModelfv(kGlLightModelAmbient, lightBuffer);
}

} // namespace net::minecraft::client::render::platform
