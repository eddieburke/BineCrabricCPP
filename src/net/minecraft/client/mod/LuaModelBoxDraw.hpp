#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/mod/lua/LuaBlockModel.hpp"

namespace net::minecraft::client::mod {
void emitModelBoxGeometry(render::Tessellator& tessellator,
                          const net::minecraft::mod::lua::ModelBox& box,
                          const net::minecraft::block::TerrainAtlasUv& uv);
}
