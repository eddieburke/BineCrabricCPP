#include "net/minecraft/server/dedicated/gui/PlayerListGui.hpp"
#include <memory>
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
namespace net::minecraft::server::dedicated::gui {
namespace {
std::wstring utf8ToWide(const std::string& text) {
 if(text.empty()) {
  return {};
 }
 const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
 if(length <= 0) {
  return {};
 }
 std::wstring wide(static_cast<std::size_t>(length), L'\0');
 MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), length);
 return wide;
}
struct PlayerListMessage {
 std::vector<std::wstring> names;
};
} // namespace
PlayerListGui::PlayerListGui(MinecraftServer& server) : server_(server) {
 server_.addTickable(this);
}
PlayerListGui::~PlayerListGui() = default;
HWND PlayerListGui::create(HWND parent, HINSTANCE instance, int x, int y, int width, int height) {
 listboxHwnd_ = CreateWindowExW(WS_EX_CLIENTEDGE,
                                L"LISTBOX",
                                L"",
                                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
                                x,
                                y,
                                width,
                                height,
                                parent,
                                nullptr,
                                instance,
                                nullptr);
 return listboxHwnd_;
}
void PlayerListGui::tick() {
 if(listboxHwnd_ == nullptr) {
  return;
 }
 if(tick_++ % 20 != 0) {
  return;
 }
 auto* payload = new PlayerListMessage{};
 payload->names.reserve(server_.playerManager.players.size());
 for(const ::net::minecraft::entity::player::ServerPlayerEntity* player : server_.playerManager.players) {
  if(player != nullptr) {
   payload->names.push_back(utf8ToWide(player->name));
  }
 }
 if(!PostMessageW(listboxHwnd_, WM_APP_REFRESH_PLAYERS, 0, reinterpret_cast<LPARAM>(payload))) {
  delete payload;
 }
}
void PlayerListGui::applyPlayerNames(const std::vector<std::wstring>& names) {
 if(listboxHwnd_ == nullptr) {
  return;
 }
 SendMessageW(listboxHwnd_, LB_RESETCONTENT, 0, 0);
 for(const std::wstring& name : names) {
  SendMessageW(listboxHwnd_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
 }
}
LRESULT handlePlayerListMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
 if(message != PlayerListGui::WM_APP_REFRESH_PLAYERS) {
  return DefWindowProcW(hwnd, message, wParam, lParam);
 }
 auto* payload = reinterpret_cast<PlayerListMessage*>(lParam);
 if(payload == nullptr) {
  return 0;
 }
 std::unique_ptr<PlayerListMessage> owned(payload);
 SendMessageW(hwnd, LB_RESETCONTENT, 0, 0);
 for(const std::wstring& name : payload->names) {
  SendMessageW(hwnd, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
 }
 return 0;
}
} // namespace net::minecraft::server::dedicated::gui
