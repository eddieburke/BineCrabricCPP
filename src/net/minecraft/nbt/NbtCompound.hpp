#pragma once
#include <string>
#include <utility>
#include <vector>
#include "net/minecraft/nbt/Nbt.hpp"
namespace net::minecraft {
class NbtList;
// Compound tag handle. Freestanding compounds own storage; nested handles alias map slots
// (same reference semantics as Java HashMap<NbtCompound>).
class NbtCompound {
 public:
 NbtCompound() : owned_(Nbt::compound()), ptr_(&owned_) {
 }
 explicit NbtCompound(Nbt tag) : owned_(tag.isCompound() ? std::move(tag) : Nbt::compound()), ptr_(&owned_) {
 }
 NbtCompound(const NbtCompound& other) : owned_(other.storage()), ptr_(&owned_) {
 }
 NbtCompound(NbtCompound&& other) noexcept : owned_(), ptr_(&owned_) {
  if(other.ptr_ == &other.owned_) {
   owned_ = std::move(other.owned_);
   other.owned_ = Nbt::compound();
  } else {
   owned_ = std::move(*other.ptr_);
  }
  other.ptr_ = &other.owned_;
 }
 NbtCompound& operator=(const NbtCompound& other) {
  if(this != &other) {
   *ptr_ = other.storage();
  }
  return *this;
 }
 NbtCompound& operator=(NbtCompound&& other) noexcept {
  if(this != &other) {
   if(other.ptr_ == &other.owned_) {
    *ptr_ = std::move(other.owned_);
    other.owned_ = Nbt::compound();
   } else {
    *ptr_ = std::move(*other.ptr_);
   }
   other.ptr_ = &other.owned_;
  }
  return *this;
 }
 [[nodiscard]] static NbtCompound bind(Nbt& node) {
  NbtCompound wrapper;
  wrapper.ptr_ = &node;
  if(!wrapper.ptr_->isCompound()) {
   *wrapper.ptr_ = Nbt::compound();
  }
  return wrapper;
 }
 [[nodiscard]] Nbt& storage() noexcept {
  return *ptr_;
 }
 [[nodiscard]] const Nbt& storage() const noexcept {
  return *ptr_;
 }
 void put(const std::string& key, Nbt value) {
  ptr_->put(key, std::move(value));
 }
 void put(const std::string& key, NbtCompound& child);
 void put(const std::string& key, const NbtCompound& child) {
  ptr_->put(key, child.storage());
 }
 void put(const std::string& key, NbtList& list);
 void put(const std::string& key, const NbtList& list);
 void putByte(const std::string& key, std::int8_t value) {
  ptr_->putByte(key, value);
 }
 void putShort(const std::string& key, std::int16_t value) {
  ptr_->putShort(key, value);
 }
 void putInt(const std::string& key, std::int32_t value) {
  ptr_->putInt(key, value);
 }
 void putLong(const std::string& key, std::int64_t value) {
  ptr_->putLong(key, value);
 }
 void putFloat(const std::string& key, float value) {
  ptr_->putFloat(key, value);
 }
 void putDouble(const std::string& key, double value) {
  ptr_->putDouble(key, value);
 }
 void putString(const std::string& key, std::string value) {
  ptr_->putString(key, std::move(value));
 }
 void putByteArray(const std::string& key, std::vector<std::uint8_t> value) {
  ptr_->putByteArray(key, std::move(value));
 }
 void putBoolean(const std::string& key, bool value) {
  ptr_->putBoolean(key, value);
 }
 [[nodiscard]] bool contains(const std::string& key) const {
  return ptr_->contains(key);
 }
 [[nodiscard]] std::int8_t getByte(const std::string& key) const {
  return ptr_->getByte(key);
 }
 [[nodiscard]] std::int16_t getShort(const std::string& key) const {
  return ptr_->getShort(key);
 }
 [[nodiscard]] std::int32_t getInt(const std::string& key) const {
  return ptr_->getInt(key);
 }
 [[nodiscard]] std::int64_t getLong(const std::string& key) const {
  return ptr_->getLong(key);
 }
 [[nodiscard]] float getFloat(const std::string& key) const {
  return ptr_->getFloat(key);
 }
 [[nodiscard]] double getDouble(const std::string& key) const {
  return ptr_->getDouble(key);
 }
 [[nodiscard]] std::string getString(const std::string& key) const {
  return ptr_->getString(key);
 }
 [[nodiscard]] std::vector<std::uint8_t> getByteArray(const std::string& key) const {
  return ptr_->getByteArray(key);
 }
 [[nodiscard]] bool getBoolean(const std::string& key) const {
  return ptr_->getBoolean(key);
 }
 [[nodiscard]] NbtCompound getCompound(const std::string& key);
 [[nodiscard]] NbtCompound getCompound(const std::string& key) const;
 [[nodiscard]] NbtList getList(const std::string& key);
 [[nodiscard]] NbtList getList(const std::string& key) const;
 void adoptInto(Nbt& slot);

 private:
 Nbt owned_{};
 Nbt* ptr_ = nullptr;
};
} // namespace net::minecraft
