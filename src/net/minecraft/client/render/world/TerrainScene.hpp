#pragma once
#include <functional>
#include <vector>
namespace net::minecraft {
class World;
}
namespace net::minecraft::entity {
class Entity;
}
namespace net::minecraft::block::entity {
class BlockEntity;
}
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::option {
class GameOptions;
struct RenderSettings;
}
namespace net::minecraft::client::render {
// The scene the terrain systems render, decoupled from WorldRenderer: the
// world, the client, the frame camera, the per-frame resolved settings/options,
// the global block-entity list and the single callback a component may invoke
// back on its owner. WorldRenderer owns the struct and refreshes it at the top
// of every rendered pass (setCamera), so the values it exposes are the same
// ones the pass's own culling and compiling read.
struct TerrainScene {
  net::minecraft::World* world = nullptr;
  net::minecraft::client::Minecraft* client = nullptr;
  const net::minecraft::Entity* camera = nullptr;
  const option::RenderSettings* settings = nullptr;
  option::GameOptions* options = nullptr;
  // Block entities that render from the world's global list rather than a
  // section. Used to live on WorldRenderer; moved here so the section system
  // and compile pipeline can reach it without the facade. ChunkBuilder keeps a
  // pointer to it, so it must outlive every section (it does: the scene member
  // is declared before the section map in WorldRenderer).
  std::vector<net::minecraft::block::entity::BlockEntity*> blockEntities;
  // The only callback the components need from their owner. WorldRenderer binds
  // it to reload() in its constructor. May be unset (never invoked).
  std::function<void()> reloadRequested;
};
} // namespace net::minecraft::client::render
