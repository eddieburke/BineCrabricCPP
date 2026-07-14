#include "net/minecraft/client/platform/FileDialog.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>
#endif
namespace net::minecraft::client::platform {
std::optional<std::filesystem::path> pickJsonFile() {
  return pickFile("JSON files", "*.json");
}
std::optional<std::filesystem::path> pickFile(std::string_view filterLabel, std::string_view filterPattern) {
#ifdef _WIN32
  char filename[MAX_PATH] = {};
  const std::string label(filterLabel);
  const std::string pattern(filterPattern);
  std::string filter = label;
  filter.push_back('\0');
  filter.append(pattern);
  filter.push_back('\0');
  filter.append("All files");
  filter.push_back('\0');
  filter.append("*.*");
  filter.push_back('\0');
  OPENFILENAMEA dialog = {};
  dialog.lStructSize = sizeof(dialog);
  dialog.hwndOwner = nullptr;
  dialog.lpstrFilter = filter.c_str();
  dialog.lpstrFile = filename;
  dialog.nMaxFile = MAX_PATH;
  dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
  if(!GetOpenFileNameA(&dialog)) {
    return std::nullopt;
  }
  return std::filesystem::path(filename);
#else
  (void)filterLabel;
  (void)filterPattern;
  return std::nullopt;
#endif
}
} // namespace net::minecraft::client::platform
