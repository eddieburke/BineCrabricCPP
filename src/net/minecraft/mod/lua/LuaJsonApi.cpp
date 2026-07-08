#include "net/minecraft/mod/lua/LuaJsonApi.hpp"

#include <cctype>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>

#include "net/minecraft/mod/lua/LuaHostApi.hpp"

namespace net::minecraft::mod::lua {
namespace {
std::string escapeJsonString(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char ch : text) {
        switch (ch) {
            case '"':
                out.append("\\\"");
                break;
            case '\\':
                out.append("\\\\");
                break;
            case '\b':
                out.append("\\b");
                break;
            case '\f':
                out.append("\\f");
                break;
            case '\n':
                out.append("\\n");
                break;
            case '\r':
                out.append("\\r");
                break;
            case '\t':
                out.append("\\t");
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(ch));
                    out.append(buf);
                } else {
                    out.push_back(ch);
                }
                break;
        }
    }
    return out;
}

bool isArrayTable(lua_State* state, int index) {
    LuaApi& api = luaApi();
    if (api.type(state, index) != kLuaTTable) {
        return false;
    }
    const std::size_t length = api.rawlen(state, index);
    if (length == 0) {
        return false;
    }
    for (std::size_t i = 1; i <= length; ++i) {
        api.rawgeti(state, index, static_cast<long long>(i));
        if (api.type(state, -1) == kLuaTNil) {
            api.settop(state, -2);
            return false;
        }
        api.settop(state, -2);
    }
    api.pushnil(state);
    while (api.next(state, index < 0 ? index - 1 : index) != 0) {
        if (api.type(state, -2) == kLuaTNumber) {
            int isNumber = 0;
            const long long key = api.tointegerx(state, -2, &isNumber);
            if (isNumber == 0 || key < 1 || static_cast<std::size_t>(key) > length) {
                api.settop(state, -2);
                return false;
            }
        } else {
            api.settop(state, -2);
            return false;
        }
        api.settop(state, -1);
    }
    return true;
}

bool encodeValue(lua_State* state, int index, std::ostringstream& out);

bool encodeTable(lua_State* state, int index, std::ostringstream& out) {
    LuaApi& api = luaApi();
    if (isArrayTable(state, index)) {
        const std::size_t length = api.rawlen(state, index);
        out << '[';
        for (std::size_t i = 1; i <= length; ++i) {
            if (i > 1) {
                out << ',';
            }
            api.rawgeti(state, index, static_cast<long long>(i));
            if (!encodeValue(state, api.gettop(state), out)) {
                api.settop(state, -2);
                return false;
            }
            api.settop(state, -2);
        }
        out << ']';
        return true;
    }
    out << '{';
    bool first = true;
    api.pushnil(state);
    while (api.next(state, index < 0 ? index - 1 : index) != 0) {
        if (api.type(state, -2) != kLuaTString) {
            api.settop(state, -2);
            return false;
        }
        const char* key = api.tolstring(state, -2, nullptr);
        if (key == nullptr) {
            api.settop(state, -2);
            return false;
        }
        if (!first) {
            out << ',';
        }
        first = false;
        out << '"' << escapeJsonString(key) << "\":";
        if (!encodeValue(state, -1, out)) {
            api.settop(state, -2);
            return false;
        }
        api.settop(state, -1);
    }
    out << '}';
    return true;
}

bool encodeValue(lua_State* state, int index, std::ostringstream& out) {
    LuaApi& api = luaApi();
    const int type = api.type(state, index);
    if (type == kLuaTNil) {
        out << "null";
        return true;
    }
    if (type == kLuaTBoolean) {
        out << (api.toboolean(state, index) != 0 ? "true" : "false");
        return true;
    }
    if (type == kLuaTNumber) {
        int isNumber = 0;
        const double number = api.tonumberx(state, index, &isNumber);
        if (isNumber == 0) {
            return false;
        }
        if (std::isfinite(number) && number == std::floor(number) && number >= -9007199254740991.0 &&
            number <= 9007199254740991.0) {
            out << static_cast<long long>(number);
        } else {
            out << number;
        }
        return true;
    }
    if (type == kLuaTString) {
        const char* text = api.tolstring(state, index, nullptr);
        out << '"' << escapeJsonString(text != nullptr ? text : "") << '"';
        return true;
    }
    if (type == kLuaTTable) {
        return encodeTable(state, index, out);
    }
    return false;
}

class JsonDecoder {
   public:
    explicit JsonDecoder(std::string_view text) : text_(text) {
    }

    bool decode(lua_State* state) {
        skipWs();
        return decodeValue(state);
    }

    [[nodiscard]] std::string error() const {
        return error_;
    }

   private:
    std::string_view text_;
    std::size_t pos_ = 0;
    std::string error_;

    char peek() const {
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }

    bool consume(char expected) {
        skipWs();
        if (peek() != expected) {
            error_ = "unexpected JSON token";
            return false;
        }
        ++pos_;
        return true;
    }

