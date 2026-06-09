#include "net/minecraft/client/input/InputSystem.hpp"



#include "net/minecraft/client/Minecraft.hpp"

#include "net/minecraft/client/gui/screen/ChatScreen.hpp"

#include "net/minecraft/client/gui/screen/ingame/InventoryScreen.hpp"

#include "net/minecraft/client/input/KeyCodes.hpp"

#include "net/minecraft/client/option/GameOptions.hpp"

#include "net/minecraft/client/platform/win32/Window.hpp"

#include "net/minecraft/client/util/DisplayManager.hpp"

#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"



#include <chrono>



#ifdef _WIN32

#ifndef NOMINMAX

#define NOMINMAX

#endif

#include <windows.h>

#endif



namespace net::minecraft::client::input {



namespace {



std::int64_t currentTimeMillis()

{

    return std::chrono::duration_cast<std::chrono::milliseconds>(

        std::chrono::system_clock::now().time_since_epoch()).count();

}



#ifdef _WIN32

POINT clientPointFromLParam(LPARAM lParam)

{

    return POINT {

        static_cast<LONG>(static_cast<short>(LOWORD(lParam))),

        static_cast<LONG>(static_cast<short>(HIWORD(lParam))),

    };

}

#endif



} // namespace



InputSystem& InputSystem::instance()

{

    static InputSystem system;

    return system;

}



InputScope::InputScope(InputLayer layer)

    : layer_(layer)

{

    InputSystem::instance().pushLayer(layer_);

}



InputScope::~InputScope()

{

    InputSystem::instance().popLayer();

}



void InputSystem::pushLayer(InputLayer layer)

{

    layerStack_.push_back(layer);

}



void InputSystem::popLayer()

{

    if (!layerStack_.empty()) {

        layerStack_.pop_back();

    }

}



bool InputSystem::acceptsInput() const

{

#ifdef _WIN32

    return platform::win32::Window::isActive();

#else

    return true;

#endif

}



InputLayer InputSystem::currentLayer() const

{

    if (!layerStack_.empty()) {

        return layerStack_.back();

    }

    return inferredLayer_;

}



bool InputSystem::passesKeyboardToGame() const

{

    return currentLayer() == InputLayer::Game || currentLayer() == InputLayer::ScreenPassthrough;

}



SlotClickModifier InputSystem::slotClickModifier() const noexcept

{

    return modifiers_.guiShift ? SlotClickModifier::Shift : SlotClickModifier::None;

}



bool InputSystem::bindingKeyMatches(int key, int bindingCode)

{

    if (key == bindingCode) {

        return true;

    }

    if (bindingCode == 42 || bindingCode == 54) {

        return key == 42 || key == 54;

    }

    if (bindingCode == 29 || bindingCode == 157) {

        return key == 29 || key == 157;

    }

    if (bindingCode == 56 || bindingCode == 184) {

        return key == 56 || key == 184;

    }

    return false;

}



bool InputSystem::isBindingDown(const option::GameOptions& options, const option::KeyBinding& binding) const

{

    (void)options;

    if (isKeyDown(binding.code)) {

        return true;

    }

    if (binding.code == 42) {

        return isKeyDown(54);

    }

    if (binding.code == 54) {

        return isKeyDown(42);

    }

    if (binding.code == 29) {

        return isKeyDown(157);

    }

    if (binding.code == 157) {

        return isKeyDown(29);

    }

    if (binding.code == 56) {

        return isKeyDown(184);

    }

    if (binding.code == 184) {

        return isKeyDown(56);

    }

    return false;

}



void InputSystem::setKeyboardRepeat(bool enabled)

{

#ifdef _WIN32

    keyboardRepeat_ = enabled;

#else

    (void)enabled;

#endif

}



void InputSystem::resetBindings()

{

    movement_ = {};

}



void InputSystem::refreshMovement(option::GameOptions& options)

{

    const bool forward = isBindingDown(options, options.forwardKey);

    const bool back = isBindingDown(options, options.backKey);

    const bool left = isBindingDown(options, options.leftKey);

    const bool right = isBindingDown(options, options.rightKey);



    movement_.sideways = 0.0f;

    movement_.forward = 0.0f;

    if (forward) {

        movement_.forward += 1.0f;

    }

    if (back) {

        movement_.forward -= 1.0f;

    }

    if (left) {

        movement_.sideways += 1.0f;

    }

    if (right) {

        movement_.sideways -= 1.0f;

    }

    movement_.jumping = isBindingDown(options, options.jumpKey);

    movement_.sneaking = isBindingDown(options, options.sneakKey);

    if (movement_.sneaking) {

        movement_.sideways = static_cast<float>(static_cast<double>(movement_.sideways) * 0.3);

        movement_.forward = static_cast<float>(static_cast<double>(movement_.forward) * 0.3);

    }

}



#ifdef _WIN32



void InputSystem::init(HWND window)

{

    InputSystem& sys = instance();

    sys.keyboardDown_.fill(false);

    sys.keyboardEvents_.clear();

    sys.keyboardReadIndex_ = 0;

    sys.mouseDown_.fill(false);

    sys.mouseEvents_.clear();

    sys.mouseReadIndex_ = 0;

    sys.cursorX_ = 0;

    sys.cursorY_ = 0;

    sys.initMouse(window);

}



void InputSystem::shutdown()

{

    clearOnDeactivate();

}



void InputSystem::clearOnDeactivate()

{

    InputSystem& sys = instance();

    sys.keyboardDown_.fill(false);

    sys.keyboardEvents_.clear();

    sys.keyboardReadIndex_ = 0;

    sys.mouseDown_.fill(false);

    sys.mouseEvents_.clear();

    sys.mouseReadIndex_ = 0;

    sys.cursorX_ = 0;

    sys.cursorY_ = 0;

    sys.releaseCapturedKeys();

    sys.resetBindings();

    sys.modifiers_ = {};

}



void InputSystem::compactQueues()

{

    InputSystem& sys = instance();

    if (sys.keyboardReadIndex_ > 0) {

        sys.keyboardEvents_.erase(sys.keyboardEvents_.begin(),

            sys.keyboardEvents_.begin() + static_cast<std::ptrdiff_t>(sys.keyboardReadIndex_));

        sys.keyboardReadIndex_ = 0;

    }

    if (sys.mouseReadIndex_ > 0) {

        sys.mouseEvents_.erase(sys.mouseEvents_.begin(),

            sys.mouseEvents_.begin() + static_cast<std::ptrdiff_t>(sys.mouseReadIndex_));

        sys.mouseReadIndex_ = 0;

    }

    sys.updateCursorPosition();

}



int InputSystem::clientMouseY(HWND hwnd, int clientY)

{

    if (hwnd == nullptr) {

        return clientY;

    }

    RECT clientRect {};

    if (!GetClientRect(hwnd, &clientRect)) {

        return clientY;

    }

    return (clientRect.bottom - clientRect.top) - clientY;

}



LRESULT InputSystem::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{

    InputSystem& sys = instance();

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {

        sys.ingestKey(static_cast<int>(wParam), true);

        return 0;

    }

    if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {

        sys.ingestKey(static_cast<int>(wParam), false);

        return 0;

    }

    if (msg == WM_CHAR) {

        sys.ingestChar(static_cast<int>(wParam));

        return 0;

    }

    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP

        || msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) {

        const int button = msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ? 0

            : (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ? 1 : 2);

        const bool down = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;

        const POINT point = clientPointFromLParam(lParam);

        sys.ingestMouseButton(button, down, point.x, clientMouseY(hwnd, static_cast<int>(point.y)));

        return 0;

    }

