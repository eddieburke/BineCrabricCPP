#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::render::glutil {
struct PPMacro {
 bool functionLike = false;
 std::vector<std::string> params;
 std::string body;
};
using PPMacroTable = std::unordered_map<std::string, PPMacro>;

bool isIdentStart(char c);
bool isIdentChar(char c);
std::string lineForDirectiveParse(const std::string& line);
bool evaluateIfExpression(const std::string& rawExpr, const PPMacroTable& macros);
void parseDefineDirective(const std::string& afterKeyword, PPMacroTable& macros);
bool parseDirective(const std::string& trimmed, std::string& keyword, std::string& rest);
void seedMacrosFromDefines(const std::string& text, PPMacroTable& macros);
}
