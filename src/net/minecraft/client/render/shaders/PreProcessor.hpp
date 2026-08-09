#pragma once
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace net::minecraft::client::render {
struct PPMacro {
 bool functionLike = false;
 std::vector<std::string> params;
 std::string body;
 bool defined = true;
};
struct PPMacroTableHash {
 using is_transparent = void;
 std::size_t operator()(std::string_view value) const noexcept {
  return std::hash<std::string_view>{}(value);
 }
};
using PPMacroTable = std::unordered_map<std::string, PPMacro, PPMacroTableHash, std::equal_to<>>;
class PPMacroOverlay {
 public:
  explicit PPMacroOverlay(const PPMacroTable& base) : base_(base) {}
  const PPMacro* find(std::string_view name) const {
   if(const auto it = extra_.find(name); it != extra_.end()) return it->second.defined ? &it->second : nullptr;
   if(const auto it = base_.find(name); it != base_.end()) return &it->second;
   return nullptr;
  }
  bool contains(std::string_view name) const {
   return find(name) != nullptr;
  }
  PPMacro& operator[](std::string_view name) {
   return extra_[std::string(name)];
  }
  void erase(std::string_view name) {
   PPMacro tombstone;
   tombstone.defined = false;
   extra_[std::string(name)] = std::move(tombstone);
  }

 private:
  const PPMacroTable& base_;
  PPMacroTable extra_;
};
inline const PPMacro* ppFind(const PPMacroTable& macros, std::string_view name) {
 const auto it = macros.find(name);
 return it == macros.end() ? nullptr : &it->second;
}
inline bool ppContains(const PPMacroTable& macros, std::string_view name) {
 return macros.contains(name);
}
inline void ppAssign(PPMacroTable& macros, std::string_view name, PPMacro macro) {
 macros[std::string(name)] = std::move(macro);
}
inline void ppErase(PPMacroTable& macros, std::string_view name) {
 macros.erase(std::string(name));
}
inline const PPMacro* ppFind(const PPMacroOverlay& macros, std::string_view name) {
 return macros.find(name);
}
inline bool ppContains(const PPMacroOverlay& macros, std::string_view name) {
 return macros.contains(name);
}
inline void ppAssign(PPMacroOverlay& macros, std::string_view name, PPMacro macro) {
 macros[std::string(name)] = std::move(macro);
}
inline void ppErase(PPMacroOverlay& macros, std::string_view name) {
 macros.erase(name);
}
inline std::string_view trimmedView(std::string_view value) {
 const std::size_t first = value.find_first_not_of(" \t\r\n");
 if(first == std::string_view::npos) return {};
 const std::size_t last = value.find_last_not_of(" \t\r\n");
 return value.substr(first, last - first + 1);
}

bool isIdentStart(char c);
bool isIdentChar(char c);
std::string lineForDirectiveParse(const std::string& line);
bool evalExpressionText(std::string_view text, double& out);
template <typename Macros>
std::string resolveDefinedOperators(std::string_view expr, const Macros& macros) {
 std::string out;
 out.reserve(expr.size());
 for(std::size_t i = 0; i < expr.size();) {
  if(isIdentStart(expr[i])) {
   std::size_t end = i;
   while(end < expr.size() && isIdentChar(expr[end])) ++end;
   const std::string_view ident(expr.data() + i, end - i);
   if(ident == "defined") {
    std::size_t p = end;
    while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
    bool paren = false;
    if(p < expr.size() && expr[p] == '(') {
     paren = true;
     ++p;
     while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
    }
    std::size_t nameEnd = p;
    while(nameEnd < expr.size() && isIdentChar(expr[nameEnd])) ++nameEnd;
    const std::string_view name(expr.data() + p, nameEnd - p);
    p = nameEnd;
    if(paren) {
     while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
     if(p < expr.size() && expr[p] == ')') ++p;
    }
    out += ppContains(macros, name) ? '1' : '0';
    i = p;
    continue;
   }
   out += ident;
   i = end;
   continue;
  }
  out += expr[i++];
 }
 return out;
}
template <typename Macros>
std::string expandIfExpression(std::string_view rawExpr, const Macros& macros) {
 std::string expr(rawExpr);
 for(int pass = 0; pass < 64; ++pass) {
  bool changed = false;
  std::string out;
  out.reserve(expr.size());
  for(std::size_t i = 0; i < expr.size();) {
   if(!isIdentStart(expr[i])) {
    out += expr[i++];
    continue;
   }
   std::size_t end = i;
   while(end < expr.size() && isIdentChar(expr[end])) ++end;
   const std::string_view ident(expr.data() + i, end - i);
   const PPMacro* found = ppFind(macros, ident);
   if(found == nullptr) {
    out += ident;
    i = end;
    continue;
   }
   const PPMacro& macro = *found;
   if(!macro.functionLike) {
    out += '(';
    out += macro.body.empty() ? "1" : macro.body;
    out += ')';
    i = end;
    changed = true;
    continue;
   }
   std::size_t p = end;
   while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
   if(p >= expr.size() || expr[p] != '(') {
    out += ident;
    i = end;
    continue;
   }
   ++p;
   std::vector<std::string> args;
   std::string current;
   int depth = 1;
   while(p < expr.size() && depth > 0) {
    const char c = expr[p];
    if(c == '(')
     ++depth;
    else if(c == ')') {
     --depth;
     if(depth == 0) break;
    } else if(c == ',' && depth == 1) {
     args.push_back(std::string(trimmedView(current)));
     current.clear();
     ++p;
     continue;
    }
    current += c;
    ++p;
   }
   args.push_back(std::string(trimmedView(current)));
   if(p < expr.size()) ++p;
   std::string substituted;
   const std::string& body = macro.body;
   for(std::size_t b = 0; b < body.size();) {
    if(!isIdentStart(body[b])) {
     substituted += body[b++];
     continue;
    }
    std::size_t be = b;
    while(be < body.size() && isIdentChar(body[be])) ++be;
    const std::string_view bodyIdent(body.data() + b, be - b);
    bool replacedParam = false;
    for(std::size_t a = 0; a < macro.params.size() && a < args.size(); ++a) {
     if(macro.params[a] == bodyIdent) {
      substituted += '(' + args[a] + ')';
      replacedParam = true;
      break;
     }
    }
    if(!replacedParam) substituted += bodyIdent;
    b = be;
   }
   out += '(' + substituted + ')';
   i = p;
   changed = true;
  }
  expr = std::move(out);
  if(!changed) break;
 }
 return expr;
}
template <typename Macros>
bool evaluateIfExpression(std::string_view rawExpr, const Macros& macros) {
 const std::string resolved = resolveDefinedOperators(rawExpr, macros);
 const std::string expanded = [&] {
  for(const char c : resolved)
   if(isIdentStart(c)) return expandIfExpression(resolved, macros);
  return resolved;
 }();
 double value = 0.0;
 if(!evalExpressionText(expanded, value)) return false;
 return value != 0.0;
}
template <typename Macros>
void parseDefineDirective(std::string_view afterKeyword, Macros& macros) {
 std::size_t i = 0;
 while(i < afterKeyword.size() && std::isspace(static_cast<unsigned char>(afterKeyword[i]))) ++i;
 std::size_t end = i;
 while(end < afterKeyword.size() && isIdentChar(afterKeyword[end])) ++end;
 if(end == i) return;
 const std::string name(afterKeyword.substr(i, end - i));
 PPMacro macro;
 if(end < afterKeyword.size() && afterKeyword[end] == '(') {
  macro.functionLike = true;
  std::size_t p = end + 1;
  std::string param;
  while(p < afterKeyword.size() && afterKeyword[p] != ')') {
   const char c = afterKeyword[p];
   if(c == ',') {
    if(!trimmedView(param).empty()) macro.params.push_back(std::string(trimmedView(param)));
    param.clear();
   } else {
    param += c;
   }
   ++p;
  }
  if(!trimmedView(param).empty()) macro.params.push_back(std::string(trimmedView(param)));
  end = p < afterKeyword.size() ? p + 1 : p;
 }
 std::string body(trimmedView(afterKeyword.substr(end)));
 const std::size_t comment = body.find("//");
 if(comment != std::string::npos) body = std::string(trimmedView(std::string_view(body).substr(0, comment)));
 macro.body = std::move(body);
 ppAssign(macros, name, std::move(macro));
}
bool parseDirective(const std::string& trimmed, std::string& keyword, std::string& rest);
void seedMacrosFromDefines(const std::string& text, PPMacroTable& macros);
extern template bool evaluateIfExpression(std::string_view, const PPMacroTable&);
extern template void parseDefineDirective(std::string_view, PPMacroTable&);
}
