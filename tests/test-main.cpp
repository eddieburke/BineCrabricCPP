#include <gtest/gtest.h>
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"

int main(int argc, char** argv) {
 testing::InitGoogleTest(&argc, argv);
 net::minecraft::util::concurrent::setMainThread();
 net::minecraft::registry::Registry::bootstrap();
 return RUN_ALL_TESTS();
}
