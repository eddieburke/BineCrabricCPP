#include <gtest/gtest.h>
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
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
}
