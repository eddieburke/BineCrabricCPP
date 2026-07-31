#include "net/minecraft/util/json/JsonFields.hpp"
#include <limits>
#include "net/minecraft/util/json/JsonValue.hpp"
namespace net::minecraft::util::json {
namespace {
[[nodiscard]] bool parseObject(const std::string& text, JsonValue& root) {
 std::string error;
 return JsonValue::parse(text, root, error) && root.isObject();
}
[[nodiscard]] const JsonValue* member(const JsonValue& object, const std::string& key) {
 if(!object.isObject()) {
  return nullptr;
 }
 for(const auto& [name, value] : object.members()) {
  if(name == key) {
   return &value;
  }
 }
 return nullptr;
}
} // namespace
std::optional<std::string> stringField(const std::string& text, const std::string& key) {
 JsonValue root;
 if(!parseObject(text, root)) {
  return std::nullopt;
 }
 const JsonValue* value = member(root, key);
 if(value == nullptr || !value->isString()) {
  return std::nullopt;
 }
 return value->asString();
}
std::optional<std::int64_t> int64Field(const std::string& text, const std::string& key) {
 JsonValue root;
 if(!parseObject(text, root)) {
  return std::nullopt;
 }
 const JsonValue* value = member(root, key);
 if(value == nullptr || !value->isNumber()) {
  return std::nullopt;
 }
 const double number = value->asNumber();
 if(number != static_cast<double>(static_cast<std::int64_t>(number))) {
  return std::nullopt;
 }
 return static_cast<std::int64_t>(number);
}
std::optional<int> intField(const std::string& text, const std::string& key) {
 const auto value = int64Field(text, key);
 if(!value || *value < std::numeric_limits<int>::min() || *value > std::numeric_limits<int>::max()) {
  return std::nullopt;
 }
 return static_cast<int>(*value);
}
std::optional<bool> boolField(const std::string& text, const std::string& key) {
 JsonValue root;
 if(!parseObject(text, root)) {
  return std::nullopt;
 }
 const JsonValue* value = member(root, key);
 if(value == nullptr || value->type() != JsonValue::Type::Boolean) {
  return std::nullopt;
 }
 return value->asBool();
}
std::optional<std::string> objectField(const std::string& text, const std::string& key) {
 JsonValue root;
 if(!parseObject(text, root)) {
  return std::nullopt;
 }
 const JsonValue* value = member(root, key);
 if(value == nullptr || !value->isObject()) {
  return std::nullopt;
 }
 return value->dump();
}
std::vector<std::string> objectArrayField(const std::string& text, const std::string& key) {
 std::vector<std::string> objects;
 JsonValue root;
 if(!parseObject(text, root)) {
  return objects;
 }
 const JsonValue* value = member(root, key);
 if(value == nullptr || !value->isArray()) {
  return objects;
 }
 for(std::size_t i = 0; i < value->size(); ++i) {
  const JsonValue& element = value->at(i);
  if(!element.isObject()) {
   return {};
  }
  objects.push_back(element.dump());
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
