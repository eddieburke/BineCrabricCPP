#include <gtest/gtest.h>
#include "net/minecraft/util/concurrent/ThreadBudget.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::util::concurrent::ThreadBudget;
TEST(ThreadBudgetTest, WorkedTable8Core) {
 const ThreadBudget budget = ThreadBudget::derive(8);
 EXPECT_EQ(budget.cpuBudget, 6U);
 EXPECT_EQ(budget.glCompile, 1U);
 EXPECT_EQ(budget.io, 2U);
 EXPECT_EQ(budget.compute, 3U);
}
TEST(ThreadBudgetTest, WorkedTable16Core) {
 const ThreadBudget budget = ThreadBudget::derive(16);
 EXPECT_EQ(budget.cpuBudget, 14U);
 EXPECT_EQ(budget.glCompile, 2U);
 EXPECT_EQ(budget.io, 3U);
 EXPECT_EQ(budget.compute, 8U);
}
TEST(ThreadBudgetTest, WorkedTable32Core) {
 const ThreadBudget budget = ThreadBudget::derive(32);
 EXPECT_EQ(budget.cpuBudget, 30U);
 EXPECT_EQ(budget.glCompile, 2U);
 EXPECT_EQ(budget.io, 3U);
 EXPECT_EQ(budget.compute, 8U);
}
TEST(ThreadBudgetTest, GlDriverThreads) {
 EXPECT_EQ(ThreadBudget::derive(8).glDriverThreads(), 1U);
 EXPECT_EQ(ThreadBudget::derive(16).glDriverThreads(), 3U);
 EXPECT_EQ(ThreadBudget::derive(32).glDriverThreads(), 7U);
}
TEST(ThreadBudgetTest, ComputeShareSubdivides) {
 const ThreadBudget budget = ThreadBudget::derive(16);
 EXPECT_EQ(budget.computeShare(3), 2U);
 EXPECT_EQ(budget.computeShare(1), 8U);
 EXPECT_EQ(budget.computeShare(0), 8U);
 EXPECT_EQ(ThreadBudget::derive(8).computeShare(3), 1U);
}
TEST(ThreadBudgetTest, LowCoreCountsNeverZero) {
 const ThreadBudget budget = ThreadBudget::derive(2);
 EXPECT_EQ(budget.cpuBudget, 1U);
 EXPECT_EQ(budget.glCompile, 1U);
 EXPECT_EQ(budget.io, 2U);
 EXPECT_EQ(budget.compute, 1U);
}
} // namespace
} // namespace net::minecraft::test