    if (msg == WM_MOUSEWHEEL) {

        POINT point {

            static_cast<LONG>(static_cast<short>(LOWORD(lParam))),

            static_cast<LONG>(static_cast<short>(HIWORD(lParam))),

        };

        ScreenToClient(hwnd, &point);

        sys.ingestMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam), point.x,

            clientMouseY(hwnd, static_cast<int>(point.y)));

        return 0;

    }

    return -1;

}



void InputSystem::ingestKey(int vk, bool down)

{

    const int key = keyFromVirtualKey(vk);

    if (key < 0 || key >= static_cast<int>(keyboardDown_.size())) {

        return;

    }

    const std::size_t index = static_cast<std::size_t>(key);

    if (keyboardDown_[index] == down) {

        if (!(down && keyboardRepeat_)) {

            return;

        }

    } else {

        keyboardDown_[index] = down;

    }

    keyboardEvents_.push_back({key, down, 0});

}



void InputSystem::ingestChar(int character)

{

    if (keyboardEvents_.empty()) {

        return;

    }

    KeyboardEvent& last = keyboardEvents_.back();

    if (!last.down) {

        return;

    }

    if (character >= 0 && character <= 255) {

        last.character = static_cast<char>(character);

    }

}



void InputSystem::ingestMouseButton(int button, bool down, int x, int y)

{

    if (button < 0 || button >= static_cast<int>(mouseDown_.size())) {

        return;

    }

    cursorX_ = x;

    cursorY_ = y;

    const std::size_t index = static_cast<std::size_t>(button);

    if (mouseDown_[index] == down) {

        return;

    }

    mouseDown_[index] = down;

    mouseEvents_.push_back({x, y, button, down, 0});

}



