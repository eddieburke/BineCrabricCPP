#include <gtest/gtest.h>
#include <cstdint>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::util::concurrent::Domain;
using net::minecraft::util::concurrent::ThreadCoordinator;
// ThreadCoordinator is a process-wide singleton whose configure() is idempotent by
// design, so every test that asserts against a specific budget must establish that
// budget itself. ctest runs one test per process (gtest_discover_tests), so a test that
// relied on BudgetDerivation8Core having run first saw a coordinator configured from
// whatever core count it passed — 32 rather than 8 — and read a different budget.
// Calling this first is a no-op when the 8-core baseline is already installed, so the
// tests behave identically whether run alone or batched.
void configure8CoreBaseline() {
 ThreadCoordinator::instance().configure(8, 2, {.maxComputeThreads = 8});
}
TEST(ThreadCoordinatorTest, BudgetDerivation8Core) {
 configure8CoreBaseline();
 const auto budget = ThreadCoordinator::instance().budget();
 EXPECT_EQ(budget.cpuBudget, 6U);
 EXPECT_EQ(budget.glCompile, 1U);
 EXPECT_EQ(budget.io, 2U);
 EXPECT_EQ(budget.compute, 3U);
}
TEST(ThreadCoordinatorTest, ConfigureIsIdempotent) {
 configure8CoreBaseline();
 // A second configure with a wildly different core count must not move the budget.
 ThreadCoordinator::instance().configure(32, 2, {.maxComputeThreads = 8});
 const auto budget = ThreadCoordinator::instance().budget();
 EXPECT_EQ(budget.cpuBudget, 6U);
 EXPECT_EQ(budget.compute, 3U);
}
TEST(ThreadCoordinatorTest, PoolCountsMatchBudget) {
 configure8CoreBaseline();
 EXPECT_EQ(ThreadCoordinator::instance().pool(Domain::Compute).threadCount(), 3U);
 EXPECT_EQ(ThreadCoordinator::instance().pool(Domain::Io).threadCount(), 2U);
 EXPECT_EQ(ThreadCoordinator::instance().pool(Domain::GlCompile).threadCount(), 1U);
}
TEST(ThreadCoordinatorTest, ComputeShare) {
 configure8CoreBaseline();
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
