#include "net/minecraft/util/json/JsonFields.hpp"
#include <cctype>
#include <limits>
namespace net::minecraft::util::json {
namespace {
std::size_t whitespace(const std::string& text, std::size_t pos) {
  while(pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  return pos;
}
int hex(char ch) {
  if(ch >= '0' && ch <= '9')
    return ch - '0';
  if(ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  if(ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  return -1;
}
bool codeUnit(const std::string& text, std::size_t& pos, std::uint32_t& value) {
  if(pos + 4 > text.size())
    return false;
  value = 0;
  for(int i = 0; i < 4; ++i) {
    const int digit = hex(text[pos++]);
    if(digit < 0)
      return false;
    value = (value << 4U) | static_cast<std::uint32_t>(digit);
  }
  return true;
}
void utf8(std::string& out, std::uint32_t value) {
  if(value <= 0x7FU) {
    out.push_back(static_cast<char>(value));
  } else if(value <= 0x7FFU) {
    out.push_back(static_cast<char>(0xC0U | (value >> 6U)));
    out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
  } else if(value <= 0xFFFFU) {
    out.push_back(static_cast<char>(0xE0U | (value >> 12U)));
    out.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
  } else {
    out.push_back(static_cast<char>(0xF0U | (value >> 18U)));
    out.push_back(static_cast<char>(0x80U | ((value >> 12U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
  }
}
bool string(const std::string& text, std::size_t& pos, std::string* out) {
  pos = whitespace(text, pos);
  if(pos >= text.size() || text[pos++] != '"')
    return false;
  std::string discarded;
  if(!out)
    out = &discarded;
  out->clear();
  while(pos < text.size()) {
    const char ch = text[pos++];
    if(ch == '"')
      return true;
    if(ch != '\\') {
      out->push_back(ch);
      continue;
    }
    if(pos >= text.size())
      return false;
    const char escaped = text[pos++];
    switch(escaped) {
    case '"':
    case '\\':
    case '/':
      out->push_back(escaped);
      break;
    case 'b':
      out->push_back('\b');
      break;
    case 'f':
      out->push_back('\f');
      break;
    case 'n':
      out->push_back('\n');
      break;
    case 'r':
      out->push_back('\r');
      break;
    case 't':
      out->push_back('\t');
      break;
    case 'u': {
      std::uint32_t value = 0;
      if(!codeUnit(text, pos, value))
        return false;
      if(value >= 0xD800U && value <= 0xDBFFU) {
        if(pos + 2 > text.size() || text[pos] != '\\' || text[pos + 1] != 'u')
          return false;
        pos += 2;
        std::uint32_t low = 0;
        if(!codeUnit(text, pos, low) || low < 0xDC00U || low > 0xDFFFU)
          return false;
        value = 0x10000U + ((value - 0xD800U) << 10U) + low - 0xDC00U;
      } else if(value >= 0xDC00U && value <= 0xDFFFU) {
        return false;
      }
      utf8(*out, value);
      break;
    }
    default:
      return false;
    }
  }
  return false;
}
bool skipValue(const std::string& text, std::size_t& pos);
bool skipContainer(const std::string& text, std::size_t& pos, char open, char close) {
  if(text[pos++] != open)
    return false;
  pos = whitespace(text, pos);
  if(pos < text.size() && text[pos] == close) {
    ++pos;
    return true;
  }
  while(pos < text.size()) {
    if(open == '{') {
      if(!string(text, pos, nullptr))
        return false;
      pos = whitespace(text, pos);
      if(pos >= text.size() || text[pos++] != ':')
        return false;
    }
    if(!skipValue(text, pos))
      return false;
    pos = whitespace(text, pos);
    if(pos >= text.size())
      return false;
    if(text[pos] == close) {
      ++pos;
      return true;
    }
    if(text[pos++] != ',')
      return false;
  }
  return false;
}
bool skipValue(const std::string& text, std::size_t& pos) {
  pos = whitespace(text, pos);
  if(pos >= text.size())
    return false;
  if(text[pos] == '"')
    return string(text, pos, nullptr);
  if(text[pos] == '{')
    return skipContainer(text, pos, '{', '}');
  if(text[pos] == '[')
    return skipContainer(text, pos, '[', ']');
  const std::size_t start = pos;
  while(pos < text.size() && text[pos] != ',' && text[pos] != '}' && text[pos] != ']' &&
        !std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  return pos > start;
}
bool range(const std::string& text, const std::string& key, std::size_t& start, std::size_t& end) {
  std::size_t pos = whitespace(text, 0);
  if(pos >= text.size() || text[pos++] != '{')
    return false;
  while((pos = whitespace(text, pos)) < text.size() && text[pos] != '}') {
    std::string current;
    if(!string(text, pos, &current))
      return false;
    pos = whitespace(text, pos);
    if(pos >= text.size() || text[pos++] != ':')
      return false;
    start = whitespace(text, pos);
    end = start;
    if(!skipValue(text, end))
      return false;
    if(current == key)
      return true;
    pos = whitespace(text, end);
    if(pos >= text.size() || text[pos++] != ',')
      return false;
  }
  return false;
}
} // namespace
std::optional<std::string> stringField(const std::string& text, const std::string& key) {
  std::size_t start = 0, end = 0;
  if(!range(text, key, start, end))
    return std::nullopt;
  std::string value;
  return string(text, start, &value) && start == end ? std::optional<std::string>(std::move(value)) : std::nullopt;
}
std::optional<std::int64_t> int64Field(const std::string& text, const std::string& key) {
  std::size_t start = 0, end = 0;
  if(!range(text, key, start, end))
    return std::nullopt;
  try {
    std::size_t used = 0;
    const auto value = std::stoll(text.substr(start, end - start), &used);
    return used == end - start ? std::optional<std::int64_t>(value) : std::nullopt;
  } catch(...) {
    return std::nullopt;
  }
}
std::optional<int> intField(const std::string& text, const std::string& key) {
  const auto value = int64Field(text, key);
  if(!value || *value < std::numeric_limits<int>::min() || *value > std::numeric_limits<int>::max())
    return std::nullopt;
  return static_cast<int>(*value);
}
std::optional<bool> boolField(const std::string& text, const std::string& key) {
  std::size_t start = 0, end = 0;
  if(!range(text, key, start, end))
    return std::nullopt;
  const std::string value = text.substr(start, end - start);
  if(value == "true")
    return true;
  if(value == "false")
    return false;
  return std::nullopt;
}
std::optional<std::string> objectField(const std::string& text, const std::string& key) {
  std::size_t start = 0, end = 0;
  if(!range(text, key, start, end) || start >= text.size() || text[start] != '{')
    return std::nullopt;
  return text.substr(start, end - start);
}
std::vector<std::string> objectArrayField(const std::string& text, const std::string& key) {
  std::vector<std::string> objects;
  std::size_t start = 0, end = 0;
  if(!range(text, key, start, end) || start >= text.size() || text[start++] != '[')
    return objects;
  while((start = whitespace(text, start)) < end && text[start] != ']') {
    const std::size_t objectStart = start;
    if(text[start] != '{' || !skipValue(text, start))
      return {};
    objects.push_back(text.substr(objectStart, start - objectStart));
    start = whitespace(text, start);
    if(start < end && text[start] == ',')
      ++start;
  }
  return objects;
}
std::string escape(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  static constexpr char hexDigits[] = "0123456789ABCDEF";
  for(const unsigned char ch : text) {
    switch(ch) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if(ch < 0x20U) {
        out += "\\u00";
        out += hexDigits[ch >> 4U];
        out += hexDigits[ch & 0x0FU];
      } else {
        out += static_cast<char>(ch);
      }
    }
  }
  return out;
}
} // namespace net::minecraft::util::json
