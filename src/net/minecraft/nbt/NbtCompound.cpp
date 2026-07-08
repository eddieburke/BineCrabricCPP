#include "net/minecraft/nbt/NbtCompound.hpp"

#include "net/minecraft/nbt/NbtList.hpp"

namespace net::minecraft {
void NbtCompound::adoptInto(Nbt& slot) {
    if (ptr_ == &owned_) {
        slot = std::move(owned_);
        ptr_ = &slot;
        return;
    }
    if (ptr_ != &slot) {
        slot = *ptr_;
        ptr_ = &slot;
    }
}

void NbtList::adoptInto(Nbt& slot) {
    if (ptr_ == &owned_) {
        slot = std::move(owned_);
        ptr_ = &slot;
        return;
    }
    if (ptr_ != &slot) {
        slot = *ptr_;
        ptr_ = &slot;
    }
}

void NbtCompound::put(const std::string& key, NbtCompound& child) {
    Nbt& slot = ptr_->asCompound()[key];
    if (!slot.isCompound()) {
        slot = Nbt::compound();
    }
    child.adoptInto(slot);
}

void NbtCompound::put(const std::string& key, const NbtList& list) {
    ptr_->put(key, list.storage());
}

void NbtCompound::put(const std::string& key, NbtList& list) {
    Nbt& slot = ptr_->asCompound()[key];
    if (!slot.isList()) {
        slot = Nbt::list();
    }
    list.adoptInto(slot);
}

NbtCompound NbtCompound::getCompound(const std::string& key) {
    Nbt* child = ptr_->get(key);
    if (child != nullptr && child->isCompound()) {
        return bind(*child);
    }
    return NbtCompound();
}

NbtCompound NbtCompound::getCompound(const std::string& key) const {
    if (const Nbt* child = ptr_->get(key); child != nullptr && child->isCompound()) {
        return NbtCompound(*child);
    }
    return NbtCompound();
}

NbtList NbtCompound::getList(const std::string& key) {
    Nbt* child = ptr_->get(key);
    if (child != nullptr && child->isList()) {
        return NbtList::bind(*child);
    }
    return NbtList();
}

NbtList NbtCompound::getList(const std::string& key) const {
    if (const Nbt* child = ptr_->get(key); child != nullptr && child->isList()) {
        return NbtList(*child);
    }
    return NbtList();
}
}  // namespace net::minecraft
