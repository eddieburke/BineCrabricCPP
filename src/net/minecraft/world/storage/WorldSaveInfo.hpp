#pragma once
#include <cstdint>
#include <string>
namespace net::minecraft {
class WorldSaveInfo {
 public:
 WorldSaveInfo(std::string saveName, std::string name, std::int64_t lastPlayed, std::int64_t size, bool sameVersion)
     : saveName_(std::move(saveName)),
       name_(std::move(name)),
       lastPlayed_(lastPlayed),
       size_(size),
       sameVersion_(sameVersion) {
 }
 [[nodiscard]] int compareTo(const WorldSaveInfo& other) const noexcept {
  if(lastPlayed_ < other.lastPlayed_) {
   return 1;
  }
  if(lastPlayed_ > other.lastPlayed_) {
   return -1;
  }
  return saveName_.compare(other.saveName_);
 }
 [[nodiscard]] const std::string& getSaveName() const noexcept {
  return saveName_;
 }
 [[nodiscard]] const std::string& getName() const noexcept {
  return name_;
 }
 [[nodiscard]] std::int64_t getSize() const noexcept {
  return size_;
 }
 [[nodiscard]] bool isSameVersion() const noexcept {
  return sameVersion_;
 }
 [[nodiscard]] std::int64_t getLastPlayed() const noexcept {
  return lastPlayed_;
 }

 private:
 std::string saveName_;
 std::string name_;
 std::int64_t lastPlayed_ = 0;
 std::int64_t size_ = 0;
 bool sameVersion_ = false;
};
} // namespace net::minecraft
