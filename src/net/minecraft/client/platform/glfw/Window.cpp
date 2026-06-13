#include "net/minecraft/client/platform/glfw/Window.hpp"

#ifdef MINECRAFT_USE_GLFW

#include "net/minecraft/client/input/InputSystem.hpp"

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace net::minecraft::client::platform::glfw {

namespace {

bool fullscreen_ = false;
bool closeRequested_ = false;
bool active_ = true;
bool initialized_ = false;
int pendingWidth_ = 854;
int pendingHeight_ = 480;
int windowedX_ = 100;
int windowedY_ = 100;
int windowedWidth_ = 854;
int windowedHeight_ = 480;
std::string title_ = "Minecraft Beta 1.7.3";
GLFWwindow* window_ = nullptr;
Window::ResizeCallback resizeCallback_ {};
Window::DeactivateCallback deactivateCallback_ {};

int glfwKeyToMinecraftKey(int key)
{
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        return 2 + (key - GLFW_KEY_1);
    }
    if (key == GLFW_KEY_0) {
        return 11;
    }
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        static constexpr int kKeys[] = {
            30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50,
            49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,
        };
        return kKeys[key - GLFW_KEY_A];
    }
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F10) {
        return 59 + (key - GLFW_KEY_F1);
    }
    if (key == GLFW_KEY_F11) {
        return 87;
    }
    if (key == GLFW_KEY_F12) {
        return 88;
    }
    switch (key) {
    case GLFW_KEY_ESCAPE: return 1;
    case GLFW_KEY_MINUS: return 12;
    case GLFW_KEY_EQUAL: return 13;
    case GLFW_KEY_BACKSPACE: return 14;
    case GLFW_KEY_TAB: return 15;
    case GLFW_KEY_LEFT_BRACKET: return 26;
    case GLFW_KEY_RIGHT_BRACKET: return 27;
    case GLFW_KEY_ENTER: return 28;
    case GLFW_KEY_LEFT_CONTROL: return 29;
    case GLFW_KEY_SEMICOLON: return 39;
    case GLFW_KEY_APOSTROPHE: return 40;
    case GLFW_KEY_GRAVE_ACCENT: return 41;
    case GLFW_KEY_LEFT_SHIFT: return 42;
    case GLFW_KEY_BACKSLASH: return 43;
    case GLFW_KEY_COMMA: return 51;
    case GLFW_KEY_PERIOD: return 52;
    case GLFW_KEY_SLASH: return 53;
    case GLFW_KEY_RIGHT_SHIFT: return 54;
    case GLFW_KEY_KP_MULTIPLY: return 55;
    case GLFW_KEY_LEFT_ALT: return 56;
    case GLFW_KEY_SPACE: return 57;
    case GLFW_KEY_CAPS_LOCK: return 58;
    case GLFW_KEY_NUM_LOCK: return 69;
    case GLFW_KEY_SCROLL_LOCK: return 70;
    case GLFW_KEY_RIGHT_CONTROL: return 157;
    case GLFW_KEY_RIGHT_ALT: return 184;
    default: return -1;
    }
}

void cursorPosition(int& x, int& y)
{
    x = 0;
    y = 0;
    if (window_ == nullptr) {
        return;
    }
    double cursorX = 0.0;
    double cursorY = 0.0;
    int width = 0;
    int height = 0;
    glfwGetCursorPos(window_, &cursorX, &cursorY);
    glfwGetWindowSize(window_, &width, &height);
    x = static_cast<int>(std::floor(cursorX));
    y = height - static_cast<int>(std::floor(cursorY));
}

void applyCallbacks(GLFWwindow* window)
{
    glfwSetWindowCloseCallback(window, [](GLFWwindow*) {
        Window::notifyCloseRequested();
    });
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int, int) {
        Window::notifyResize();
    });
    glfwSetWindowFocusCallback(window, [](GLFWwindow*, int focused) {
        Window::setActive(focused == GLFW_TRUE);
    });
    glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int) {
        if (action != GLFW_PRESS && action != GLFW_RELEASE) {
            return;
        }
        const int minecraftKey = glfwKeyToMinecraftKey(key);
        if (minecraftKey >= 0) {
            input::InputSystem::pushKeyEvent(minecraftKey, action == GLFW_PRESS);
        }
    });
    glfwSetCharCallback(window, [](GLFWwindow*, unsigned int codepoint) {
        input::InputSystem::pushCharEvent(static_cast<int>(codepoint));
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, int) {
        if (button < GLFW_MOUSE_BUTTON_LEFT || button > GLFW_MOUSE_BUTTON_MIDDLE) {
            return;
        }
        int x = 0;
        int y = 0;
        cursorPosition(x, y);
        input::InputSystem::pushMouseButtonEvent(button, action == GLFW_PRESS, x, y);
    });
    glfwSetScrollCallback(window, [](GLFWwindow*, double, double yoffset) {
        int x = 0;
        int y = 0;
        cursorPosition(x, y);
        const int delta = static_cast<int>(std::round(yoffset * 120.0));
        input::InputSystem::pushMouseWheelEvent(delta, x, y);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow*, double xpos, double ypos) {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window_, &width, &height);
        input::InputSystem::setCursorPosition(
            static_cast<int>(std::floor(xpos)),
            height - static_cast<int>(std::floor(ypos)));
    });
}

GLFWmonitor* primaryMonitor()
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor == nullptr) {
        throw std::runtime_error("glfwGetPrimaryMonitor failed");
    }
    return monitor;
}

