#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace net::minecraft {
class Nbt {
   public:
    enum class Type : std::uint8_t {
        End = 0,
        Byte = 1,
        Short = 2,
        Int = 3,
        Long = 4,
        Float = 5,
        Double = 6,
        ByteArray = 7,
        String = 8,
        List = 9,
        Compound = 10,
        IntArray = 11,
        LongArray = 12
    };
    using List = std::vector<Nbt>;
    using Compound = std::unordered_map<std::string, Nbt>;
    using ByteArray = std::vector<std::uint8_t>;
    using IntArray = std::vector<std::int32_t>;
    using LongArray = std::vector<std::int64_t>;
    Nbt() = default;

    explicit Nbt(std::int8_t value) : type_(Type::Byte), value_(value) {
    }

    explicit Nbt(std::int16_t value) : type_(Type::Short), value_(value) {
    }

    explicit Nbt(std::int32_t value) : type_(Type::Int), value_(value) {
    }

    explicit Nbt(std::int64_t value) : type_(Type::Long), value_(value) {
    }

    explicit Nbt(float value) : type_(Type::Float), value_(value) {
    }

    explicit Nbt(double value) : type_(Type::Double), value_(value) {
    }

    explicit Nbt(std::string value) : type_(Type::String), value_(std::move(value)) {
    }

    explicit Nbt(const char* value) : Nbt(std::string(value)) {
    }

    explicit Nbt(std::vector<std::uint8_t> value) : type_(Type::ByteArray), value_(std::move(value)) {
    }

    explicit Nbt(std::vector<std::int32_t> value) : type_(Type::IntArray), value_(std::move(value)) {
    }

    explicit Nbt(std::vector<std::int64_t> value) : type_(Type::LongArray), value_(std::move(value)) {
    }

    static Nbt list() {
        Nbt tag;
        tag.type_ = Type::List;
        tag.value_ = List{};
        return tag;
    }

    static Nbt compound() {
        Nbt tag;
        tag.type_ = Type::Compound;
        tag.value_ = Compound{};
        return tag;
    }

    [[nodiscard]] Type type() const noexcept {
        return type_;
    }

    [[nodiscard]] bool isCompound() const noexcept {
        return type_ == Type::Compound;
    }

    [[nodiscard]] bool isList() const noexcept {
        return type_ == Type::List;
    }

    [[nodiscard]] Compound& asCompound() {
        ensureType(Type::Compound);
        return std::get<Compound>(value_);
    }

    [[nodiscard]] const Compound& asCompound() const {
        ensureType(Type::Compound);
        return std::get<Compound>(value_);
    }

    [[nodiscard]] std::int8_t asByte() const {
        ensureType(Type::Byte);
        return std::get<std::int8_t>(value_);
    }

    [[nodiscard]] std::int16_t asShort() const {
        ensureType(Type::Short);
        return std::get<std::int16_t>(value_);
    }

    [[nodiscard]] std::int32_t asInt() const {
        ensureType(Type::Int);
        return std::get<std::int32_t>(value_);
    }

    [[nodiscard]] std::int64_t asLong() const {
        ensureType(Type::Long);
        return std::get<std::int64_t>(value_);
    }

    [[nodiscard]] float asFloat() const {
        ensureType(Type::Float);
        return std::get<float>(value_);
    }

    [[nodiscard]] double asDouble() const {
        ensureType(Type::Double);
        return std::get<double>(value_);
    }

    [[nodiscard]] const std::string& asString() const {
        ensureType(Type::String);
        return std::get<std::string>(value_);
    }

    [[nodiscard]] ByteArray& asByteArray() {
        ensureType(Type::ByteArray);
        return std::get<ByteArray>(value_);
    }

    [[nodiscard]] const ByteArray& asByteArray() const {
        ensureType(Type::ByteArray);
        return std::get<ByteArray>(value_);
    }

    [[nodiscard]] IntArray& asIntArray() {
        ensureType(Type::IntArray);
        return std::get<IntArray>(value_);
    }

    [[nodiscard]] const IntArray& asIntArray() const {
        ensureType(Type::IntArray);
        return std::get<IntArray>(value_);
    }

