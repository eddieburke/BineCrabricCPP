#include <gtest/gtest.h>
#include "net/minecraft/registry/Registry.hpp"
int main(int argc, char** argv) {
 testing::InitGoogleTest(&argc, argv);
 net::minecraft::registry::Registry::bootstrap();
 return RUN_ALL_TESTS();
}
