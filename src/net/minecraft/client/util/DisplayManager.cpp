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
#include "net/minecraft/client/platform/win32/Window.hpp"
#endif

namespace net::minecraft::client::util {

namespace lifecycle = net::minecraft::client::lifecycle;
namespace win32 = net::minecraft::client::platform::win32;

void DisplayManager::setupAndCreateDisplay(Minecraft& client)
{
#ifdef _WIN32
    lifecycle::setStartupPhase("init: display");
    win32::Window::setResizeCallback([&client](int width, int height) {
        resize(client, width, height);
    });
    win32::Window::setDeactivateCallback([&client]() {
        client.unlockMouse();
    });
    if (client.canvas != nullptr) {
        win32::Window::setParent(client.canvas);
    } else if (client.fullscreen) {
        const win32::DisplayMode mode = win32::Window::getDesktopDisplayMode();
        client.displayWidth = mode.getWidth();
        client.displayHeight = mode.getHeight();
        if (client.displayWidth <= 0) {
            client.displayWidth = 1;
        }
        if (client.displayHeight <= 0) {
            client.displayHeight = 1;
        }
        win32::Window::setDisplayMode(mode);
        win32::Window::setFullscreen(true);
    } else {
        win32::DisplayMode mode;
        mode.width = client.displayWidth;
        mode.height = client.displayHeight;
        win32::Window::setDisplayMode(mode);
    }
    win32::Window::setTitle("Minecraft Minecraft Beta 1.7.3");
    try {
        win32::Window::create();
    } catch (...) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        win32::Window::create();
    }
    ensureGlContext();
    gl::GlExtensions::ensureLoaded();
    const win32::DisplayMode actualMode = win32::Window::getDisplayMode();
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
        win32::DisplayMode targetMode;
        if (client.fullscreen) {
            targetMode = win32::Window::getDesktopDisplayMode();
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
        win32::Window::setDisplayMode(targetMode);
        win32::Window::setFullscreen(client.fullscreen);
        pumpAndPresent();
        const win32::DisplayMode actualMode = win32::Window::getDisplayMode();
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
    win32::Window::ensureGlContext();
}

void DisplayManager::present()
{
    win32::Window::present();
}

void DisplayManager::pumpAndPresent()
{
    win32::Window::pumpAndPresent();
}

HWND DisplayManager::hwnd()
{
    return win32::Window::hwnd();
}

bool DisplayManager::isActive()
{
    return win32::Window::isActive();
}

bool DisplayManager::isCloseRequested()
{
    return win32::Window::isCloseRequested();
}

#endif

} // namespace net::minecraft::client::util