void InputSystem::ingestMouseWheel(int delta, int x, int y)

{

    if (delta == 0) {

        return;

    }

    cursorX_ = x;

    cursorY_ = y;

    mouseEvents_.push_back({x, y, -1, false, delta});

}



void InputSystem::updateCursorPosition()

{

    POINT cursor {};

    GetCursorPos(&cursor);

    HWND hwnd = platform::win32::Window::hwnd();

    if (hwnd != nullptr) {

        ScreenToClient(hwnd, &cursor);

        RECT clientRect {};

        GetClientRect(hwnd, &clientRect);

        cursorX_ = cursor.x;

        cursorY_ = (clientRect.bottom - clientRect.top) - cursor.y;

    } else {

        cursorX_ = cursor.x;

        cursorY_ = cursor.y;

    }

}



void InputSystem::initMouse(HWND parentWindow)

{

    mouseWindow_ = parentWindow;

    mouseCursor_ = LoadCursor(nullptr, IDC_ARROW);

    if (mouseWindow_ != nullptr) {

        RECT rect {};

        if (GetClientRect(mouseWindow_, &rect)) {

            mouseLastPoint_.x = (rect.right - rect.left) / 2;

            mouseLastPoint_.y = (rect.bottom - rect.top) / 2;

            mouseHasLastPoint_ = true;

        }

    }

}



void InputSystem::lockCursor()

{

    cursorGrabbed_ = true;

    mouseLookDeltaX_ = 0;

    mouseLookDeltaY_ = 0;

    if (mouseWindow_ == nullptr) {

        return;

    }

    RECT rect {};

    if (!GetClientRect(mouseWindow_, &rect)) {

        return;

    }

    POINT topLeft {rect.left, rect.top};

    POINT bottomRight {rect.right, rect.bottom};

    ClientToScreen(mouseWindow_, &topLeft);

    ClientToScreen(mouseWindow_, &bottomRight);

    rect.left = topLeft.x;

    rect.top = topLeft.y;

    rect.right = bottomRight.x;

    rect.bottom = bottomRight.y;

    ClipCursor(&rect);

    while (ShowCursor(FALSE) >= 0) {

    }

}



void InputSystem::unlockCursor()

{

    cursorGrabbed_ = false;

    ClipCursor(nullptr);

    while (ShowCursor(TRUE) < 0) {

    }

    if (mouseWindow_ == nullptr) {

        return;

    }

    RECT rect {};

    if (!GetClientRect(mouseWindow_, &rect)) {

        return;

    }

    const int centerX = (rect.right - rect.left) / 2;

    const int centerY = (rect.bottom - rect.top) / 2;

    POINT center {centerX, centerY};

    ClientToScreen(mouseWindow_, &center);

    SetCursorPos(center.x, center.y);

    mouseLastPoint_ = {centerX, centerY};

    mouseHasLastPoint_ = true;

}



void InputSystem::pollMouseLook()

{

    POINT point {};

    if (!GetCursorPos(&point)) {

        mouseLookDeltaX_ = 0;

        mouseLookDeltaY_ = 0;

        return;

    }

    if (mouseWindow_ != nullptr) {

        ScreenToClient(mouseWindow_, &point);

    }

    if (!mouseHasLastPoint_) {

        mouseLastPoint_ = point;

        mouseHasLastPoint_ = true;

        mouseLookDeltaX_ = 0;

        mouseLookDeltaY_ = 0;

        return;

    }

    mouseLookDeltaX_ = point.x - mouseLastPoint_.x;

    mouseLookDeltaY_ = mouseLastPoint_.y - point.y;

    if (cursorGrabbed_ && mouseWindow_ != nullptr) {

        RECT rect {};

        if (GetClientRect(mouseWindow_, &rect)) {

            const int centerX = (rect.right - rect.left) / 2;

            const int centerY = (rect.bottom - rect.top) / 2;

            POINT center {centerX, centerY};

            ClientToScreen(mouseWindow_, &center);

            SetCursorPos(center.x, center.y);

            mouseLastPoint_.x = centerX;

            mouseLastPoint_.y = centerY;

        }

    } else {

        mouseLastPoint_ = point;

    }

}



bool InputSystem::isKeyDown(int keyCode) const

