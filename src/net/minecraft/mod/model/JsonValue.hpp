#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace net::minecraft::mod::model {
class JsonValue {
public:
  enum class Type {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
  };
  static bool parse(std::string_view text, JsonValue& out, std::string& error);
  [[nodiscard]] Type type() const noexcept {
    return type_;
  }
  [[nodiscard]] bool isNull() const noexcept {
    return type_ == Type::Null;
  }
  [[nodiscard]] bool isNumber() const noexcept {
    return type_ == Type::Number;
  }
  [[nodiscard]] bool isString() const noexcept {
    return type_ == Type::String;
  }
  [[nodiscard]] bool isArray() const noexcept {
    return type_ == Type::Array;
  }
  [[nodiscard]] bool isObject() const noexcept {
    return type_ == Type::Object;
  }
  [[nodiscard]] bool asBool(bool fallback = false) const noexcept {
    return type_ == Type::Boolean ? boolean_ : fallback;
  }
  [[nodiscard]] double asNumber(double fallback = 0.0) const noexcept {
    return type_ == Type::Number ? number_ : fallback;
  }
  [[nodiscard]] const std::string& asString() const noexcept {
    return string_;
  }
  [[nodiscard]] std::size_t size() const noexcept {
    return type_ == Type::Array ? array_.size() : members_.size();
  }
  [[nodiscard]] const JsonValue& at(std::size_t index) const noexcept;
  [[nodiscard]] const JsonValue& operator[](std::string_view key) const noexcept;
  [[nodiscard]] const std::vector<std::pair<std::string, JsonValue>>& members() const noexcept {
    return members_;
  }

private:
  friend class JsonParser;
  Type type_ = Type::Null;
  bool boolean_ = false;
  double number_ = 0.0;
  std::string string_;
  std::vector<JsonValue> array_;
  std::vector<std::pair<std::string, JsonValue>> members_;
};
} // namespace net::minecraft::mod::model
