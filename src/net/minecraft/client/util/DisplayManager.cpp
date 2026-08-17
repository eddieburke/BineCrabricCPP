#include "net/minecraft/client/util/DisplayManager.hpp"
#include <iostream>
#include <thread>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/platform/glfw/Window.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
namespace net::minecraft::client::util {
namespace diagnostics = net::minecraft::client::diagnostics;
namespace display = net::minecraft::client::platform::glfw;
namespace {
void clampPositive(int& width, int& height) {
 if(width <= 0) {
  width = 1;
 }
 if(height <= 0) {
  height = 1;
 }
}
void syncClientSizeFromWindow(Minecraft& client) {
 const display::DisplayMode mode = display::Window::getDisplayMode();
 client.displayWidth = mode.width;
 client.displayHeight = mode.height;
 clampPositive(client.displayWidth, client.displayHeight);
}
void relockCursorIfFocused(Minecraft& client) {
 if(client.focused.load()) {
  input::InputSystem::instance().lockCursor();
 }
}
} // namespace
void DisplayManager::setupAndCreateDisplay(Minecraft& client) {
 diagnostics::setStartupPhase("init: display");
 display::Window::setResizeCallback([&client](int width, int height) { resize(client, width, height); });
 display::Window::setDeactivateCallback([&client]() { client.unlockMouse(); });
 if(client.fullscreen) {
  const display::DisplayMode mode = display::Window::getDesktopDisplayMode();
  client.displayWidth = mode.width;
  client.displayHeight = mode.height;
  clampPositive(client.displayWidth, client.displayHeight);
  display::Window::setDisplayMode(mode);
  display::Window::setFullscreen(true);
 } else {
  display::DisplayMode mode;
  mode.width = client.displayWidth;
  mode.height = client.displayHeight;
  display::Window::setDisplayMode(mode);
 }
 display::Window::setTitle("Minecraft Beta 1.7.3");
 display::Window::setVisible(!client.headlessMode());
 try {
  display::Window::create();
 } catch(...) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  display::Window::create();
 }
 display::Window::ensureGlContext();
 gl::GLCore::ensureLoaded();
 syncClientSizeFromWindow(client);
}
void DisplayManager::logGlError(const std::string& phase) {
 auto errorName = [](int error) -> const char* {
  switch(error) {
  case 0x0500:
   return "GL_INVALID_ENUM";
  case 0x0501:
   return "GL_INVALID_VALUE";
  case 0x0502:
   return "GL_INVALID_OPERATION";
  case 0x0503:
   return "GL_STACK_OVERFLOW";
  case 0x0504:
   return "GL_STACK_UNDERFLOW";
  case 0x0505:
   return "GL_OUT_OF_MEMORY";
  case 0x0506:
   return "GL_INVALID_FRAMEBUFFER_OPERATION";
  default:
   return "GL_UNKNOWN";
  }
 };
 for(;;) {
  const int error = static_cast<int>(::glGetError());
  if(error == 0) {
   break;
  }
  std::cerr << "########## GL ERROR ##########\n@ " << phase << '\n'
            << error << " (" << errorName(error) << ")\n";
 }
}
void DisplayManager::toggleFullscreen(Minecraft& client) {
 client.fullscreen = !client.fullscreen;
 if(client.fullscreen) {
  display::DisplayMode targetMode = display::Window::getDesktopDisplayMode();
  clampPositive(targetMode.width, targetMode.height);
  display::Window::setFullscreen(true);
  display::Window::setDisplayMode(targetMode);
 } else {
  display::Window::setFullscreen(false);
 }
 display::Window::pumpMessages();
 display::Window::present();
 syncClientSizeFromWindow(client);
 if(client.currentScreen() != nullptr) {
  resize(client, client.displayWidth, client.displayHeight);
 }
 relockCursorIfFocused(client);
}
void DisplayManager::resize(Minecraft& client, int width, int height) {
 clampPositive(width, height);
 client.displayWidth = width;
 client.displayHeight = height;
 render::core::viewport(0, 0, client.displayWidth, client.displayHeight);
 if(client.currentScreen() != nullptr) {
  const UiScale scale = uiScale(client.options, width, height);
  client.currentScreen()->init(&client, scale.scaledWidth, scale.scaledHeight);
 }
 relockCursorIfFocused(client);
}
} // namespace net::minecraft::client::util
