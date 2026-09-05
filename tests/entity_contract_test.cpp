#include <gtest/gtest.h>
#include <vector>
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity::test {
namespace {
class LivingEntityProbe : public LivingEntity {
 public:
 using LivingEntity::LivingEntity;
 using LivingEntity::tickLiving;
};
}
TEST(EntityContract, PlayerStandingEyeHeightDispatchesThroughEntity) {
 player::PlayerEntity player;
 Entity& entity = player;
 EXPECT_DOUBLE_EQ(entity.getStandingEyeHeight(), player.getStandingEyeHeight());
}
TEST(EntityContract, BaseLivingTickClearsStaleJumpInputOutsideFluid) {
 LivingEntityProbe entity;
 entity.jumping = true;
 entity.tickLiving();
 EXPECT_FALSE(entity.jumping);
}
TEST(EntityContract, WorldEntityIndexTracksReassignmentAndUnload) {
 World world("entity-index", 42);
 Entity first(&world);
 Entity second(&world);
 first.id = 1001;
 second.id = 1002;
 std::vector<Entity*> entities{&first, &second};
 world.addEntities(entities);
 EXPECT_EQ(world.findEntity(1001), &first);
 EXPECT_EQ(world.findEntity(1002), &second);
 world.reassignEntityId(first, 1003);
 EXPECT_EQ(world.findEntity(1001), nullptr);
 EXPECT_EQ(world.findEntity(1003), &first);
 std::vector<Entity*> unloading{&second};
 world.unloadEntities(unloading);
 world.updateEntityLists();
 EXPECT_EQ(world.findEntity(1002), nullptr);
}
}
