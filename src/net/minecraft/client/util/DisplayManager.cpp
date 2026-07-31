#include "net/minecraft/client/util/DisplayManager.hpp"
#include <iostream>
#include <thread>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#ifdef _WIN32
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/platform/glfw/Window.hpp"
#endif
namespace net::minecraft::client::util {
namespace diagnostics = net::minecraft::client::diagnostics;
namespace {
#ifdef _WIN32
gl::SwapPacing desiredSwapPacing_ = gl::SwapPacing::VSync;
#endif
void clampPositive(int& width, int& height) {
 if(width <= 0) {
  width = 1;
 }
 if(height <= 0) {
  height = 1;
 }
}
} // namespace
#ifdef _WIN32
namespace display = net::minecraft::client::platform::glfw;
namespace {
void syncClientSizeFromWindow(Minecraft& client) {
 const display::DisplayMode mode = display::Window::getDisplayMode();
 client.displayWidth = mode.getWidth();
 client.displayHeight = mode.getHeight();
 clampPositive(client.displayWidth, client.displayHeight);
}
void relockCursorIfFocused(Minecraft& client) {
 if(client.focused.load()) {
  input::InputSystem::instance().lockCursor();
 }
}
} // namespace
#endif
void DisplayManager::setupAndCreateDisplay(Minecraft& client) {
#ifdef _WIN32
 diagnostics::setStartupPhase("init: display");
 display::Window::setResizeCallback([&client](int width, int height) { resize(client, width, height); });
 display::Window::setDeactivateCallback([&client]() { client.unlockMouse(); });
 if(client.canvas != nullptr) {
  display::Window::setParent(client.canvas);
 } else if(client.fullscreen) {
  const display::DisplayMode mode = display::Window::getDesktopDisplayMode();
  client.displayWidth = mode.getWidth();
  client.displayHeight = mode.getHeight();
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
 try {
  display::Window::create();
 } catch(...) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  display::Window::create();
 }
 ensureGlContext();
 gl::GLCore::ensureLoaded();
 syncClientSizeFromWindow(client);
#endif
}
void DisplayManager::logGlError(Minecraft& client, const std::string& phase) {
 (void)client;
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
void DisplayManager::scheduleScreenResize(Minecraft& client) {
 client.pendingScreenResize_ = true;
}
void DisplayManager::toggleFullscreen(Minecraft& client) {
 try {
  client.fullscreen = !client.fullscreen;
#ifdef _WIN32
  display::DisplayMode targetMode;
  if(client.fullscreen) {
   targetMode = display::Window::getDesktopDisplayMode();
  } else {
   targetMode.width = client.initWidth;
   targetMode.height = client.initHeight;
  }
  clampPositive(targetMode.width, targetMode.height);
  client.displayWidth = targetMode.width;
  client.displayHeight = targetMode.height;
  display::Window::setDisplayMode(targetMode);
  display::Window::setFullscreen(client.fullscreen);
  pumpAndPresent();
  syncClientSizeFromWindow(client);
  if(client.currentScreen() != nullptr) {
   resize(client, client.displayWidth, client.displayHeight);
  }
  relockCursorIfFocused(client);
#endif
 } catch(const std::exception& exception) {
  (void)exception;
 }
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
#ifdef _WIN32
 relockCursorIfFocused(client);
#endif
}
#ifdef _WIN32
void DisplayManager::ensureGlContext() {
 display::Window::ensureGlContext();
}
void DisplayManager::setSwapPacing(gl::SwapPacing pacing) {
 desiredSwapPacing_ = pacing;
}
void DisplayManager::present() {
 gl::GLCore::setSwapPacing(desiredSwapPacing_);
 display::Window::present();
}
void DisplayManager::pumpAndPresent() {
 display::Window::pumpMessages();
 present();
}
HWND DisplayManager::hwnd() {
 return display::Window::hwnd();
}
bool DisplayManager::isActive() {
 return display::Window::isActive();
}
bool DisplayManager::isCloseRequested() {
 return display::Window::isCloseRequested();
}
void DisplayManager::destroy() {
 display::Window::destroy();
}
#endif
} // namespace net::minecraft::client::util