{

    if (keyCode < 0 || keyCode >= 256) {

        return false;

    }

    if (keyboardDown_[static_cast<std::size_t>(keyCode)]) {

        return true;

    }

    if (!platform::win32::Window::isActive()) {

        return false;

    }

    const int vk = virtualKeyFromKey(keyCode);

    if (vk < 0) {

        return false;

    }

    return (GetAsyncKeyState(vk) & 0x8000) != 0;

}



#endif



bool InputSystem::isMouseButtonDown(int button) const

{

#ifdef _WIN32

    if (button < 0 || button >= static_cast<int>(mouseDown_.size())) {

        return false;

    }

    return mouseDown_[static_cast<std::size_t>(button)];

#else

    (void)button;

    return false;

#endif

}



int InputSystem::mouseX() const noexcept

{

#ifdef _WIN32

    return cursorX_;

#else

    return 0;

#endif

}



int InputSystem::mouseY() const noexcept

{

#ifdef _WIN32

    return cursorY_;

#else

    return 0;

#endif

}



void InputSystem::beginFrame(Minecraft& client)

{

    gui::screen::Screen* screen = client.currentScreen();

    if (screen != activeScreen_) {

        onScreenChanged(screen);

        activeScreen_ = screen;

    }



    if (screen == nullptr) {

        inferredLayer_ = InputLayer::Game;

    } else if (screen->passEvents) {

        inferredLayer_ = InputLayer::ScreenPassthrough;

    } else {

        inferredLayer_ = InputLayer::ScreenModal;

    }



    if (!acceptsInput()) {

        releaseCapturedKeys();

        resetBindings();

        modifiers_ = {};

        return;

    }



#ifdef _WIN32

    compactQueues();

#endif

    refreshModifiers();

}



void InputSystem::refreshModifiers()

{

#ifdef _WIN32

    modifiers_.shift = isKeyDown(42) || isKeyDown(54);

    modifiers_.ctrl = isKeyDown(29) || isKeyDown(157);

    modifiers_.alt = isKeyDown(56) || isKeyDown(184);

    modifiers_.guiShift = isKeyDown(54) || (isKeyDown(42) && guiLeftShiftIntent_);

#else

    modifiers_ = {};

#endif

}



void InputSystem::handleKeyboardEdge(int key, bool down)

{

    if (key == 42) {

        if (down && currentLayer() == InputLayer::ScreenPassthrough) {

            guiLeftShiftIntent_ = true;

        }

        if (!down) {

            guiLeftShiftIntent_ = false;

        }

    }

}



void InputSystem::onScreenChanged(gui::screen::Screen* screen)

{

    guiLeftShiftIntent_ = false;

    if (screen != nullptr && screen->passEvents) {

#ifdef _WIN32

        if (isKeyDown(42) && !isKeyDown(54)) {

            guiLeftShiftIntent_ = false;

        }

#endif

    }

    (void)screen;

}



void InputSystem::releaseCapturedKeys()

{

    layerStack_.clear();

    guiLeftShiftIntent_ = false;

}



bool InputSystem::nextMouseEvent()

{

#ifdef _WIN32

    if (mouseReadIndex_ >= mouseEvents_.size()) {

        return false;

    }

    const MouseEvent& ev = mouseEvents_[mouseReadIndex_];

    ++mouseReadIndex_;

    eventMouseX_ = ev.x;

    eventMouseY_ = ev.y;

    eventMouseButton_ = ev.button;

    eventMouseButtonDown_ = ev.down;

    eventMouseWheel_ = ev.wheel;

    return true;

#else

    return false;

#endif

}



bool InputSystem::nextKeyboardEvent()

{

#ifdef _WIN32

    if (keyboardReadIndex_ >= keyboardEvents_.size()) {

        return false;

    }

    const KeyboardEvent& ev = keyboardEvents_[keyboardReadIndex_];

    ++keyboardReadIndex_;

    eventKey_ = ev.key;

    eventKeyDown_ = ev.down;

    eventChar_ = ev.character;

    handleKeyboardEdge(eventKey_, eventKeyDown_);

    return true;

#else

    return false;

#endif

}



int InputSystem::eventMouseX() const noexcept { return eventMouseX_; }

int InputSystem::eventMouseY() const noexcept { return eventMouseY_; }

int InputSystem::eventMouseButton() const noexcept { return eventMouseButton_; }

bool InputSystem::eventMouseButtonDown() const noexcept { return eventMouseButtonDown_; }

int InputSystem::eventMouseWheel() const noexcept { return eventMouseWheel_; }

int InputSystem::eventKey() const noexcept { return eventKey_; }

bool InputSystem::eventKeyDown() const noexcept { return eventKeyDown_; }

char InputSystem::eventChar() const noexcept { return eventChar_; }



