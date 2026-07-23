#include "net/minecraft/server/dedicated/gui/LogHandler.hpp"
#include <memory>
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
struct LogAppendMessage {
 LogHandler* handler = nullptr;
 std::wstring text;
};
} // namespace
LogHandler::LogHandler(HWND logEditHwnd) : logEditHwnd_(logEditHwnd) {
}
void LogHandler::detach() noexcept {
 logEditHwnd_ = nullptr;
}
std::wstring LogHandler::formatRecord(const LogRecord& record) {
 std::string formatted;
 switch(record.level) {
 case LogLevel::Finest:
  formatted += "[FINEST] ";
  break;
 case LogLevel::Finer:
  formatted += "[FINER] ";
  break;
 case LogLevel::Fine:
  formatted += "[FINE] ";
  break;
 case LogLevel::Info:
  formatted += "[INFO] ";
  break;
 case LogLevel::Warning:
  formatted += "[WARNING] ";
  break;
 case LogLevel::Severe:
  formatted += "[SEVERE] ";
  break;
 }
 formatted += record.message;
 formatted += '\n';
 if(record.thrown != nullptr) {
  formatted += record.thrown->what();
 }
 return utf8ToWide(formatted);
}
void LogHandler::publish(const LogRecord& record) {
 if(logEditHwnd_ == nullptr) {
  return;
 }
 auto* payload = new LogAppendMessage{this, formatRecord(record)};
 if(!PostMessageW(logEditHwnd_, WM_APP_APPEND_LOG, 0, reinterpret_cast<LPARAM>(payload))) {
  delete payload;
 }
}
void LogHandler::applyAppend(HWND logEditHwnd, const std::wstring& text) {
 if(lineCharCounts_[bufferIndex_] != 0) {
  const int length = GetWindowTextLengthW(logEditHwnd);
  const int trimChars = lineCharCounts_[bufferIndex_];
  if(length > trimChars) {
   std::wstring current(static_cast<std::size_t>(length), L'\0');
   GetWindowTextW(logEditHwnd, current.data(), length + 1);
   current.erase(0, static_cast<std::size_t>(trimChars));
   SetWindowTextW(logEditHwnd, current.c_str());
  } else {
   SetWindowTextW(logEditHwnd, L"");
  }
 }
 if(!text.empty()) {
  SendMessageW(logEditHwnd, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
  SendMessageW(logEditHwnd, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
  SendMessageW(logEditHwnd, EM_SCROLLCARET, 0, 0);
 }
 lineCharCounts_[bufferIndex_] = static_cast<int>(text.size());
 bufferIndex_ = (bufferIndex_ + 1) % 1024;
}
LRESULT handleLogEditMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
 if(message != LogHandler::WM_APP_APPEND_LOG) {
  return DefWindowProcW(hwnd, message, wParam, lParam);
 }
 auto* payload = reinterpret_cast<LogAppendMessage*>(lParam);
 if(payload == nullptr) {
  return 0;
 }
 std::unique_ptr<LogAppendMessage> owned(payload);
 if(payload->handler != nullptr) {
  payload->handler->applyAppend(hwnd, payload->text);
 }
 return 0;
}
} // namespace net::minecraft::server::dedicated::gui
