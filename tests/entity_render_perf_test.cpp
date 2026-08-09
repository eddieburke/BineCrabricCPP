#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::test {
namespace {
class PerfStubEntity : public entity::Entity {
 public:
 using entity::Entity::Entity;
};
struct EntityRenderPerfResult {
 double totalMs = 0.0;
 double msPerFrame = 0.0;
 double usPerEntity = 0.0;
};
using FrameFn = std::function<void(client::render::entity::EntityRenderDispatcher&)>;
EntityRenderPerfResult measureEntityFrames(client::render::entity::EntityRenderDispatcher& dispatcher,
                                           int warmupFrames,
                                           int timedFrames,
                                           int entityCount,
                                           FrameFn frame) {
 for(int i = 0; i < warmupFrames; ++i) {
  frame(dispatcher);
 }
 const auto start = std::chrono::steady_clock::now();
 for(int i = 0; i < timedFrames; ++i) {
  frame(dispatcher);
 }
 const auto end = std::chrono::steady_clock::now();
 EntityRenderPerfResult result;
 result.totalMs = std::chrono::duration<double, std::milli>(end - start).count();
 result.msPerFrame = result.totalMs / static_cast<double>(timedFrames);
 result.usPerEntity = result.msPerFrame * 1000.0 / static_cast<double>(entityCount);
 return result;
}
void setPositions(entity::Entity& entity, int index) {
 const double x = static_cast<double>((index % 12) - 6) * 1.7;
 const double z = static_cast<double>((index / 12) % 12 - 6) * 1.7;
 const double y = 64.0 + static_cast<double>((index / 144) % 4) * 2.0;
 entity.setPositionAndAnglesKeepPrevAngles(x, y, z, static_cast<float>((index * 37) % 360), 0.0f);
}
std::vector<std::unique_ptr<entity::Entity>> makeHerd(World* world, int count) {
 static const char* kTypes[] = {"Cow", "Pig", "Chicken", "Sheep", "Zombie", "Skeleton"};
 std::vector<std::unique_ptr<entity::Entity>> herd;
 herd.reserve(static_cast<std::size_t>(count));
 for(int i = 0; i < count; ++i) {
  const char* type = kTypes[i % 6];
  std::unique_ptr<entity::Entity> entity = entity::EntityRegistry::create(type, world);
  EXPECT_NE(entity, nullptr) << "EntityRegistry::create failed for " << type;
  if(entity == nullptr) {
   continue;
  }
  setPositions(*entity, i);
  herd.push_back(std::move(entity));
 }
 return herd;
}
class EntityRenderPerf : public ::testing::Test {
 protected:
 void SetUp() override {
  world_ = std::make_unique<World>("EntityPerfWorld", 12345);
  options_.debugHud = false;
  options_.entityShadows = false;
  options_.fancyGraphics = true;
  matrices_.load(util::math::Matrix4f::identityMatrix());
  client::render::Tessellator::INSTANCE.setCaptureOnly(true);
  dispatcher_ = &client::render::entity::EntityRenderDispatcher::instance();
  dispatcher_->init(world_.get(), nullptr, nullptr, nullptr, &options_, 0.5f);
 }
 void TearDown() override {
  client::render::Tessellator::INSTANCE.setCaptureOnly(false);
 }
 std::unique_ptr<World> world_;
 client::option::GameOptions options_;
 util::math::MatrixStack matrices_;
 client::render::entity::EntityRenderDispatcher* dispatcher_ = nullptr;
};
} // namespace
TEST_F(EntityRenderPerf, LivingEntityFullRenderPath) {
 ASSERT_NE(dispatcher_->get(std::type_index(typeid(entity::passive::CowEntity))), nullptr);
 std::vector<std::unique_ptr<entity::Entity>> herd = makeHerd(world_.get(), 100);
 std::vector<entity::Entity*> pointers;
 pointers.reserve(herd.size());
 for(const auto& entity : herd) {
  pointers.push_back(entity.get());
 }
 const double tickDelta = 0.5f;
 const int warmupFrames = 200;
 const int timedFrames = 2000;
 FrameFn frame = [&](client::render::entity::EntityRenderDispatcher& dispatcher) {
  for(entity::Entity* entity : pointers) {
   dispatcher.render(*entity, tickDelta, matrices_, util::math::Matrix4f::identityMatrix());
  }
 };
 const EntityRenderPerfResult result =
     measureEntityFrames(*dispatcher_, warmupFrames, timedFrames, static_cast<int>(pointers.size()), frame);
 std::printf("[PERF_ENTITY] full render path: %d entities x %d frames: %.2f ms total, %.3f ms/frame, %.1f us/entity\n",
             static_cast<int>(pointers.size()),
             timedFrames,
             result.totalMs,
             result.msPerFrame,
             result.usPerEntity);
 std::fflush(stdout);
 EXPECT_LT(result.usPerEntity, 1000.0) << "per-entity CPU cost blew past a full ms; a regression or a "
                                          "pathological allocation was reintroduced";
}
TEST_F(EntityRenderPerf, DispatcherOverheadOnly) {
 std::vector<std::unique_ptr<entity::Entity>> stubs;
 stubs.reserve(100);
 for(int i = 0; i < 100; ++i) {
  auto entity = std::make_unique<PerfStubEntity>(world_.get());
  setPositions(*entity, i);
  stubs.push_back(std::move(entity));
 }
 std::vector<entity::Entity*> pointers;
 pointers.reserve(stubs.size());
 for(const auto& entity : stubs) {
  pointers.push_back(entity.get());
 }
 ASSERT_EQ(dispatcher_->get(std::type_index(typeid(PerfStubEntity))), nullptr);
 const double tickDelta = 0.5f;
 const int warmupFrames = 2000;
 const int timedFrames = 20000;
 FrameFn frame = [&](client::render::entity::EntityRenderDispatcher& dispatcher) {
  for(entity::Entity* entity : pointers) {
   dispatcher.render(*entity, tickDelta, matrices_, util::math::Matrix4f::identityMatrix());
  }
 };
 const EntityRenderPerfResult result =
     measureEntityFrames(*dispatcher_, warmupFrames, timedFrames, static_cast<int>(pointers.size()), frame);
 std::printf("[PERF_ENTITY] dispatcher overhead only: %d entities x %d frames: %.2f ms total, %.3f ms/frame, %.2f us/entity\n",
             static_cast<int>(pointers.size()),
             timedFrames,
             result.totalMs,
             result.msPerFrame,
             result.usPerEntity);
 std::fflush(stdout);
 EXPECT_LT(result.usPerEntity, 200.0) << "dispatcher-only overhead (typeid, shader-id cache, renderer lookup, "
                                         "brightness) blew past 200 us per entity";
}
} // namespace net::minecraft::test