void InputSystem::drainScreenEvents(gui::screen::Screen& screen)

{

    if (!acceptsInput()) {

        return;

    }

    while (nextMouseEvent()) {

        screen.onMouseEvent();

    }

    if (!screen.passEvents) {

        while (nextKeyboardEvent()) {

            screen.onKeyboardEvent();

        }

    }

}



void InputSystem::pollGame(Minecraft& client)

{

    if (!acceptsInput()) {

        return;

    }



#ifdef _WIN32

    if ((client.currentScreen() == nullptr || client.currentScreen()->passEvents) && client.player != nullptr) {

        while (nextMouseEvent()) {

            if (currentTimeMillis() - client.lastTickTime > 200) {

                continue;

            }

            const int wheel = eventMouseWheel();

            if (wheel != 0) {

                client.player->inventory.scrollInHotbar(wheel);

                if (client.options.discreteScroll) {

                    int discrete = wheel;

                    if (discrete > 0) {

                        discrete = 1;

                    }

                    if (discrete < 0) {

                        discrete = -1;

                    }

                    client.options.totalDiscreteScroll += static_cast<float>(discrete) * 0.25f;

                }

            }

            if (client.currentScreen() == nullptr) {

                if (!client.focused.load() && eventMouseButtonDown()) {

                    client.lockMouse();

                    continue;

                }

                if (eventMouseButton() == 0 && eventMouseButtonDown()) {

                    client.handleMouseClick(0);

                    client.lastClickTicks = client.ticksPlayed;

                }

                if (eventMouseButton() == 1 && eventMouseButtonDown()) {

                    client.handleMouseClick(1);

                    client.lastClickTicks = client.ticksPlayed;

                }

                if (eventMouseButton() == 2 && eventMouseButtonDown()) {

                    client.handlePickBlock();

                }

            }

        }

        if (client.attackCooldown > 0) {

            --client.attackCooldown;

        }

        while (nextKeyboardEvent()) {

            if (!passesKeyboardToGame()) {

                continue;

            }

            if (!eventKeyDown()) {

                continue;

            }

            if (eventKey() == 87) {

                util::DisplayManager::toggleFullscreen(client);

                continue;

            }

            if (client.currentScreen() != nullptr) {

                client.currentScreen()->onKeyboardEvent();

            } else {

                if (eventKey() == 1) {

                    client.pauseGame();

                }

                if (eventKey() == 31 && isKeyDown(61)) {

                    client.forceResourceReload();

                }

                if (eventKey() == 59) {

                    client.options.hideHud = !client.options.hideHud;

                }

                if (eventKey() == 61) {

                    client.options.debugHud = !client.options.debugHud;

                }

                if (eventKey() == 63) {

                    client.options.thirdPerson = !client.options.thirdPerson;

                }

                if (eventKey() == 66) {

                    client.options.cinematicMode = !client.options.cinematicMode;

                }

                if (eventKey() == static_cast<int>(client.options.inventoryKey.code) && client.player != nullptr) {

                    client.setScreen(std::make_unique<gui::screen::ingame::InventoryScreen>(&client.player->playerScreenHandler));

                }

                if (eventKey() == static_cast<int>(client.options.dropKey.code)) {

                    client.player->dropSelectedItem();

                }

                if (client.isWorldRemote() && eventKey() == static_cast<int>(client.options.chatKey.code)) {

                    client.setScreen(std::make_unique<gui::screen::ChatScreen>());

                }

                for (int slot = 0; slot < 9; ++slot) {

                    if (eventKey() == 2 + slot) {

                        client.player->inventory.selectedSlot = slot;

                    }

                }

                if (eventKey() == static_cast<int>(client.options.fogKey.code)) {

                    client.options.cycle("viewDistance", modifiers_.shift ? -1 : 1);

                }

            }

        }

        if (client.currentScreen() == nullptr) {

            if (isMouseButtonDown(0)

                && static_cast<float>(client.ticksPlayed - client.lastClickTicks) >= client.timer.tps / 4.0f

                && client.focused.load()) {

                client.handleMouseClick(0);

                client.lastClickTicks = client.ticksPlayed;

            }

            if (isMouseButtonDown(1)

                && static_cast<float>(client.ticksPlayed - client.lastClickTicks) >= client.timer.tps / 4.0f

                && client.focused.load()) {

                client.handleMouseClick(1);

                client.lastClickTicks = client.ticksPlayed;

            }

        }

        client.handleMouseDown(0, client.currentScreen() == nullptr && isMouseButtonDown(0) && client.focused.load());

        refreshMovement(client.options);

    }

#endif

}



} // namespace net::minecraft::client::input


