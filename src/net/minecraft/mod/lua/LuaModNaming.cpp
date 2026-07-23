#include "net/minecraft/mod/lua/LuaModNaming.hpp"
#include <cctype>
namespace net::minecraft::mod::lua {
std::string toSnakeCase(std::string_view raw) {
 std::string out;
 out.reserve(raw.size() + 4);
 for(std::size_t i = 0; i < raw.size(); ++i) {
  const unsigned char ch = static_cast<unsigned char>(raw[i]);
  if(ch == '_' || ch == '-' || ch == ' ') {
   if(!out.empty() && out.back() != '_') {
    out.push_back('_');
   }
   continue;
  }
  if(i > 0) {
   const unsigned char prev = static_cast<unsigned char>(raw[i - 1]);
   if(std::islower(prev) != 0 && std::isupper(ch) != 0) {
    if(!out.empty() && out.back() != '_') {
     out.push_back('_');
    }
   }
  }
  out.push_back(static_cast<char>(std::tolower(ch)));
 }
 while(!out.empty() && out.back() == '_') {
  out.pop_back();
 }
 return out;
}
std::string humanizeTranslationKey(std::string_view raw) {
 std::string out;
 out.reserve(raw.size() + 8);
 for(std::size_t i = 0; i < raw.size(); ++i) {
  const unsigned char ch = static_cast<unsigned char>(raw[i]);
  if(ch == '_' || ch == '-' || ch == '.') {
   if(!out.empty() && out.back() != ' ') {
    out.push_back(' ');
   }
   continue;
  }
  if(i > 0) {
   const unsigned char prev = static_cast<unsigned char>(raw[i - 1]);
   if(std::islower(prev) != 0 && std::isupper(ch) != 0) {
    if(!out.empty() && out.back() != ' ') {
     out.push_back(' ');
    }
   }
  }
  if(out.empty() || out.back() == ' ') {
   out.push_back(static_cast<char>(std::toupper(ch)));
  } else {
   out.push_back(static_cast<char>(ch));
  }
 }
 return out;
}
} // namespace net::minecraft::mod::lua
