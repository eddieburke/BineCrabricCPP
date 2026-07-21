#include "net/minecraft/mod/model/JsonValue.hpp"
#include <cctype>
#include <cstdlib>
namespace net::minecraft::mod::model {
namespace {
const JsonValue kNullValue;
}
const JsonValue& JsonValue::at(std::size_t index) const noexcept {
 if(type_ == Type::Array && index < array_.size()) {
  return array_[index];
 }
 return kNullValue;
}
const JsonValue& JsonValue::operator[](std::string_view key) const noexcept {
 if(type_ == Type::Object) {
  for(const auto& [name, value] : members_) {
   if(name == key) {
    return value;
   }
  }
 }
 return kNullValue;
}
class JsonParser {
 public:
 explicit JsonParser(std::string_view text) : text_(text) {
 }
 bool parse(JsonValue& out) {
  skipWs();
  if(!parseValue(out)) {
   return false;
  }
  skipWs();
  if(pos_ != text_.size()) {
   error_ = "trailing characters after JSON value";
   return false;
  }
  return true;
 }
 [[nodiscard]] const std::string& error() const {
  return error_;
 }

 private:
 std::string_view text_;
 std::size_t pos_ = 0;
 std::string error_;
 [[nodiscard]] char peek() const {
  return pos_ < text_.size() ? text_[pos_] : '\0';
 }
 void skipWs() {
  while(pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
   ++pos_;
  }
 }
 bool consume(char expected) {
  skipWs();
  if(peek() != expected) {
   error_ = std::string("expected '") + expected + "'";
   return false;
  }
  ++pos_;
  return true;
 }
 bool parseValue(JsonValue& out) {
  skipWs();
  switch(peek()) {
  case '"':
   out.type_ = JsonValue::Type::String;
   return parseString(out.string_);
  case '{':
   return parseObject(out);
  case '[':
   return parseArray(out);
  case 't':
  case 'f':
  case 'n':
   return parseLiteral(out);
  default:
   return parseNumber(out);
  }
 }
 bool parseLiteral(JsonValue& out) {
  if(text_.substr(pos_, 4) == "true") {
   pos_ += 4;
   out.type_ = JsonValue::Type::Boolean;
   out.boolean_ = true;
   return true;
  }
  if(text_.substr(pos_, 5) == "false") {
   pos_ += 5;
   out.type_ = JsonValue::Type::Boolean;
   out.boolean_ = false;
   return true;
  }
  if(text_.substr(pos_, 4) == "null") {
   pos_ += 4;
   out.type_ = JsonValue::Type::Null;
   return true;
  }
  error_ = "invalid JSON literal";
  return false;
 }
 bool parseNumber(JsonValue& out) {
  const std::size_t start = pos_;
  if(peek() == '-') {
   ++pos_;
  }
  while(std::isdigit(static_cast<unsigned char>(peek()))) {
   ++pos_;
  }
  if(peek() == '.') {
   ++pos_;
   while(std::isdigit(static_cast<unsigned char>(peek()))) {
    ++pos_;
   }
  }
  if(peek() == 'e' || peek() == 'E') {
   ++pos_;
   if(peek() == '+' || peek() == '-') {
    ++pos_;
   }
   while(std::isdigit(static_cast<unsigned char>(peek()))) {
    ++pos_;
   }
  }
  if(pos_ == start || (pos_ == start + 1 && text_[start] == '-')) {
   error_ = "invalid JSON number";
   return false;
  }
  const std::string token(text_.substr(start, pos_ - start));
  char* end = nullptr;
  const double number = std::strtod(token.c_str(), &end);
  if(end == nullptr || *end != '\0') {
   error_ = "invalid JSON number";
   return false;
  }
  out.type_ = JsonValue::Type::Number;
  out.number_ = number;
  return true;
 }
 bool parseString(std::string& out) {
  if(!consume('"')) {
   return false;
  }
  out.clear();
  while(pos_ < text_.size()) {
   const char ch = text_[pos_++];
   if(ch == '"') {
    return true;
   }
   if(ch != '\\') {
    out.push_back(ch);
    continue;
   }
   if(pos_ >= text_.size()) {
    break;
   }
   const char esc = text_[pos_++];
   switch(esc) {
   case '"':
   case '\\':
   case '/':
    out.push_back(esc);
    break;
   case 'b':
    out.push_back('\b');
    break;
   case 'f':
    out.push_back('\f');
    break;
   case 'n':
    out.push_back('\n');
    break;
   case 'r':
    out.push_back('\r');
    break;
   case 't':
    out.push_back('\t');
    break;
   case 'u': {
    unsigned codepoint = 0;
    if(!parseHex4(codepoint)) {
     return false;
    }
    if(codepoint >= 0xD800 && codepoint <= 0xDBFF && text_.substr(pos_, 2) == "\\u") {
     pos_ += 2;
     unsigned low = 0;
     if(!parseHex4(low)) {
      return false;
     }
     codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
    }
    appendUtf8(out, codepoint);
    break;
   }
   default:
    error_ = "invalid JSON string escape";
    return false;
   }
  }
  error_ = "unterminated JSON string";
  return false;
 }
 bool parseHex4(unsigned& value) {
  if(pos_ + 4 > text_.size()) {
   error_ = "incomplete unicode escape";
   return false;
  }
  value = 0;
  for(int i = 0; i < 4; ++i) {
   const char ch = text_[pos_++];
   value <<= 4;
   if(ch >= '0' && ch <= '9') {
    value += static_cast<unsigned>(ch - '0');
   } else if(ch >= 'a' && ch <= 'f') {
    value += static_cast<unsigned>(ch - 'a' + 10);
   } else if(ch >= 'A' && ch <= 'F') {
    value += static_cast<unsigned>(ch - 'A' + 10);
   } else {
    error_ = "invalid unicode escape";
    return false;
   }
  }
  return true;
 }
 static void appendUtf8(std::string& out, unsigned codepoint) {
  if(codepoint <= 0x7F) {
   out.push_back(static_cast<char>(codepoint));
  } else if(codepoint <= 0x7FF) {
   out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
   out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if(codepoint <= 0xFFFF) {
   out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
   out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
   out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
   out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
   out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
   out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
   out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
 }
 bool parseArray(JsonValue& out) {
  if(!consume('[')) {
   return false;
  }
  out.type_ = JsonValue::Type::Array;
  skipWs();
  if(peek() == ']') {
   ++pos_;
   return true;
  }
  for(;;) {
   JsonValue& element = out.array_.emplace_back();
   if(!parseValue(element)) {
    return false;
   }
   skipWs();
   if(peek() == ']') {
    ++pos_;
    return true;
   }
   if(!consume(',')) {
    return false;
   }
  }
 }
 bool parseObject(JsonValue& out) {
  if(!consume('{')) {
   return false;
  }
  out.type_ = JsonValue::Type::Object;
  skipWs();
  if(peek() == '}') {
   ++pos_;
   return true;
  }
  for(;;) {
   skipWs();
   std::string key;
   if(!parseString(key)) {
    return false;
   }
   if(!consume(':')) {
    return false;
   }
   JsonValue& value = out.members_.emplace_back(std::move(key), JsonValue()).second;
   if(!parseValue(value)) {
    return false;
   }
   skipWs();
   if(peek() == '}') {
    ++pos_;
    return true;
   }
   if(!consume(',')) {
    return false;
   }
  }
 }
};
bool JsonValue::parse(std::string_view text, JsonValue& out, std::string& error) {
 out = JsonValue();
 JsonParser parser(text);
 if(parser.parse(out)) {
  return true;
 }
 error = parser.error();
 return false;
}
} // namespace net::minecraft::mod::model
