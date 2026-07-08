#pragma once
#include "net/minecraft/mod/runtime/ModHost.hpp"
struct lua_State;

namespace net::minecraft::mod::runtime {
void installScreenApi(lua_State* state);
}
#ifdef MINECRAFT_NATIVE_EXPORTS
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"

namespace net::minecraft::mod::runtime {
class ScopedLuaGuiDraw {
   public:
    explicit ScopedLuaGuiDraw(bool enabled = true);
    ~ScopedLuaGuiDraw();
    ScopedLuaGuiDraw(const ScopedLuaGuiDraw&) = delete;
    ScopedLuaGuiDraw& operator=(const ScopedLuaGuiDraw&) = delete;

   private:
    bool enabled_ = false;
};

[[nodiscard]] bool luaGuiDrawActive() noexcept;

struct ActiveScreenUi {
    ScreenUiContext* context = nullptr;
    ModHost::LoadedLuaMod* mod = nullptr;
    int stackedY = 0;
    bool trackStacked = false;
};

ActiveScreenUi* activeScreenUi();
class LuaScreen;

struct ActiveLuaScreen {
    LuaScreen* screen = nullptr;
    ModHost::LoadedLuaMod* mod = nullptr;
    bool initPhase = false;
};

ActiveLuaScreen* activeLuaScreen();
enum class LuaScreenPhase {
    Init,
    Render,
    Tick,
    Key,
    Mouse,
    Scroll,
    Close,
};

struct LuaScreenEvent {
    LuaScreen* screen = nullptr;
    LuaScreenPhase phase = LuaScreenPhase::Init;
    int mouseX = 0;
    int mouseY = 0;
    float tickDelta = 0.0f;
    char character = 0;
    int keyCode = 0;
    int button = 0;
    bool released = false;
    int scrollDelta = 0;
    bool handled = false;
};

struct LuaScreenField {
    std::string name;
    std::unique_ptr<client::gui::widget::TextFieldWidget> widget;
    bool numeric = false;
    bool allowSign = false;
    bool allowDot = false;
};

class LuaScreen : public client::gui::screen::Screen {
   public:
    explicit LuaScreen(std::string id, std::string title = {}) : id_(std::move(id)), title_(std::move(title)) {
    }

    [[nodiscard]] std::string_view getScreenUiId() const override {
        return id_;
    }

    [[nodiscard]] const std::string& id() const {
        return id_;
    }

    [[nodiscard]] bool shouldPause() const override {
        return true;
    }

    void init() override;
    void tick() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;
    void mouseReleased(int mouseX, int mouseY, int button) override;
    void keyPressed(char character, int keyCode) override;
    void mouseScrolled(int mouseX, int mouseY, int delta) override;
    void removed() override;
    client::gui::widget::TextFieldWidget* addField(const std::string& name,
                                                   int x,
                                                   int y,
                                                   int width,
                                                   int height,
                                                   const std::string& text,
                                                   int maxLength,
                                                   bool numeric,
                                                   bool allowSign,
                                                   bool allowDot);
    [[nodiscard]] client::gui::widget::TextFieldWidget* field(const std::string& name);
    void addLuaButton(int x, int y, int width, int height, std::string text, std::function<void()> onClick);

    void setFieldsVisible(bool visible) {
        fieldsVisible_ = visible;
    }

    [[nodiscard]] const std::string& title() const {
        return title_;
    }

    void setTitle(std::string title) {
        title_ = std::move(title);
    }

   private:
    static bool charAllowed(const LuaScreenField& field, char character);
    std::string id_;
    std::string title_;
    std::vector<LuaScreenField> fields_;
    bool fieldsVisible_ = true;
};

int retainButtonCallback(lua_State* state, ModHost::LoadedLuaMod* mod, int fnIndex);
void invokeButtonCallback(ModHost::LoadedLuaMod* mod, int ref);
void pushScreenUiTable(lua_State* state);
void pushHostFieldsTable(lua_State* state, client::gui::screen::Screen* screen);
}  // namespace net::minecraft::mod::runtime
#endif
