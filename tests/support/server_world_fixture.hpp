#pragma once
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include <gtest/gtest.h>
#include <memory>
namespace net::minecraft::test {
class ServerWorldFixture : public ::testing::Test {
protected:
  void SetUp() override {
    stat::Stats::initialize();
    server_ = std::make_unique<server::MinecraftServer>();
  }
  void TearDown() override {
    server_.reset();
  }
  server::MinecraftServer& server() {
    return *server_;
  }

private:
  std::unique_ptr<server::MinecraftServer> server_;
};
} // namespace net::minecraft::test
