#include <gtest/gtest.h>
#include <cstdint>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::util::concurrent::Domain;
using net::minecraft::util::concurrent::ThreadCoordinator;
TEST(ThreadCoordinatorTest, BudgetDerivation8Core) {
 ThreadCoordinator::instance().configure(8, 2, {.maxComputeThreads = 8});
 const auto budget = ThreadCoordinator::instance().budget();
 EXPECT_EQ(budget.cpuBudget, 6U);
 EXPECT_EQ(budget.glCompile, 1U);
 EXPECT_EQ(budget.io, 2U);
 EXPECT_EQ(budget.compute, 3U);
}
TEST(ThreadCoordinatorTest, ConfigureIsIdempotent) {
 ThreadCoordinator::instance().configure(32, 2, {.maxComputeThreads = 8});
 const auto budget = ThreadCoordinator::instance().budget();
 EXPECT_EQ(budget.cpuBudget, 6U);
 EXPECT_EQ(budget.compute, 3U);
}
TEST(ThreadCoordinatorTest, PoolCountsMatchBudget) {
 EXPECT_EQ(ThreadCoordinator::instance().pool(Domain::Compute).threadCount(), 3U);
 EXPECT_EQ(ThreadCoordinator::instance().pool(Domain::Io).threadCount(), 2U);
 EXPECT_EQ(ThreadCoordinator::instance().pool(Domain::GlCompile).threadCount(), 1U);
}
TEST(ThreadCoordinatorTest, ComputeShare) {
 EXPECT_EQ(ThreadCoordinator::instance().computeShare(3), 1U);
}
TEST(ThreadCoordinatorTest, ReserveDynamicTracksPending) {
 const std::uint64_t before = ThreadCoordinator::instance().totalPending();
 ThreadCoordinator::instance().reserveDynamic(2);
 EXPECT_EQ(ThreadCoordinator::instance().totalPending(), before + 2U);
 ThreadCoordinator::instance().releaseDynamic(2);
 EXPECT_EQ(ThreadCoordinator::instance().totalPending(), before);
}
} // namespace
} // namespace net::minecraft::test
