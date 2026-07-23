#pragma once
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/resource/language/TranslationStorage.hpp"
namespace net::minecraft::client::resource::language {
class I18n {
 public:
 static void setTranslations(const TranslationStorage* translations) noexcept {
  translations_ = translations;
 }
 // Mods call this in their registerClass() to supply display names.
 // Key must include the ".name" suffix (e.g. "tile.coral.name").
 static void addTranslation(const std::string& key, std::string value) {
  modTranslations_[key] = std::move(value);
 }
 [[nodiscard]] static std::string getTranslation(std::string_view name) {
  const auto modIt = modTranslations_.find(std::string(name));
  if(modIt != modTranslations_.end()) {
   return modIt->second;
  }
  if(translations_ == nullptr) {
   return std::string(name);
  }
  return translations_->get(name);
 }
 [[nodiscard]] static std::string getClientTranslation(std::string_view name) {
  const std::string withSuffix = std::string(name) + ".name";
  const auto modIt = modTranslations_.find(withSuffix);
  if(modIt != modTranslations_.end()) {
   return modIt->second;
  }
  if(translations_ == nullptr) {
   return {};
  }
  return translations_->getClientTranslation(name);
 }
 // Java: getTranslation(String name, Object... args) -> String.format(value, args).
 // The format string is the resolved translation; args fill its %-conversions
 // (only %s / %d appear in vanilla beta 1.7.3 keys), left to right.
 template <typename... Args>
 [[nodiscard]] static std::string getTranslation(std::string_view name, Args&&... args) {
  const std::string format = getTranslation(name);
  return formatJava(format, {toStringArg(std::forward<Args>(args))...});
 }
 // Java String.format: supports %s, %d, and positional %1$s / %2$d forms.
 [[nodiscard]] static std::string formatJava(const std::string& format, std::initializer_list<std::string> args) {
  return javaFormat(format, std::vector<std::string>(args));
 }
 [[nodiscard]] static std::string formatJava(const std::string& format, const std::vector<std::string>& args) {
  return javaFormat(format, args);
 }

 private:
 template <typename T>
 [[nodiscard]] static std::string toStringArg(T&& value) {
  std::ostringstream out;
  out << std::forward<T>(value);
  return out.str();
 }
 [[nodiscard]] static std::string javaFormat(const std::string& format, const std::vector<std::string>& args) {
  std::string result;
  result.reserve(format.size());
  std::size_t nextSequential = 0;
  for(std::size_t i = 0; i < format.size(); ++i) {
   if(format[i] != '%' || i + 1 >= format.size()) {
    result.push_back(format[i]);
    continue;
   }
   if(format[i + 1] == '%') {
    result.push_back('%');
    ++i;
    continue;
   }
   std::size_t cursor = i + 1;
   std::size_t argIndex = 0;
   bool hasIndex = false;
   while(cursor < format.size() && format[cursor] >= '0' && format[cursor] <= '9') {
    hasIndex = true;
    argIndex = argIndex * 10 + static_cast<std::size_t>(format[cursor] - '0');
    ++cursor;
   }
   if(hasIndex && cursor < format.size() && format[cursor] == '$') {
    ++cursor;
   } else {
    argIndex = nextSequential + 1;
    cursor = i + 1;
   }
   const char conv = format[cursor];
   if(conv == 's' || conv == 'd') {
    const std::size_t selected = argIndex > 0 ? argIndex - 1 : 0;
    if(selected < args.size()) {
     result += args[selected];
    }
    if(!hasIndex) {
     ++nextSequential;
    }
    i = cursor;
    continue;
   }
   result.push_back(format[i]);
  }
  return result;
 }
 inline static const TranslationStorage* translations_ = nullptr;
 inline static std::unordered_map<std::string, std::string> modTranslations_;
};
} // namespace net::minecraft::client::resource::language
