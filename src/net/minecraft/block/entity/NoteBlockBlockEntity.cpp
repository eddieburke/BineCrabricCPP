#include "net/minecraft/block/entity/NoteBlockBlockEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block::entity {
void NoteBlockBlockEntity::writeNbt(NbtCompound& nbt) const {
  BlockEntity::writeNbt(nbt);
  nbt.putByte("note", note);
}
void NoteBlockBlockEntity::readNbt(const NbtCompound& nbt) {
  BlockEntity::readNbt(nbt);
  note = nbt.getByte("note");
  if(note < 0) {
    note = 0;
  }
  if(note > 24) {
    note = 24;
  }
}
void NoteBlockBlockEntity::cycleNote() {
  note = static_cast<std::int8_t>((note + 1) % 25);
  markDirty();
}
void NoteBlockBlockEntity::playNote(World* world, int noteX, int noteY, int noteZ) {
  if(world == nullptr) {
    return;
  }
  if(&world->getMaterial(noteX, noteY + 1, noteZ) != &block::material::Material::AIR) {
    return;
  }
  const block::material::Material& below = world->getMaterial(noteX, noteY - 1, noteZ);
  int instrument = 0;
  if(&below == &block::material::Material::STONE) {
    instrument = 1;
  } else if(&below == &block::material::Material::SAND) {
    instrument = 2;
  } else if(&below == &block::material::Material::GLASS) {
    instrument = 3;
  } else if(&below == &block::material::Material::WOOD) {
    instrument = 4;
  }
  world->playNoteBlockActionAt(noteX, noteY, noteZ, instrument, note);
}
} // namespace net::minecraft::block::entity