void rememberWindowedPlacement()
{
    if (window_ == nullptr || fullscreen_) {
        return;
    }
    glfwGetWindowPos(window_, &windowedX_, &windowedY_);
    glfwGetWindowSize(window_, &windowedWidth_, &windowedHeight_);
}

} // namespace

void Window::setParent(void* /*canvas*/) {}

void Window::setResizeCallback(ResizeCallback callback)
{
    resizeCallback_ = std::move(callback);
}

void Window::setDeactivateCallback(DeactivateCallback callback)
{
    deactivateCallback_ = std::move(callback);
}

void Window::setFullscreen(bool value)
{
    if (fullscreen_ == value) {
        return;
    }
    if (window_ == nullptr) {
        fullscreen_ = value;
        return;
    }
    if (value) {
        rememberWindowedPlacement();
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor());
        if (mode == nullptr) {
            throw std::runtime_error("glfwGetVideoMode failed");
        }
        const int width = pendingWidth_ > 0 ? pendingWidth_ : mode->width;
        const int height = pendingHeight_ > 0 ? pendingHeight_ : mode->height;
        fullscreen_ = true;
        glfwSetWindowMonitor(window_, primaryMonitor(), 0, 0, width, height, mode->refreshRate);
    } else {
        fullscreen_ = false;
        const int width = windowedWidth_ > 0 ? windowedWidth_ : pendingWidth_;
        const int height = windowedHeight_ > 0 ? windowedHeight_ : pendingHeight_;
        glfwSetWindowMonitor(window_, nullptr, windowedX_, windowedY_, width, height, 0);
    }
    glfwSwapInterval(0);
    notifyResize();
}

void Window::setDisplayMode(const DisplayMode& mode)
{
    pendingWidth_ = mode.width > 0 ? mode.width : pendingWidth_;
    pendingHeight_ = mode.height > 0 ? mode.height : pendingHeight_;
    if (window_ == nullptr) {
        return;
    }
    if (fullscreen_) {
        const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor());
        const int refreshRate = videoMode != nullptr ? videoMode->refreshRate : GLFW_DONT_CARE;
        glfwSetWindowMonitor(window_, primaryMonitor(), 0, 0, pendingWidth_, pendingHeight_, refreshRate);
    } else {
        rememberWindowedPlacement();
        windowedWidth_ = pendingWidth_;
        windowedHeight_ = pendingHeight_;
        glfwSetWindowSize(window_, pendingWidth_, pendingHeight_);
    }
    notifyResize();
}

void Window::setTitle(const char* title)
{
    if (title == nullptr) {
        return;
    }
    title_ = title;
    if (window_ != nullptr) {
        glfwSetWindowTitle(window_, title_.c_str());
    }
}

void Window::notifyResize()
{
    if (window_ == nullptr || !resizeCallback_) {
        return;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }
    pendingWidth_ = width;
    pendingHeight_ = height;
    resizeCallback_(width, height);
}

void Window::notifyCloseRequested()
{
    closeRequested_ = true;
}

void Window::setActive(bool active)
{
    active_ = active;
    if (!active) {
        input::InputSystem::clearOnDeactivate();
        if (deactivateCallback_) {
            deactivateCallback_();
        }
    }
}

void Window::create()
{
    if (!initialized_) {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("glfwInit failed");
        }
        initialized_ = true;
    }
    if (window_ != nullptr) {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    GLFWmonitor* monitor = fullscreen_ ? primaryMonitor() : nullptr;
    if (fullscreen_) {
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor());
        if (mode != nullptr) {
            pendingWidth_ = pendingWidth_ > 0 ? pendingWidth_ : mode->width;
            pendingHeight_ = pendingHeight_ > 0 ? pendingHeight_ : mode->height;
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        }
    }

    window_ = glfwCreateWindow(pendingWidth_, pendingHeight_, title_.c_str(), monitor, nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("glfwCreateWindow failed");
    }
    applyCallbacks(window_);
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(0);
    glfwShowWindow(window_);
    notifyResize();
}

void Window::ensureGlContext()
{
    if (window_ == nullptr) {
        throw std::runtime_error("OpenGL context not created");
    }
    glfwMakeContextCurrent(window_);
}

void Window::destroy()
{
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    if (initialized_) {
        glfwTerminate();
        initialized_ = false;
    }
}

void Window::pumpMessages()
{
    glfwPollEvents();
    if (window_ != nullptr && glfwWindowShouldClose(window_) == GLFW_TRUE) {
        notifyCloseRequested();
    }
}

void Window::present()
{
    if (window_ != nullptr) {
        glfwSwapBuffers(window_);
    }
}

void Window::pumpAndPresent()
{
    pumpMessages();
    input::InputSystem::compactQueues();
    present();
}

bool Window::isCloseRequested()
{
    return closeRequested_;
}

bool Window::isActive()
{
    return active_;
}

bool Window::isFullscreen()
{
    return fullscreen_;
}

DisplayMode Window::getDisplayMode()
{
    if (window_ == nullptr) {
        return {pendingWidth_, pendingHeight_};
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    return {width, height};
}

DisplayMode Window::getDesktopDisplayMode()
{
    if (!initialized_) {
        if (glfwInit() != GLFW_TRUE) {
            return {};
        }
        initialized_ = true;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor());
    if (mode == nullptr) {
        return {};
    }
    return {mode->width, mode->height};
}

#ifdef _WIN32
HWND Window::hwnd()
{
    return window_ != nullptr ? glfwGetWin32Window(window_) : nullptr;
}
#endif

GLFWwindow* Window::window()
{
    return window_;
}

} // namespace net::minecraft::client::platform::glfw

#endif // MINECRAFT_USE_GLFW
