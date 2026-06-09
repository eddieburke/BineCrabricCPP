#include "net/minecraft/client/platform/win32/Window.hpp"

#include "net/minecraft/client/input/InputSystem.hpp"

#include <stdexcept>
#include <string>

#ifdef _WIN32

namespace net::minecraft::client::platform::win32 {

namespace {

std::wstring utf8ToWide(const char* text)
{
    if (text == nullptr || text[0] == '\0') {
        return {};
    }
    const int needed = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (needed <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), needed);
    if (!wide.empty() && wide.back() == L'\0') {
        wide.pop_back();
    }
    return wide;
}

DWORD windowedStyleFlags()
{
    return WS_OVERLAPPEDWINDOW;
}

void resizeClientArea(HWND hwnd, int clientWidth, int clientHeight, DWORD style)
{
    if (hwnd == nullptr || clientWidth <= 0 || clientHeight <= 0) {
        return;
    }
    RECT wantClient {0, 0, clientWidth, clientHeight};
    AdjustWindowRect(&wantClient, style, FALSE);
    const int outerWidth = wantClient.right - wantClient.left;
    const int outerHeight = wantClient.bottom - wantClient.top;
    SetWindowPos(hwnd, nullptr, 0, 0, outerWidth, outerHeight,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

bool fullscreen_ = false;
bool closeRequested_ = false;
bool active_ = true;
int pendingWidth_ = 854;
int pendingHeight_ = 480;
HWND hwnd_ = nullptr;
HDC deviceContext_ = nullptr;
HGLRC glContext_ = nullptr;
Window::ResizeCallback resizeCallback_ {};
Window::DeactivateCallback deactivateCallback_ {};
bool hasSavedWindowedState_ = false;
WINDOWPLACEMENT windowedPlacement_ {};
LONG windowedStyle_ = 0;
LONG windowedExStyle_ = 0;

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CLOSE) {
        Window::notifyCloseRequested();
        return 0;
    }
    if (msg == WM_SIZE) {
        if (wParam != SIZE_MINIMIZED) {
            Window::notifyResize();
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    if (msg == WM_ACTIVATE || msg == WM_ACTIVATEAPP) {
        const bool becomingActive = (msg == WM_ACTIVATE)
            ? (LOWORD(wParam) != WA_INACTIVE)
            : (wParam != FALSE);
        Window::setActive(becomingActive);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    const LRESULT inputResult = input::InputSystem::handleWindowMessage(hwnd, msg, wParam, lParam);
    if (inputResult != -1) {
        return inputResult;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
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

void Window::applyWindowedClientSize(int width, int height)
{
    if (hwnd_ == nullptr || width <= 0 || height <= 0) {
        pendingWidth_ = width > 0 ? width : pendingWidth_;
        pendingHeight_ = height > 0 ? height : pendingHeight_;
        return;
    }
    pendingWidth_ = width;
    pendingHeight_ = height;
    resizeClientArea(hwnd_, width, height, windowedStyleFlags());
    notifyResize();
}

void Window::enterBorderlessFullscreen()
{
    if (hwnd_ == nullptr) {
        return;
    }
    if (!hasSavedWindowedState_) {
        windowedStyle_ = GetWindowLongW(hwnd_, GWL_STYLE);
        windowedExStyle_ = GetWindowLongW(hwnd_, GWL_EXSTYLE);
        windowedPlacement_.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd_, &windowedPlacement_);
        hasSavedWindowedState_ = true;
    }
    const DisplayMode desktop = getDesktopDisplayMode();
    pendingWidth_ = desktop.width;
    pendingHeight_ = desktop.height;
    SetWindowLongW(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowLongW(hwnd_, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowPos(hwnd_, HWND_TOP, 0, 0, desktop.width, desktop.height,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    notifyResize();
}

void Window::exitBorderlessFullscreen()
{
    if (hwnd_ == nullptr) {
        return;
    }
    if (hasSavedWindowedState_) {
        SetWindowLongW(hwnd_, GWL_STYLE, windowedStyle_);
        SetWindowLongW(hwnd_, GWL_EXSTYLE, windowedExStyle_);
        SetWindowPlacement(hwnd_, &windowedPlacement_);
        hasSavedWindowedState_ = false;
    } else {
        SetWindowLongW(hwnd_, GWL_STYLE, windowedStyleFlags());
        SetWindowLongW(hwnd_, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    }
    applyWindowedClientSize(pendingWidth_, pendingHeight_);
}

void Window::setFullscreen(bool value)
{
    if (fullscreen_ == value) {
        return;
    }
    fullscreen_ = value;
    if (hwnd_ == nullptr) {
        return;
    }
    if (fullscreen_) {
        enterBorderlessFullscreen();
    } else {
        exitBorderlessFullscreen();
    }
}

void Window::setDisplayMode(const DisplayMode& mode)
{
    pendingWidth_ = mode.width;
    pendingHeight_ = mode.height;
    if (hwnd_ == nullptr) {
        return;
    }
    if (fullscreen_) {
        SetWindowPos(hwnd_, HWND_TOP, 0, 0, mode.width, mode.height,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        notifyResize();
        return;
    }
    applyWindowedClientSize(mode.width, mode.height);
}

void Window::setTitle(const char* title)
{
    if (hwnd_ == nullptr || title == nullptr) {
        return;
    }
    const std::wstring wide = utf8ToWide(title);
    if (!wide.empty()) {
        SetWindowTextW(hwnd_, wide.c_str());
    }
}

void Window::notifyResize()
{
    if (hwnd_ == nullptr || !resizeCallback_) {
        return;
    }
    RECT rect {};
    if (!GetClientRect(hwnd_, &rect)) {
        return;
    }
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
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
    WNDCLASSW wc {};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MinecraftNativeClient";
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    RegisterClassW(&wc);

    RECT wantClient {0, 0, pendingWidth_, pendingHeight_};
    AdjustWindowRect(&wantClient, WS_OVERLAPPEDWINDOW, FALSE);
    const int outerWidth = wantClient.right - wantClient.left;
    const int outerHeight = wantClient.bottom - wantClient.top;
    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"Minecraft Beta 1.7.3", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, outerWidth, outerHeight, nullptr, nullptr, wc.hInstance, nullptr);
    deviceContext_ = GetDC(hwnd_);
    PIXELFORMATDESCRIPTOR format {};
    format.nSize = sizeof(format);
    format.nVersion = 1;
    format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    format.iPixelType = PFD_TYPE_RGBA;
    format.cColorBits = 24;
    format.cAlphaBits = 8;
    format.cDepthBits = 24;
    const int pixelFormat = ChoosePixelFormat(deviceContext_, &format);
    if (pixelFormat == 0 || !SetPixelFormat(deviceContext_, pixelFormat, &format)) {
        throw std::runtime_error("ChoosePixelFormat/SetPixelFormat failed");
    }
    glContext_ = wglCreateContext(deviceContext_);
    if (glContext_ == nullptr || !wglMakeCurrent(deviceContext_, glContext_)) {
        throw std::runtime_error("wglCreateContext/wglMakeCurrent failed");
    }
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    if (fullscreen_) {
        enterBorderlessFullscreen();
    }
}

void Window::ensureGlContext()
{
    if (deviceContext_ == nullptr || glContext_ == nullptr) {
        throw std::runtime_error("OpenGL context not created");
    }
    if (!wglMakeCurrent(deviceContext_, glContext_)) {
        throw std::runtime_error("wglMakeCurrent failed");
    }
}

void Window::destroy()
{
    if (glContext_ != nullptr) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(glContext_);
        glContext_ = nullptr;
    }
    if (hwnd_ != nullptr && deviceContext_ != nullptr) {
        ReleaseDC(hwnd_, deviceContext_);
        deviceContext_ = nullptr;
    }
    if (hwnd_ != nullptr) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void Window::pumpMessages()
{
    MSG msg {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Window::present()
{
    if (deviceContext_ != nullptr) {
        SwapBuffers(deviceContext_);
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
    if (hwnd_ == nullptr) {
        return {pendingWidth_, pendingHeight_};
    }
    RECT rect {};
    GetClientRect(hwnd_, &rect);
    return {rect.right - rect.left, rect.bottom - rect.top};
}

DisplayMode Window::getDesktopDisplayMode()
{
    return {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
}

HWND Window::hwnd()
{
    return hwnd_;
}

} // namespace net::minecraft::client::platform::win32

#endif // _WIN32