    [[nodiscard]] LongArray& asLongArray() {
        ensureType(Type::LongArray);
        return std::get<LongArray>(value_);
    }

    [[nodiscard]] const LongArray& asLongArray() const {
        ensureType(Type::LongArray);
        return std::get<LongArray>(value_);
    }

    [[nodiscard]] List& asList() {
        ensureType(Type::List);
        return std::get<List>(value_);
    }

    [[nodiscard]] const List& asList() const {
        ensureType(Type::List);
        return std::get<List>(value_);
    }

    void put(const std::string& key, Nbt value) {
        asCompound()[key] = std::move(value);
    }

    void putByte(const std::string& key, std::int8_t value) {
        put(key, Nbt(value));
    }

    void putShort(const std::string& key, std::int16_t value) {
        put(key, Nbt(value));
    }

    void putInt(const std::string& key, std::int32_t value) {
        put(key, Nbt(value));
    }

    void putLong(const std::string& key, std::int64_t value) {
        put(key, Nbt(value));
    }

    void putFloat(const std::string& key, float value) {
        put(key, Nbt(value));
    }

    void putDouble(const std::string& key, double value) {
        put(key, Nbt(value));
    }

    void putString(const std::string& key, std::string value) {
        put(key, Nbt(std::move(value)));
    }

    void putByteArray(const std::string& key, ByteArray value) {
        put(key, Nbt(std::move(value)));
    }

    void putIntArray(const std::string& key, IntArray value) {
        put(key, Nbt(std::move(value)));
    }

    void putLongArray(const std::string& key, LongArray value) {
        put(key, Nbt(std::move(value)));
    }

    void putBoolean(const std::string& key, bool value) {
        putByte(key, value ? 1 : 0);
    }

    [[nodiscard]] const Nbt* get(const std::string& key) const {
        if (!isCompound()) {
            return nullptr;
        }
        const auto& compound = asCompound();
        const auto it = compound.find(key);
        if (it == compound.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] Nbt* get(const std::string& key) {
        if (!isCompound()) {
            return nullptr;
        }
        auto& compound = asCompound();
        const auto it = compound.find(key);
        if (it == compound.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] bool contains(const std::string& key) const {
        return isCompound() && asCompound().find(key) != asCompound().end();
    }

    [[nodiscard]] std::int8_t getByte(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Byte) {
            return std::get<std::int8_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] std::int16_t getShort(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Short) {
            return std::get<std::int16_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] std::int32_t getInt(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Int) {
            return std::get<std::int32_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] std::int64_t getLong(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Long) {
            return std::get<std::int64_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] float getFloat(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Float) {
            return std::get<float>(tag->value_);
        }
        return 0.0F;
    }

    [[nodiscard]] double getDouble(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Double) {
            return std::get<double>(tag->value_);
        }
        return 0.0;
    }

    [[nodiscard]] std::string getString(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::String) {
            return std::get<std::string>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] ByteArray getByteArray(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::ByteArray) {
            return std::get<ByteArray>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] IntArray getIntArray(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::IntArray) {
            return std::get<IntArray>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] LongArray getLongArray(const std::string& key) const {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::LongArray) {
            return std::get<LongArray>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] bool getBoolean(const std::string& key) const {
        return getByte(key) != 0;
    }

    void write(std::ostream& out) const;
    void writeCompressed(std::ostream& out) const;
    [[nodiscard]] std::vector<std::uint8_t> toBytes() const;
    static Nbt read(std::istream& in);
    static Nbt read(const std::vector<std::uint8_t>& bytes);
    static Nbt readCompressed(std::istream& in);
    static Nbt readCompressed(const std::vector<std::uint8_t>& bytes);

   private:
    using Storage = std::variant<std::monostate,
                                 std::int8_t,
                                 std::int16_t,
                                 std::int32_t,
                                 std::int64_t,
                                 float,
                                 double,
                                 std::vector<std::uint8_t>,
                                 std::string,
                                 List,
                                 Compound,
                                 std::vector<std::int32_t>,
                                 std::vector<std::int64_t>>;
    void ensureType(Type expected) const;
    Type type_ = Type::End;
    Storage value_{};
};
}  // namespace net::minecraft
