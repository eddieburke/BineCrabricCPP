#pragma once
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include <vector>
namespace net::minecraft::client::render::chunk {
struct ModChunkMesh {
  int texture = 0;
  TessellatorMesh mesh;
};
struct ModMeshCollector {
  struct Entry {
    int texture;
    Tessellator tess;
  };
  // reserved upfront — references into `entries` must remain stable
  static constexpr int kMaxModTextures = 64;
  std::vector<Entry> entries;
  double chunkOffX = 0.0;
  double chunkOffY = 0.0;
  double chunkOffZ = 0.0;
  ModMeshCollector() { entries.reserve(kMaxModTextures); }
  Tessellator& tessFor(int textureId, Tessellator& terrain) {
    if(!net::minecraft::mod::isMod(textureId)) {
      return terrain;
    }
    for(Entry& e : entries) {
      if(e.texture == textureId) {
        return e.tess;
      }
    }
    entries.push_back({textureId, Tessellator{}});
    Tessellator& t = entries.back().tess;
    // Mod chunk meshes are built on worker threads alongside the terrain
    // tessellator; capture mode keeps a busy chunk from flushing through GL
    // off the render thread when its vertex buffer crosses the threshold.
    t.setCaptureOnly(true);
    t.startQuads();
    t.translate(chunkOffX, chunkOffY, chunkOffZ);
    return t;
  }
};
} // namespace net::minecraft::client::render::chunk
