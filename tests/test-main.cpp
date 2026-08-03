#include <gtest/gtest.h>
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
int main(int argc, char** argv) {
 testing::InitGoogleTest(&argc, argv);
 // The process's main thread is the one gtest runs bodies on, so mark it as such.
 // setMainThread() is otherwise only reached from GLCore's init path, which a test
 // binary never runs — leaving assertOnMainThread() (ThreadNames.hpp, active because
 // this build does not define NDEBUG) to abort the whole process the moment a test
 // drove main-thread-confined render state. That is what killed
 // MultiplayerParityUpdates.RespawnTeleportAppliesToReplacementPlayer with
 // STATUS_STACK_BUFFER_OVERRUN: applyDeferredRespawn -> respawnPlayer -> prepareWorld
 // -> ProgressRenderer::progressStart -> gui_proj::begin -> setAlphaTestRef -> abort.
 net::minecraft::util::concurrent::setMainThread();
 net::minecraft::registry::Registry::bootstrap();
 return RUN_ALL_TESTS();
}
