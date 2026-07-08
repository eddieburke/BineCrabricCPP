#include "net/minecraft/mod/runtime/LuaScreenBindings.hpp"
#include "net/minecraft/mod/runtime/LuaGuiArgs.hpp"
#include "net/minecraft/mod/runtime/LuaGuiViewport.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/HostScreenRegistry.hpp"
#include "net/minecraft/client/session/OfflineIdentity.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#endif
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
namespace {
thread_local int g_luaGuiDepth = 0;
client::render::item::ItemRenderer g_luaItemRenderer;
} // namespace
// --- minecraft.session ---------------------------------------------------
// Read/write the runtime identity override used for offline-mode joins, plus reads of
// the live session. Exposed so any mod (e.g. an offline-mode helper) can change the name
// the client presents to non-authenticated servers without touching engine internals.
int luaSessionSetOfflineUsername(lua_State* state) {
  client::session::setOfflineUsername(luaString(state, 1, ""));
  return 0;
}
int luaSessionClearOfflineUsername(lua_State* state) {
  (void)state;
  client::session::clearOfflineUsername();
  return 0;
}
int luaSessionIsOfflineMode(lua_State* state) {
  LuaApi& api = luaApi();
  api.pushboolean(state, client::session::hasOfflineUsername() ? 1 : 0);
  return 1;
}
int luaSessionGetOfflineUsername(lua_State* state) {
  LuaApi& api = luaApi();
  const std::string& name = client::session::offlineUsername();
  api.pushlstring(state, name.data(), name.size());
  return 1;
}
int luaSessionGetUsername(lua_State* state) {
  LuaApi& api = luaApi();
  const client::Minecraft* client = client::Minecraft::INSTANCE;
  const std::string& name = client != nullptr ? client->session.username : std::string{};
  api.pushlstring(state, name.data(), name.size());
  return 1;
}
int luaSessionIsAuthenticated(lua_State* state) {
  LuaApi& api = luaApi();
  const client::Minecraft* client = client::Minecraft::INSTANCE;
  const bool authed = client != nullptr && msauth::isAuthenticated(client->session);
  api.pushboolean(state, authed ? 1 : 0);
  return 1;
}
ScopedLuaGuiDraw::ScopedLuaGuiDraw(bool enabled) : enabled_(enabled) {
  if(enabled_) {
    ++g_luaGuiDepth;
  }
}
ScopedLuaGuiDraw::~ScopedLuaGuiDraw() {
  if(enabled_) {
    --g_luaGuiDepth;
  }
}
bool luaGuiDrawActive() noexcept {
  return g_luaGuiDepth > 0;
}
ActiveScreenUi* activeScreenUi() {
  thread_local ActiveScreenUi session;
  return &session;
}
ActiveLuaScreen* activeLuaScreen() {
  thread_local ActiveLuaScreen session;
  return &session;
}
void LuaScreen::init() {
  enableTextInput();
  fields_.clear();
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  session->initPhase = true;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Init;
  hooks().publish(event);
  session->initPhase = false;
  session->screen = nullptr;
}
void LuaScreen::tick() {
  for(LuaScreenField& field : fields_) {
    if(field.widget != nullptr) {
      field.widget->tick();
    }
  }
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Tick;
  hooks().publish(event);
  session->screen = nullptr;
}
void LuaScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(!title_.empty() && textRenderer() != nullptr) {
    drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
  }
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Render;
  event.mouseX = mouseX;
  event.mouseY = mouseY;
  event.tickDelta = tickDelta;
  {
    const ScopedLuaGuiDraw drawScope;
    hooks().publish(event);
  }
  session->screen = nullptr;
  if(fieldsVisible_) {
    for(LuaScreenField& field : fields_) {
      if(field.widget != nullptr) {
        field.widget->render();
      }
    }
  }
  client::gui::screen::Screen::render(mouseX, mouseY, tickDelta);
}
void LuaScreen::mouseClicked(int mouseX, int mouseY, int button) {
  if(fieldsVisible_) {
    for(LuaScreenField& field : fields_) {
      if(field.widget != nullptr) {
        field.widget->mouseClicked(mouseX, mouseY, button);
      }
    }
  }
  client::gui::screen::Screen::mouseClicked(mouseX, mouseY, button);
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Mouse;
  event.mouseX = mouseX;
  event.mouseY = mouseY;
  event.button = button;
  hooks().publish(event);
  session->screen = nullptr;
}
void LuaScreen::mouseReleased(int mouseX, int mouseY, int button) {
  client::gui::screen::Screen::mouseReleased(mouseX, mouseY, button);
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Mouse;
  event.mouseX = mouseX;
  event.mouseY = mouseY;
  event.button = button;
  event.released = true;
  hooks().publish(event);
  session->screen = nullptr;
}
void LuaScreen::keyPressed(char character, int keyCode) {
  if(fieldsVisible_) {
    for(LuaScreenField& field : fields_) {
      if(field.widget != nullptr && field.widget->focused) {
        if(!(character >= 32 && field.numeric && !charAllowed(field, character))) {
          field.widget->keyPressed(character, keyCode);
        }
        break;
      }
    }
  }
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Key;
  event.character = character;
  event.keyCode = keyCode;
  hooks().publish(event);
  session->screen = nullptr;
}
void LuaScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Scroll;
  event.mouseX = mouseX;
  event.mouseY = mouseY;
  event.scrollDelta = delta;
  hooks().publish(event);
  session->screen = nullptr;
}
void LuaScreen::removed() {
  ActiveLuaScreen* session = activeLuaScreen();
  session->screen = this;
  LuaScreenEvent event;
  event.screen = this;
  event.phase = LuaScreenPhase::Close;
  hooks().publish(event);
  session->screen = nullptr;
}
client::gui::widget::TextFieldWidget* LuaScreen::addField(const std::string& name, int x, int y, int width, int height,
                                                          const std::string& text, int maxLength, bool numeric,
                                                          bool allowSign, bool allowDot) {
  LuaScreenField field;
  field.name = name;
  field.numeric = numeric;
  field.allowSign = allowSign;
  field.allowDot = allowDot;
  field.widget =
      std::make_unique<client::gui::widget::TextFieldWidget>(this, textRenderer(), x, y, width, height, text);
  if(maxLength > 0) {
    field.widget->setMaxLength(maxLength);
  }
  client::gui::widget::TextFieldWidget* ptr = field.widget.get();
  fields_.push_back(std::move(field));
  return ptr;
}
client::gui::widget::TextFieldWidget* LuaScreen::field(const std::string& name) {
  for(LuaScreenField& entry : fields_) {
    if(entry.name == name) {
      return entry.widget.get();
    }
  }
  return nullptr;
}
void LuaScreen::addLuaButton(int x, int y, int width, int height, std::string text, std::function<void()> onClick) {
  (void)addButton<client::gui::widget::ActionButtonWidget>(client::gui::widget::ActionButtonWidget::kNoId, x, y,
                                                           width, height, std::move(text), std::move(onClick));
}
bool LuaScreen::charAllowed(const LuaScreenField& field, char character) {
  if(character >= '0' && character <= '9') {
    return true;
  }
  if(character == '-' && field.allowSign && field.widget->getText().empty()) {
    return true;
  }
  if(character == '.' && field.allowDot && field.widget->getText().find('.') == std::string::npos) {
    return true;
  }
  return false;
}
int retainButtonCallback(lua_State* state, ModHost::LoadedLuaMod* mod, int fnIndex) {
  if(mod == nullptr) {
    return kLuaNoRef;
  }
  LuaApi& api = luaApi();
  if(api.type(state, fnIndex) != kLuaTFunction) {
    return kLuaNoRef;
  }
  api.pushvalue(state, fnIndex);
  const int ref = api.ref(state, kLuaRegistryIndex);
  if(ref != kLuaNoRef) {
    mod->buttonCallbackRefs.push_back(ref);
  }
  return ref;
}
void invokeButtonCallback(ModHost::LoadedLuaMod* mod, int ref) {
  LuaApi& api = luaApi();
  if(mod == nullptr || ref == kLuaNoRef || !api.ready()) {
    return;
  }
  const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
  if(!mod->active || mod->state == nullptr) {
    return;
  }
  auto* state = static_cast<lua_State*>(mod->state);
  api.rawgeti(state, kLuaRegistryIndex, ref);
  if(api.type(state, -1) != kLuaTFunction) {
    api.settop(state, -2);
    return;
  }
  const int status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  if(status != kLuaOk) {
    const char* error = api.tolstring(state, -1, nullptr);
    runtimeLog(mod->modId, "error", error != nullptr ? error : "button callback failed");
    api.settop(state, -2);
  }
}
namespace {
// Shared guard for gui.draw_* calls: only valid inside a Lua GUI draw scope
// with a live client + text renderer.
[[nodiscard]] client::Minecraft* guiDrawClient() {
  if(!luaGuiDrawActive()) {
    return nullptr;
  }
  client::Minecraft* client = client::Minecraft::INSTANCE;
  return (client == nullptr || client->textRenderer == nullptr) ? nullptr : client;
}
struct LuaGuiWidgetDrawer : client::gui::DrawContext {};
void drawVanillaButton(client::Minecraft& minecraft, client::font::TextRenderer& textRenderer, int x, int y, int width,
                       int height, const std::string& text, bool active, bool hovered) {
  LuaGuiWidgetDrawer drawer;
  const int textureId = minecraft.textureManager.getTextureId("/gui/gui.png");
  gl::bindTexture(gl::cap::Texture2D, textureId);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  int imageY = 1;
  if(!active) {
    imageY = 0;
  } else if(hovered) {
    imageY = 2;
  }
  drawer.drawTexture(x, y, 0, 46 + imageY * 20, width / 2, height);
  drawer.drawTexture(x + width / 2, y, 200 - width / 2, 46 + imageY * 20, width / 2, height);
  const int textY = y + (height - 8) / 2;
  if(!active) {
    drawer.drawCenteredTextWithShadow(textRenderer, text, x + width / 2, textY, 0xFFA0A0A0);
  } else if(hovered) {
    drawer.drawCenteredTextWithShadow(textRenderer, text, x + width / 2, textY, 0xFFFFA0);
  } else {
    drawer.drawCenteredTextWithShadow(textRenderer, text, x + width / 2, textY, 0xFFE0E0E0);
  }
}
void drawVanillaSlider(client::Minecraft& minecraft, client::font::TextRenderer& textRenderer, int x, int y, int width,
                       int height, float normalized, const std::string& text, bool hovered) {
  LuaGuiWidgetDrawer drawer;
  const int textureId = minecraft.textureManager.getTextureId("/gui/gui.png");
  gl::bindTexture(gl::cap::Texture2D, textureId);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  const int imageY = hovered ? 2 : 1;
  drawer.drawTexture(x, y, 0, 46 + imageY * 20, width / 2, height);
  drawer.drawTexture(x + width / 2, y, 200 - width / 2, 46 + imageY * 20, width / 2, height);
  const float clamped = std::clamp(normalized, 0.0f, 1.0f);
  const int knobX = x + static_cast<int>(clamped * static_cast<float>(width - 8));
  drawer.drawTexture(knobX, y, 0, 66, 4, height);
  drawer.drawTexture(knobX + 4, y, 196, 66, 4, height);
  drawer.drawCenteredTextWithShadow(textRenderer, text, x + width / 2, y + (height - 8) / 2, 0xFFE0E0E0);
}
[[nodiscard]] std::string toggleStateLabel(bool enabled) {
  const char* key = enabled ? "options.on" : "options.off";
  const std::string translated = client::resource::language::I18n::getTranslation(key);
  if(translated.empty() || translated == key) {
    return enabled ? "ON" : "OFF";
  }
  return translated;
}
void drawVanillaToggle(client::Minecraft& minecraft, client::font::TextRenderer& textRenderer, int x, int y, int width,
                       int height, const std::string& label, bool enabled, bool hovered) {
  const std::string text = label + ": " + toggleStateLabel(enabled);
  drawVanillaButton(minecraft, textRenderer, x, y, width, height, text, true, hovered);
}
void prepareGuiDrawState() {
  const gl::preset::ModLuaGuiDraw guiDraw;
  client::render::platform::Lighting::turnOff();
}
void drawGuiFillRect(int x, int y, int width, int height, std::uint32_t color) {
  const gl::preset::SolidFill fillCaps;
  const int rgb = static_cast<int>(color & 0x00FFFFFFU);
  int alpha = static_cast<int>((color >> 24U) & 0xFFU);
  if(alpha == 0) {
    alpha = 255;
  }
  client::render::Tessellator& tess = client::render::Tessellator::INSTANCE;
  client::gui::draw::coloredQuad(tess, x, y, x + width, y + height, rgb, alpha, 0.0f);
}
int luaGuiDrawButton(lua_State* state) {
  client::Minecraft* client = guiDrawClient();
  GuiDrawArgs args;
  if(client == nullptr || !args.init(state, 5, 5)) {
    return 0;
  }
  const std::string text = args.text("text", nullptr, 5);
  const bool active = args.boolean("active", nullptr, 6, true);
  const bool hovered = args.hovered(7);
  const GuiRect& rect = args.rect();
  prepareGuiDrawState();
  drawVanillaButton(*client, *client->textRenderer, rect.x, rect.y, rect.w, rect.h, text, active, hovered);
  return 0;
}
int luaGuiDrawSlider(lua_State* state) {
  client::Minecraft* client = guiDrawClient();
  GuiDrawArgs args;
  if(client == nullptr || !args.init(state, 6, 6)) {
    return 0;
  }
  const float normalized = args.number("value", "normalized", 5, 0.0f);
  const std::string text = args.text("text", nullptr, 6);
  const bool hovered = args.hovered(7);
  const GuiRect& rect = args.rect();
  prepareGuiDrawState();
  drawVanillaSlider(*client, *client->textRenderer, rect.x, rect.y, rect.w, rect.h, normalized, text, hovered);
  return 0;
}
int luaGuiDrawToggle(lua_State* state) {
  client::Minecraft* client = guiDrawClient();
  GuiDrawArgs args;
  if(client == nullptr || !args.init(state, 6, 5)) {
    return 0;
  }
  const std::string label = args.text("label", "text", 5);
  const bool enabled = args.boolean("value", "enabled", 6, false);
  const bool hovered = args.hovered(7);
  const GuiRect& rect = args.rect();
  prepareGuiDrawState();
  drawVanillaToggle(*client, *client->textRenderer, rect.x, rect.y, rect.w, rect.h, label, enabled, hovered);
  return 0;
}
int luaGuiDrawCenteredText(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = guiDrawClient();
  if(client == nullptr) {
    return 0;
  }
  int x = 0;
  int y = 0;
  int width = 0;
  std::string text;
  std::uint32_t color = 0xFFFFFFFFU;
  if(api.type(state, 1) == kLuaTTable) {
    x = static_cast<int>(luaFloatField(state, 1, "x", 0.0f));
    y = static_cast<int>(luaFloatField(state, 1, "y", 0.0f));
    width = static_cast<int>(luaFloatField(state, 1, "width", luaFloatField(state, 1, "w", 0.0f)));
    text = luaStringField(state, 1, "text", "");
    color = luaArgb(state, 1, 0xFFFFFFFFU);
  } else {
    if(api.gettop(state) < 4 || api.type(state, 3) != kLuaTString) {
      return 0;
    }
    x = luaIntArg(state, 1);
    y = luaIntArg(state, 2);
    width = luaIntArg(state, 3);
    text = luaString(state, 4, "");
    color = api.gettop(state) >= 5 ? luaArgb(state, 5) : 0xFFFFFFFFU;
  }
  const int textWidth = client->textRenderer->getWidth(text);
  const int drawX = x + (width - textWidth) / 2;
  prepareGuiDrawState();
  client->textRenderer->draw(text, drawX, y, std::bit_cast<int>(color));
  return 0;
}
int luaGuiFillRect(lua_State* state) {
  if(!luaGuiDrawActive() || luaApi().gettop(state) < 5) {
    return 0;
  }
  drawGuiFillRect(luaIntArg(state, 1), luaIntArg(state, 2), luaIntArg(state, 3), luaIntArg(state, 4),
                  luaArgb(state, 5));
  return 0;
}
int luaGuiDrawText(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = guiDrawClient();
  if(client == nullptr || api.gettop(state) < 4 || api.type(state, 3) != kLuaTString) {
    return 0;
  }
  const std::string text = luaString(state, 3, "");
  const gl::preset::ModLuaGuiTextDraw textCaps;
  client->textRenderer->draw(text, luaIntArg(state, 1), luaIntArg(state, 2), std::bit_cast<int>(luaArgb(state, 4)));
  return 0;
}
int luaGuiDrawItem(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = guiDrawClient();
  if(client == nullptr || api.gettop(state) < 4) {
    return 0;
  }
  const int x = luaIntArg(state, 1);
  const int y = luaIntArg(state, 2);
  const int itemId = luaIntArg(state, 3);
  const int count = luaIntArg(state, 4);
  const int damage = luaIntArg(state, 5, 0);
  if(itemId <= 0 || count <= 0) {
    return 0;
  }
  net::minecraft::ItemStack stack(itemId, count, damage);
  const gl::preset::ModLuaGuiItemDraw itemCaps;
  g_luaItemRenderer.renderGuiItem(*client->textRenderer, client->textureManager, stack, x, y);
  g_luaItemRenderer.renderGuiItemDecoration(*client->textRenderer, client->textureManager, stack, x, y);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  return 0;
}
int luaGuiTextWidth(lua_State* state) {
  LuaApi& api = luaApi();
  const std::string text = luaString(state, 1, "");
  int width = 0;
  if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->textRenderer != nullptr) {
    width = client::Minecraft::INSTANCE->textRenderer->getWidth(text);
  }
  api.pushinteger(state, static_cast<long long>(width));
  return 1;
}
int luaGuiTextureId(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = client::Minecraft::INSTANCE;
  if(client == nullptr || api.type(state, 1) != kLuaTString) {
    api.pushinteger(state, 0);
    return 1;
  }
  const int textureId = client->textureManager.getTextureId(luaString(state, 1, ""));
  api.pushinteger(state, static_cast<long long>(textureId));
  return 1;
}
int luaGuiDrawSprite(lua_State* state) {
  LuaApi& api = luaApi();
  if(!luaGuiDrawActive() || api.gettop(state) < 7) {
    return 0;
  }
  client::Minecraft* client = client::Minecraft::INSTANCE;
  if(client == nullptr) {
    return 0;
  }
  int textureId = 0;
  int arg = 1;
  if(api.type(state, 1) == kLuaTString) {
    textureId = client->textureManager.getTextureId(luaString(state, 1, ""));
    arg = 2;
  } else {
    textureId = luaIntArg(state, 1);
  }
  if(textureId <= 0) {
    return 0;
  }
  const int x = luaIntArg(state, arg);
  const int y = luaIntArg(state, arg + 1);
  const int u = luaIntArg(state, arg + 2);
  const int v = luaIntArg(state, arg + 3);
  const int w = luaIntArg(state, arg + 4);
  const int h = luaIntArg(state, arg + 5);
  const gl::preset::ModLuaGuiSpriteDraw spriteCaps;
  gl::bindTexture(gl::cap::Texture2D, textureId);
  client::render::Tessellator& tess = client::render::Tessellator::INSTANCE;
  tess.startQuads();
  client::gui::draw::appendAtlasQuad(tess, x, y, u, v, w, h, 0.0f);
  tess.draw();
  return 0;
}
int luaGuiDrawTexture(lua_State* state) {
  if(!luaGuiDrawActive() || luaApi().gettop(state) < 5) {
    return 0;
  }
  const int textureId = luaIntArg(state, 1);
  const int x = luaIntArg(state, 2);
  const int y = luaIntArg(state, 3);
  const int w = luaIntArg(state, 4);
  const int h = luaIntArg(state, 5);
  if(textureId <= 0) {
    return 0;
  }
  const gl::preset::ModLuaGuiSpriteDraw spriteCaps;
  gl::bindTexture(gl::cap::Texture2D, textureId);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  client::render::Tessellator& tess = client::render::Tessellator::INSTANCE;
  client::gui::draw::texturedQuad(tess, x, y, x + w, y + h, 0.0f, 1.0f, 1.0f, 0.0f);
  return 0;
}
int luaScreenUiAddCenteredButton(lua_State* state) {
  ActiveScreenUi* session = activeScreenUi();
  if(session->context == nullptr || session->mod == nullptr || session->context->screen == nullptr) {
    return 0;
  }
  LuaApi& api = luaApi();
  if(api.gettop(state) < 2 || api.type(state, 1) != kLuaTNumber || api.type(state, 2) != kLuaTString) {
    return 0;
  }
  const int y = luaIntArg(state, 1);
  const std::string text = luaString(state, 2, "");
  const int ref = api.gettop(state) >= 3 ? retainButtonCallback(state, session->mod, 3) : kLuaNoRef;
  (void)session->context->addCenteredButton(y, text, [mod = session->mod, ref]() { invokeButtonCallback(mod, ref); });
  return 0;
}
int luaScreenUiAddButton(lua_State* state) {
  ActiveScreenUi* session = activeScreenUi();
  if(session->context == nullptr || session->mod == nullptr || session->context->screen == nullptr) {
    return 0;
  }
  LuaApi& api = luaApi();
  if(api.gettop(state) < 5 || api.type(state, 5) != kLuaTString) {
    return 0;
  }
  const int x = luaIntArg(state, 1);
  const int y = luaIntArg(state, 2);
  const int width = luaIntArg(state, 3);
  const int height = luaIntArg(state, 4);
  const std::string text = luaString(state, 5, "");
  const int ref = api.gettop(state) >= 6 ? retainButtonCallback(state, session->mod, 6) : kLuaNoRef;
  (void)session->context->addButton(x, y, width, height, text, [mod = session->mod, ref]() {
    invokeButtonCallback(mod, ref);
  });
  return 0;
}
int luaScreenUiAddStackedCenteredButton(lua_State* state) {
  ActiveScreenUi* session = activeScreenUi();
  if(session->context == nullptr || session->mod == nullptr || session->context->screen == nullptr ||
     session->context->stackedButtonY == nullptr) {
    return 0;
  }
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1 || api.type(state, 1) != kLuaTString) {
    return 0;
  }
  const std::string text = luaString(state, 1, "");
  const int ref = api.gettop(state) >= 2 ? retainButtonCallback(state, session->mod, 2) : kLuaNoRef;
  (void)session->context->addStackedCenteredButton(text, [mod = session->mod, ref]() {
    invokeButtonCallback(mod, ref);
  });
  session->stackedY = *session->context->stackedButtonY;
  return 0;
}
int luaScreenOpen(lua_State* state) {
  LuaApi& api = luaApi();
  if(client::Minecraft::INSTANCE == nullptr || api.type(state, 1) != kLuaTString) {
    return 0;
  }
  std::string title;
  if(api.type(state, 2) == kLuaTTable) {
    title = luaStringField(state, 2, "title", "");
  }
  client::Minecraft::INSTANCE->setScreen(
      std::make_unique<LuaScreen>(luaString(state, 1, ""), std::move(title)));
  return 0;
}
int luaScreenClose(lua_State* state) {
  (void)state;
  if(client::Minecraft::INSTANCE != nullptr) {
    client::Minecraft::INSTANCE->setScreen(nullptr);
  }
  return 0;
}
int luaScreenHostField(lua_State* state) {
  LuaApi& api = luaApi();
  std::string text;
  if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->currentScreen() != nullptr &&
     api.type(state, 1) == kLuaTString) {
    text = client::Minecraft::INSTANCE->currentScreen()->hostFieldText(luaString(state, 1, ""));
  }
  api.pushstring(state, text.c_str());
  return 1;
}
int luaScreenHostSetField(lua_State* state) {
  if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->currentScreen() != nullptr) {
    client::Minecraft::INSTANCE->currentScreen()->setHostFieldText(luaString(state, 1, ""), luaString(state, 2, ""));
  }
  return 0;
}
int luaScreenOpenHost(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTString) {
    return 0;
  }
  const std::string screenId = luaString(state, 1, "");
  std::unordered_map<std::string, std::string> fields;
  if(api.type(state, 2) == kLuaTTable) {
    api.pushnil(state);
    while(api.next(state, 2) != 0) {
      if(api.type(state, -2) == kLuaTString) {
        const char* key = api.tolstring(state, -2, nullptr);
        if(key != nullptr) {
          fields.emplace(key, luaString(state, -1, ""));
        }
      }
      api.settop(state, -2);
    }
  }
  (void)mod::openHostScreen(screenId, fields);
  return 0;
}
int luaScreenAddField(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen == nullptr || !session->initPhase || api.type(state, 1) != kLuaTString) {
    return 0;
  }
  const std::string name = luaString(state, 1, "");
  const int x = luaIntArg(state, 2);
  const int y = luaIntArg(state, 3);
  const int width = luaIntArg(state, 4);
  const int height = luaIntArg(state, 5, 20);
  std::string text;
  int maxLen = 0;
  bool numeric = false;
  bool allowSign = false;
  bool allowDot = false;
  if(api.type(state, 6) == kLuaTTable) {
    text = luaStringField(state, 6, "text", "");
    maxLen = luaIntField(state, 6, "max_len", 0);
    numeric = luaBoolField(state, 6, "numeric", false);
    allowSign = luaBoolField(state, 6, "signed", false);
    allowDot = luaBoolField(state, 6, "decimal", false);
  }
  session->screen->addField(name, x, y, width, height, text, maxLen, numeric, allowSign, allowDot);
  return 0;
}
int luaScreenFieldText(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  std::string text;
  if(session->screen != nullptr && api.type(state, 1) == kLuaTString) {
    if(client::gui::widget::TextFieldWidget* field = session->screen->field(luaString(state, 1, ""))) {
      text = field->getText();
    }
  }
  api.pushstring(state, text.c_str());
  return 1;
}
int luaScreenSetFieldText(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen != nullptr && api.type(state, 1) == kLuaTString) {
    if(client::gui::widget::TextFieldWidget* field = session->screen->field(luaString(state, 1, ""))) {
      field->setText(luaString(state, 2, ""));
    }
  }
  return 0;
}
int luaScreenSetFieldsVisible(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen != nullptr) {
    session->screen->setFieldsVisible(api.toboolean(state, 1) != 0);
  }
  return 0;
}
int luaScreenAddButton(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen == nullptr || !session->initPhase || api.gettop(state) < 5 ||
     api.type(state, 5) != kLuaTString) {
    return 0;
  }
  const int x = luaIntArg(state, 1);
  const int y = luaIntArg(state, 2);
  const int width = luaIntArg(state, 3);
  const int height = luaIntArg(state, 4);
  const std::string text = luaString(state, 5, "");
  const int ref = api.gettop(state) >= 6 ? retainButtonCallback(state, session->mod, 6) : kLuaNoRef;
  ModHost::LoadedLuaMod* mod = session->mod;
  session->screen->addLuaButton(x, y, width, height, text, [mod, ref]() { invokeButtonCallback(mod, ref); });
  return 0;
}
} // namespace
void pushScreenUiTable(lua_State* state) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 3);
  api.pushcclosure(state, luaScreenUiAddCenteredButton, 0);
  api.setfield(state, -2, "add_centered_button");
  api.pushcclosure(state, luaScreenUiAddButton, 0);
  api.setfield(state, -2, "add_button");
  api.pushcclosure(state, luaScreenUiAddStackedCenteredButton, 0);
  api.setfield(state, -2, "add_stacked_centered_button");
}
void pushHostFieldsTable(lua_State* state, client::gui::screen::Screen* screen) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 4);
  if(screen != nullptr) {
    screen->forEachHostField([state](std::string_view name, std::string_view value) {
      setField(state, std::string(name).c_str(), std::string(value));
    });
  }
}
void installScreenApi(lua_State* state) {
  LuaApi& api = luaApi();
  pushFunctionTable(state, {
                               {"fill_rect", luaGuiFillRect},
                               {"draw_text", luaGuiDrawText},
                               {"draw_item", luaGuiDrawItem},
                               {"text_width", luaGuiTextWidth},
                               {"texture_id", luaGuiTextureId},
                               {"draw_sprite", luaGuiDrawSprite},
                               {"draw_texture", luaGuiDrawTexture},
                               {"draw_button", luaGuiDrawButton},
                               {"draw_slider", luaGuiDrawSlider},
                               {"draw_toggle", luaGuiDrawToggle},
                               {"draw_centered_text", luaGuiDrawCenteredText},
                           });
  installLuaGuiViewportBindings(state);
  api.setfield(state, -2, "gui");
  pushFunctionTable(state, {
                               {"open", luaScreenOpen},
                               {"close", luaScreenClose},
                               {"host_field", luaScreenHostField},
                               {"host_set_field", luaScreenHostSetField},
                               {"open_host", luaScreenOpenHost},
                               {"add_field", luaScreenAddField},
                               {"field_text", luaScreenFieldText},
                               {"set_field_text", luaScreenSetFieldText},
                               {"add_button", luaScreenAddButton},
                               {"set_fields_visible", luaScreenSetFieldsVisible},
                           });
  api.createtable(state, 0, 36);
  setField(state, "login", std::string(screen_ids::kLogin));
  setField(state, "title", std::string(screen_ids::kTitle));
  setField(state, "game_menu", std::string(screen_ids::kGameMenu));
  setField(state, "multiplayer", std::string(screen_ids::kMultiplayer));
  setField(state, "connect", std::string(screen_ids::kConnect));
  setField(state, "disconnected", std::string(screen_ids::kDisconnected));
  setField(state, "downloading_terrain", std::string(screen_ids::kDownloadingTerrain));
  setField(state, "death", std::string(screen_ids::kDeath));
  setField(state, "chat", std::string(screen_ids::kChat));
  setField(state, "sleeping_chat", std::string(screen_ids::kSleepingChat));
  setField(state, "confirm", std::string(screen_ids::kConfirm));
  setField(state, "create_world", std::string(screen_ids::kCreateWorld));
  setField(state, "select_world", std::string(screen_ids::kSelectWorld));
  setField(state, "edit_world", std::string(screen_ids::kEditWorld));
  setField(state, "world_settings", std::string(screen_ids::kWorldSettings));
  setField(state, "world_save_conflict", std::string(screen_ids::kWorldSaveConflict));
  setField(state, "inventory", std::string(screen_ids::kInventory));
  setField(state, "crafting", std::string(screen_ids::kCrafting));
  setField(state, "dispenser", std::string(screen_ids::kDispenser));
  setField(state, "double_chest", std::string(screen_ids::kDoubleChest));
  setField(state, "furnace", std::string(screen_ids::kFurnace));
  setField(state, "sign_edit", std::string(screen_ids::kSignEdit));
  setField(state, "options", std::string(screen_ids::kOptions));
  setField(state, "video_options", std::string(screen_ids::kVideoOptions));
  setField(state, "detail_settings", std::string(screen_ids::kDetailSettings));
  setField(state, "keybinds", std::string(screen_ids::kKeybinds));
  setField(state, "mods", std::string(screen_ids::kMods));
  setField(state, "achievements", std::string(screen_ids::kAchievements));
  setField(state, "stats", std::string(screen_ids::kStats));
  setField(state, "lan", std::string(screen_ids::kLan));
  setField(state, "lan_info", std::string(screen_ids::kLanInfo));
  setField(state, "server_mod_download", std::string(screen_ids::kServerModDownload));
  setField(state, "fatal_error", std::string(screen_ids::kFatalError));
  setField(state, "out_of_memory", std::string(screen_ids::kOutOfMemory));
  api.setfield(state, -2, "ids");
  api.createtable(state, 0, 3);
  setField(state, "footer", std::string(screen_regions::kFooter));
  setField(state, "screen", std::string(screen_regions::kScreen));
  setField(state, "side_panel", std::string(screen_regions::kSidePanel));
  api.setfield(state, -2, "regions");
  api.setfield(state, -2, "screen");
  pushFunctionTable(state, {
                               {"set_offline_username", luaSessionSetOfflineUsername},
                               {"clear_offline_username", luaSessionClearOfflineUsername},
                               {"is_offline_mode", luaSessionIsOfflineMode},
                               {"get_offline_username", luaSessionGetOfflineUsername},
                               {"get_username", luaSessionGetUsername},
                               {"is_authenticated", luaSessionIsAuthenticated},
                           });
  api.setfield(state, -2, "session");
}
#else
void installScreenApi(lua_State* state) {
  (void)state;
}
#endif
} // namespace net::minecraft::mod::runtime
