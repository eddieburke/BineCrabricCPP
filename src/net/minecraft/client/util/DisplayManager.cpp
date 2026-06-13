#include "net/minecraft/client/util/DisplayManager.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/gl/GlExtensions.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/util/UiScale.hpp"

#include <iostream>
#include <thread>

#ifdef _WIN32
#include "net/minecraft/client/lifecycle/ClientInitializer.hpp"
#ifdef MINECRAFT_USE_GLFW
#include "net/minecraft/client/platform/glfw/Window.hpp"
#else
#include "net/minecraft/client/platform/win32/Window.hpp"
#endif
#endif

namespace net::minecraft::client::util {

namespace lifecycle = net::minecraft::client::lifecycle;
#ifdef _WIN32
#ifdef MINECRAFT_USE_GLFW
namespace display = net::minecraft::client::platform::glfw;
#else
namespace display = net::minecraft::client::platform::win32;
#endif
#endif

void DisplayManager::setupAndCreateDisplay(Minecraft& client)
{
#ifdef _WIN32
    lifecycle::setStartupPhase("init: display");
    display::Window::setResizeCallback([&client](int width, int height) {
        resize(client, width, height);
    });
    display::Window::setDeactivateCallback([&client]() {
        client.unlockMouse();
    });
    if (client.canvas != nullptr) {
        display::Window::setParent(client.canvas);
    } else if (client.fullscreen) {
        const display::DisplayMode mode = display::Window::getDesktopDisplayMode();
        client.displayWidth = mode.getWidth();
        client.displayHeight = mode.getHeight();
        if (client.displayWidth <= 0) {
            client.displayWidth = 1;
        }
        if (client.displayHeight <= 0) {
            client.displayHeight = 1;
        }
        display::Window::setDisplayMode(mode);
        display::Window::setFullscreen(true);
    } else {
        display::DisplayMode mode;
        mode.width = client.displayWidth;
        mode.height = client.displayHeight;
        display::Window::setDisplayMode(mode);
    }
    display::Window::setTitle("Minecraft Minecraft Beta 1.7.3");
    try {
        display::Window::create();
    } catch (...) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        display::Window::create();
    }
    ensureGlContext();
    gl::GlExtensions::ensureLoaded();
    gl::GlExtensions::setSwapInterval(0);
    const display::DisplayMode actualMode = display::Window::getDisplayMode();
    client.displayWidth = actualMode.getWidth();
    client.displayHeight = actualMode.getHeight();
    if (client.displayWidth <= 0) {
        client.displayWidth = 1;
    }
    if (client.displayHeight <= 0) {
        client.displayHeight = 1;
    }
#endif
}

void DisplayManager::logGlError(Minecraft& client, const std::string& phase)
{
    (void)client;
    const int error = gl::GL11::glGetError();
    if (error != 0) {
        std::cout << "########## GL ERROR ##########" << std::endl;
        std::cout << "@ " << phase << std::endl;
        std::cout << error << std::endl;
    }
}

void DisplayManager::scheduleScreenResize(Minecraft& client)
{
    client.pendingScreenResize_ = true;
}

void DisplayManager::toggleFullscreen(Minecraft& client)
{
    try {
        client.fullscreen = !client.fullscreen;
#ifdef _WIN32
        display::DisplayMode targetMode;
        if (client.fullscreen) {
            targetMode = display::Window::getDesktopDisplayMode();
        } else {
            targetMode.width = client.initWidth;
            targetMode.height = client.initHeight;
        }
        if (targetMode.width <= 0) {
            targetMode.width = 1;
        }
        if (targetMode.height <= 0) {
            targetMode.height = 1;
        }
        client.displayWidth = targetMode.width;
        client.displayHeight = targetMode.height;
        display::Window::setDisplayMode(targetMode);
        display::Window::setFullscreen(client.fullscreen);
        pumpAndPresent();
        const display::DisplayMode actualMode = display::Window::getDisplayMode();
        client.displayWidth = actualMode.getWidth();
        client.displayHeight = actualMode.getHeight();
        if (client.displayWidth <= 0) {
            client.displayWidth = 1;
        }
        if (client.displayHeight <= 0) {
            client.displayHeight = 1;
        }
        if (client.currentScreen() != nullptr) {
            resize(client, client.displayWidth, client.displayHeight);
        }
        if (client.focused.load()) {
            input::InputSystem::instance().lockCursor();
        }
#endif
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
    }
}

void DisplayManager::resize(Minecraft& client, int width, int height)
{
    if (width <= 0) {
        width = 1;
    }
    if (height <= 0) {
        height = 1;
    }
    client.displayWidth = width;
    client.displayHeight = height;
    gl::GL11::glViewport(0, 0, client.displayWidth, client.displayHeight);
    if (client.currentScreen() != nullptr) {
        const UiScale scale = uiScale(
            client.options, uiFramebufferWidth(client.options, width), height);
        client.currentScreen()->init(&client, scale.scaledWidth, scale.scaledHeight);
    }
#ifdef _WIN32
    if (client.focused.load()) {
        input::InputSystem::instance().lockCursor();
    }
#endif
}

#ifdef _WIN32

void DisplayManager::ensureGlContext()
{
    display::Window::ensureGlContext();
}

void DisplayManager::present()
{
    display::Window::present();
}

void DisplayManager::pumpAndPresent()
{
    display::Window::pumpAndPresent();
}

HWND DisplayManager::hwnd()
{
    return display::Window::hwnd();
}

bool DisplayManager::isActive()
{
    return display::Window::isActive();
}

bool DisplayManager::isCloseRequested()
{
    return display::Window::isCloseRequested();
}

void DisplayManager::destroy()
{
    display::Window::destroy();
}

#endif

} // namespace net::minecraft::client::util
