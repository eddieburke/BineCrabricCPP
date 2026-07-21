#include "net/minecraft/block/ChestBlock.hpp"
#include "net/minecraft/block/DispenserBlock.hpp"
#include "net/minecraft/block/JukeboxBlock.hpp"
#include "net/minecraft/block/NoteBlock.hpp"
#include "net/minecraft/block/SignBlock.hpp"
#include "net/minecraft/block/SpawnerBlock.hpp"
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/JukeboxBlockEntity.hpp"
#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/block/entity/NoteBlockBlockEntity.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
namespace net::minecraft::block {
std::unique_ptr<entity::BlockEntity> ChestBlock::createBlockEntity() {
 return std::make_unique<entity::ChestBlockEntity>();
}
std::unique_ptr<entity::BlockEntity> DispenserBlock::createBlockEntity() {
 return std::make_unique<entity::DispenserBlockEntity>();
}
std::unique_ptr<entity::BlockEntity> SignBlock::createBlockEntity() {
 return std::make_unique<entity::SignBlockEntity>();
}
std::unique_ptr<entity::BlockEntity> JukeboxBlock::createBlockEntity() {
 return std::make_unique<entity::JukeboxBlockEntity>();
}
std::unique_ptr<entity::BlockEntity> NoteBlock::createBlockEntity() {
 return std::make_unique<entity::NoteBlockBlockEntity>();
}
std::unique_ptr<entity::BlockEntity> SpawnerBlock::createBlockEntity() {
 return std::make_unique<entity::MobSpawnerBlockEntity>();
}
} // namespace net::minecraft::block
