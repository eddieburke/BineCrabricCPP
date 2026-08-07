#include "net/minecraft/client/render/shaders/PreProcessor.hpp"
#include <cctype>
#include <charconv>
#include <cstring>
#include <system_error>
#include <utility>
namespace net::minecraft::client::render {
namespace {
std::string_view trimmedView(std::string_view value) {
 const std::size_t first = value.find_first_not_of(" \t\r\n");
 if(first == std::string_view::npos) return {};
 const std::size_t last = value.find_last_not_of(" \t\r\n");
 return value.substr(first, last - first + 1);
}
} // namespace
bool isIdentStart(char c) {
 return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
bool isIdentChar(char c) {
 return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}
std::string lineForDirectiveParse(const std::string& line) {
 std::string out;
 out.reserve(line.size());
 for(std::size_t i = 0; i < line.size();) {
  if(line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') break;
  if(line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*') {
   const std::size_t close = line.find("*/", i + 2);
   if(close == std::string::npos) break;
   i = close + 2;
   out += ' ';
   continue;
  }
  out += line[i++];
 }
 return out;
}
class PPExpressionEval {
 public:
 explicit PPExpressionEval(std::string_view s) : s_(s) {}
 bool eval(double& out) {
  pos_ = 0;
  ok_ = true;
  out = ternary();
  skipWs();
  if(pos_ < s_.size()) ok_ = false;
  return ok_;
 }

 private:
 std::string_view s_;
 std::size_t pos_ = 0;
 bool ok_ = true;
 void skipWs() {
  while(pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_;
 }
 char peek() {
  skipWs();
  return pos_ < s_.size() ? s_[pos_] : '\0';
 }
 bool match(const char* tok) {
  skipWs();
  const std::size_t n = std::strlen(tok);
  if(s_.compare(pos_, n, tok) == 0) {
   pos_ += n;
   return true;
  }
  return false;
 }
 static long long toInt(double v) {
  return static_cast<long long>(v);
 }
 double ternary() {
  double c = lor();
  if(peek() == '?') {
   ++pos_;
   const double a = ternary();
   if(peek() == ':')
    ++pos_;
   else
    ok_ = false;
   const double b = ternary();
   return c != 0.0 ? a : b;
  }
  return c;
 }
 double lor() {
  double v = land();
  while(true) {
   skipWs();
   if(s_.compare(pos_, 2, "||") == 0) {
    pos_ += 2;
    const double r = land();
    v = (v != 0.0 || r != 0.0) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double land() {
  double v = bitor_();
  while(true) {
   skipWs();
   if(s_.compare(pos_, 2, "&&") == 0) {
    pos_ += 2;
    const double r = bitor_();
    v = (v != 0.0 && r != 0.0) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double bitor_() {
  double v = bitxor_();
  while(true) {
   skipWs();
   if(pos_ < s_.size() && s_[pos_] == '|' && (pos_ + 1 >= s_.size() || s_[pos_ + 1] != '|')) {
    ++pos_;
    v = static_cast<double>(toInt(v) | toInt(bitxor_()));
   } else
    break;
  }
  return v;
 }
 double bitxor_() {
  double v = bitand_();
  while(peek() == '^') {
   ++pos_;
   v = static_cast<double>(toInt(v) ^ toInt(bitand_()));
  }
  return v;
 }
 double bitand_() {
  double v = equality();
  while(true) {
   skipWs();
   if(pos_ < s_.size() && s_[pos_] == '&' && (pos_ + 1 >= s_.size() || s_[pos_ + 1] != '&')) {
    ++pos_;
    v = static_cast<double>(toInt(v) & toInt(equality()));
   } else
    break;
  }
  return v;
 }
 double equality() {
  double v = relational();
  while(true) {
   if(match("==")) {
    v = (v == relational()) ? 1.0 : 0.0;
   } else if(match("!=")) {
    v = (v != relational()) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double relational() {
  double v = shift();
  while(true) {
   skipWs();
   if(s_.compare(pos_, 2, "<=") == 0) {
    pos_ += 2;
    v = (v <= shift()) ? 1.0 : 0.0;
   } else if(s_.compare(pos_, 2, ">=") == 0) {
    pos_ += 2;
    v = (v >= shift()) ? 1.0 : 0.0;
   } else if(pos_ < s_.size() && s_[pos_] == '<' && s_.compare(pos_, 2, "<<") != 0) {
    ++pos_;
    v = (v < shift()) ? 1.0 : 0.0;
   } else if(pos_ < s_.size() && s_[pos_] == '>' && s_.compare(pos_, 2, ">>") != 0) {
    ++pos_;
    v = (v > shift()) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double shift() {
  double v = additive();
  while(true) {
   if(match("<<")) {
    v = static_cast<double>(toInt(v) << toInt(additive()));
   } else if(match(">>")) {
    v = static_cast<double>(toInt(v) >> toInt(additive()));
   } else
    break;
  }
  return v;
 }
 double additive() {
  double v = multiplicative();
  while(true) {
   const char c = peek();
   if(c == '+') {
    ++pos_;
    v += multiplicative();
   } else if(c == '-') {
    ++pos_;
    v -= multiplicative();
   } else
    break;
  }
  return v;
 }
 double multiplicative() {
  double v = unary();
  while(true) {
   const char c = peek();
   if(c == '*') {
    ++pos_;
    v *= unary();
   } else if(c == '/') {
    ++pos_;
    const double r = unary();
    v = r != 0.0 ? v / r : 0.0;
   } else if(c == '%') {
    ++pos_;
    const long long r = toInt(unary());
    v = static_cast<double>(r != 0 ? toInt(v) % r : 0);
   } else
    break;
  }
  return v;
 }
 double unary() {
  const char c = peek();
  if(c == '!') {
   ++pos_;
   return unary() == 0.0 ? 1.0 : 0.0;
  }
  if(c == '~') {
   ++pos_;
   return static_cast<double>(~toInt(unary()));
  }
  if(c == '-') {
   ++pos_;
   return -unary();
  }
  if(c == '+') {
   ++pos_;
   return unary();
  }
  return primary();
 }
 double primary() {
  skipWs();
  if(pos_ >= s_.size()) {
   ok_ = false;
   return 0.0;
  }
  const char c = s_[pos_];
  if(c == '(') {
   ++pos_;
   const double v = ternary();
   if(peek() == ')')
    ++pos_;
   else
    ok_ = false;
   return v;
  }
   if(std::isdigit(static_cast<unsigned char>(c)) ||
      (c == '.' && pos_ + 1 < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_ + 1])))) {
    double value = 0.0;
    const auto [ptr, error] = std::from_chars(s_.data() + pos_, s_.data() + s_.size(), value);
    if(error != std::errc()) {
     ok_ = false;
     return 0.0;
    }
    pos_ = static_cast<std::size_t>(ptr - s_.data());
    while(pos_ < s_.size() &&
          (s_[pos_] == 'u' || s_[pos_] == 'U' || s_[pos_] == 'l' || s_[pos_] == 'L' || s_[pos_] == 'f' ||
           s_[pos_] == 'F'))
     ++pos_;
    return value;
   }
  if(isIdentStart(c)) {
   std::size_t end = pos_;
   while(end < s_.size() && isIdentChar(s_[end])) ++end;
   const std::string_view ident = s_.substr(pos_, end - pos_);
   pos_ = end;
   if(ident == "true") return 1.0;
   return 0.0;
  }
  ok_ = false;
  ++pos_;
  return 0.0;
 }
};
namespace {
std::string resolveDefinedOperators(const std::string& expr, const PPMacroTable& macros) {
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
    out += macros.count(name) > 0 ? '1' : '0';
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
std::string expandIfExpression(std::string_view rawExpr, const PPMacroTable& macros) {
 std::string expr(rawExpr);
 expr = resolveDefinedOperators(expr, macros);
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
   const auto found = macros.find(ident);
   if(found == macros.end()) {
    out += ident;
    i = end;
    continue;
   }
   const PPMacro& macro = found->second;
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
} // namespace
bool evaluateIfExpression(std::string_view rawExpr, const PPMacroTable& macros) {
 const std::string expanded = expandIfExpression(rawExpr, macros);
 PPExpressionEval eval(expanded);
 double value = 0.0;
 if(!eval.eval(value)) return false;
 return value != 0.0;
}
void parseDefineDirective(std::string_view afterKeyword, PPMacroTable& macros) {
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
 // The option comment (`//[128 192 ...]`) must not leak into the macro body:
 // `#if COLORED_LIGHTING > 0` would then evaluate `0 //[128 192 ...]` as a
 // division chain and always come out zero.
 const std::size_t comment = body.find("//");
 if(comment != std::string::npos) body = std::string(trimmedView(std::string_view(body).substr(0, comment)));
 macro.body = std::move(body);
 macros[name] = std::move(macro);
}
bool parseDirective(const std::string& trimmed, std::string& keyword, std::string& rest) {
 if(trimmed.empty() || trimmed[0] != '#') return false;
 std::size_t i = 1;
 while(i < trimmed.size() && (trimmed[i] == ' ' || trimmed[i] == '\t')) ++i;
 std::size_t end = i;
 while(end < trimmed.size() && std::isalpha(static_cast<unsigned char>(trimmed[end]))) ++end;
 keyword = trimmed.substr(i, end - i);
 rest = std::string(trimmedView(trimmed.substr(end)));
 return true;
}
// PARSING ONLY — this function knows nothing about what the engine defines, and must
// not learn. It used to open with ~16 hardcoded macros (MC_GL_VERSION 460, every
// IRIS_FEATURE_* on, GL_ARB_gpu_shader5 and GL_ARB_shader_image_load_store asserted
// present regardless of the driver) which were a second, hand-maintained copy of what
// versionPreambleForStages() emits. Callers that had the real preamble silently
// overwrote them; the one caller that passed an empty string got the literals and
// scanned pack constants under a different `#if` branch than the GPU compiled.
// seedEngineMacros() in SourceProcessor.cpp is now the single source; see
// SourceProcessor.hpp.
void seedMacrosFromDefines(const std::string& text, PPMacroTable& macros) {
 std::size_t lineStart = 0;
 while(lineStart < text.size()) {
  const std::size_t lineEnd = text.find('\n', lineStart);
  const std::size_t lineLen = lineEnd == std::string::npos ? text.size() - lineStart : lineEnd - lineStart;
  const std::string_view line(text.data() + lineStart, lineLen);
  lineStart = lineEnd == std::string::npos ? text.size() : lineEnd + 1;
  const std::string trimmed(trimmedView(lineForDirectiveParse(std::string(line))));
  std::string keyword, rest;
  if(!parseDirective(trimmed, keyword, rest)) continue;
  if(keyword == "define") parseDefineDirective(rest, macros);
  if(keyword == "version") {
   const std::string_view restView(rest);
   std::size_t p = 0;
   while(p < restView.size() && std::isspace(static_cast<unsigned char>(restView[p]))) ++p;
   int version = 0;
   const auto [ptr, error] = std::from_chars(restView.data() + p, restView.data() + restView.size(), version);
   if(error == std::errc() && ptr != restView.data() + p) {
    PPMacro macro;
    macro.body = std::to_string(version);
    macros["__VERSION__"] = std::move(macro);
    p = static_cast<std::size_t>(ptr - restView.data());
    while(p < restView.size() && std::isspace(static_cast<unsigned char>(restView[p]))) ++p;
    std::size_t profileEnd = p;
    while(profileEnd < restView.size() && !std::isspace(static_cast<unsigned char>(restView[profileEnd]))) ++profileEnd;
    const std::string_view profile = restView.substr(p, profileEnd - p);
    PPMacro flag;
    flag.body = "1";
    if(profile == "core")
     macros["GL_core_profile"] = flag;
    else if(profile == "compatibility")
     macros["GL_compatibility_profile"] = flag;
   }
  }
 }
 }
} // namespace net::minecraft::client::render
