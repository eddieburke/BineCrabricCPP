#include "net/minecraft/client/render/shaders/PreProcessor.hpp"
#include <cctype>
#include <charconv>
#include <cstring>
#include <system_error>
#include <utility>
namespace net::minecraft::client::render {
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
bool evalExpressionText(std::string_view text, double& out) {
 PPExpressionEval eval(text);
 return eval.eval(out);
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
template bool evaluateIfExpression(std::string_view, const PPMacroTable&);
template void parseDefineDirective(std::string_view, PPMacroTable&);
} // namespace net::minecraft::client::render