    void skipWs() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool decodeValue(lua_State* state) {
        skipWs();
        const char ch = peek();
        if (ch == '"') {
            return decodeString(state);
        }
        if (ch == '{') {
            return decodeObject(state);
        }
        if (ch == '[') {
            return decodeArray(state);
        }
        if (ch == 't') {
            return decodeLiteral(state, "true", 1);
        }
        if (ch == 'f') {
            return decodeLiteral(state, "false", 0);
        }
        if (ch == 'n') {
            return decodeNull(state);
        }
        return decodeNumber(state);
    }

    bool decodeLiteral(lua_State* state, const char* literal, int booleanValue) {
        const std::size_t len = std::strlen(literal);
        if (text_.substr(pos_, len) != literal) {
            error_ = "invalid JSON literal";
            return false;
        }
        pos_ += len;
        luaApi().pushboolean(state, booleanValue);
        return true;
    }

    bool decodeNull(lua_State* state) {
        if (text_.substr(pos_, 4) != "null") {
            error_ = "invalid JSON null";
            return false;
        }
        pos_ += 4;
        luaApi().pushnil(state);
        return true;
    }

    bool decodeString(lua_State* state) {
        if (!consume('"')) {
            return false;
        }
        std::string out;
        while (pos_ < text_.size()) {
            const char ch = text_[pos_++];
            if (ch == '"') {
                luaApi().pushstring(state, out.c_str());
                return true;
            }
            if (ch == '\\') {
                if (pos_ >= text_.size()) {
                    error_ = "unterminated JSON string escape";
                    return false;
                }
                const char esc = text_[pos_++];
                switch (esc) {
                    case '"':
                        out.push_back('"');
                        break;
                    case '\\':
                        out.push_back('\\');
                        break;
                    case '/':
                        out.push_back('/');
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
                    default:
                        error_ = "invalid JSON string escape";
                        return false;
                }
                continue;
            }
            out.push_back(ch);
        }
        error_ = "unterminated JSON string";
        return false;
    }

    bool decodeNumber(lua_State* state) {
        skipWs();
        const std::size_t start = pos_;
        if (peek() == '-') {
            ++pos_;
        }
        if (peek() == '0') {
            ++pos_;
        } else if (std::isdigit(static_cast<unsigned char>(peek()))) {
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        } else {
            error_ = "invalid JSON number";
            return false;
        }
        if (peek() == '.') {
            ++pos_;
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                error_ = "invalid JSON number";
                return false;
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            ++pos_;
            if (peek() == '+' || peek() == '-') {
                ++pos_;
            }
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                error_ = "invalid JSON number";
                return false;
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        }
        const std::string token(text_.substr(start, pos_ - start));
        try {
            if (token.find_first_of(".eE") != std::string::npos) {
                luaApi().pushnumber(state, std::stod(token));
            } else {
                luaApi().pushinteger(state, std::stoll(token));
            }
            return true;
        } catch (...) {
            error_ = "invalid JSON number";
            return false;
        }
    }

    bool decodeArray(lua_State* state) {
        if (!consume('[')) {
            return false;
        }
        LuaApi& api = luaApi();
        api.createtable(state, 0, 0);
        const int tableIndex = api.gettop(state);
        skipWs();
        if (peek() == ']') {
            ++pos_;
            return true;
        }
        int index = 1;
        for (;;) {
            if (!decodeValue(state)) {
                return false;
            }
            api.rawseti(state, tableIndex, static_cast<long long>(index++));
            skipWs();
            if (peek() == ']') {
                ++pos_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }

    bool decodeObject(lua_State* state) {
        if (!consume('{')) {
            return false;
        }
        LuaApi& api = luaApi();
        api.createtable(state, 0, 0);
        const int tableIndex = api.gettop(state);
        skipWs();
        if (peek() == '}') {
            ++pos_;
            return true;
        }
        for (;;) {
            if (!decodeString(state)) {
                return false;
            }
            const std::string key = luaString(state, -1, "");
            api.settop(state, -2);
            if (!consume(':')) {
                return false;
            }
            if (!decodeValue(state)) {
                return false;
            }
            api.pushstring(state, key.c_str());
            api.settable(state, tableIndex);
            skipWs();
            if (peek() == '}') {
                ++pos_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }
};
}  // namespace

int luaJsonEncode(lua_State* state) {
    LuaApi& api = luaApi();
    if (api.type(state, 1) != kLuaTTable) {
        api.pushnil(state);
        api.pushstring(state, "json_encode expects a table");
        return 2;
    }
    std::ostringstream out;
    if (!encodeValue(state, 1, out)) {
        api.pushnil(state);
        api.pushstring(state, "value is not JSON-serializable");
        return 2;
    }
    api.pushstring(state, out.str().c_str());
    return 1;
}

int luaJsonDecode(lua_State* state) {
    LuaApi& api = luaApi();
    const std::string json = luaString(state, 1, "");
    if (json.empty()) {
        api.pushnil(state);
        api.pushstring(state, "empty JSON input");
        return 2;
    }
    JsonDecoder decoder(json);
    if (!decoder.decode(state)) {
        api.pushnil(state);
        api.pushstring(state, decoder.error().c_str());
        return 2;
    }
    return 1;
}
}  // namespace net::minecraft::mod::lua
