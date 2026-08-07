#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::render {
struct PPMacro {
 bool functionLike = false;
 std::vector<std::string> params;
 std::string body;
};
struct PPMacroTableHash {
 using is_transparent = void;
 std::size_t operator()(std::string_view value) const noexcept {
  return std::hash<std::string_view>{}(value);
 }
};
using PPMacroTable = std::unordered_map<std::string, PPMacro, PPMacroTableHash, std::equal_to<>>;

bool isIdentStart(char c);
bool isIdentChar(char c);
std::string lineForDirectiveParse(const std::string& line);
bool evaluateIfExpression(std::string_view rawExpr, const PPMacroTable& macros);
void parseDefineDirective(std::string_view afterKeyword, PPMacroTable& macros);
bool parseDirective(const std::string& trimmed, std::string& keyword, std::string& rest);
void seedMacrosFromDefines(const std::string& text, PPMacroTable& macros);
}
