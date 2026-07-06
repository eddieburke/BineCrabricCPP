#pragma once
#include "net/minecraft/nbt/Nbt.hpp"
namespace net::minecraft {
class NbtList {
public:
  NbtList() : owned_(Nbt::list()), ptr_(&owned_) {}
  explicit NbtList(Nbt tag) : owned_(tag.isList() ? std::move(tag) : Nbt::list()), ptr_(&owned_) {}
  NbtList(const NbtList& other) : owned_(other.storage()), ptr_(&owned_) {}
  NbtList(NbtList&& other) noexcept : owned_(), ptr_(&owned_) {
    if(other.ptr_ == &other.owned_) {
      owned_ = std::move(other.owned_);
      other.owned_ = Nbt::list();
    } else {
      owned_ = std::move(*other.ptr_);
    }
    other.ptr_ = &other.owned_;
  }
  NbtList& operator=(const NbtList& other) {
    if(this != &other) {
      *ptr_ = other.storage();
    }
    return *this;
  }
  NbtList& operator=(NbtList&& other) noexcept {
    if(this != &other) {
      if(other.ptr_ == &other.owned_) {
        *ptr_ = std::move(other.owned_);
        other.owned_ = Nbt::list();
      } else {
        *ptr_ = std::move(*other.ptr_);
      }
      other.ptr_ = &other.owned_;
    }
    return *this;
  }
  [[nodiscard]] static NbtList bind(Nbt& node) {
    NbtList wrapper;
    wrapper.ptr_ = &node;
    if(!wrapper.ptr_->isList()) {
      *wrapper.ptr_ = Nbt::list();
    }
    return wrapper;
  }
  [[nodiscard]] Nbt& storage() noexcept {
    return *ptr_;
  }
  [[nodiscard]] const Nbt& storage() const noexcept {
    return *ptr_;
  }
  void add(Nbt value) {
    ptr_->asList().push_back(std::move(value));
  }
  [[nodiscard]] std::size_t size() const {
    return ptr_->asList().size();
  }
  [[nodiscard]] const Nbt::List& entries() const {
    return ptr_->asList();
  }
  void adoptInto(Nbt& slot);

private:
  Nbt owned_{};
  Nbt* ptr_ = nullptr;
};
} // namespace net::minecraft
