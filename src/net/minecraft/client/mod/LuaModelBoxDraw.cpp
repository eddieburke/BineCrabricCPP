#include "net/minecraft/client/mod/LuaModelBoxDraw.hpp"
namespace net::minecraft::client::mod {
void emitModelBoxGeometry(render::Tessellator& t, const net::minecraft::mod::lua::ModelBox& box,
                          const net::minecraft::block::TerrainAtlasUv& uv) {
  t.startQuads();
  t.normal(1.0f, 0.0f, 0.0f);
  t.vertex(box.maxX, box.maxY, box.minZ, uv.uMax, uv.vMin);
  t.vertex(box.maxX, box.minY, box.minZ, uv.uMax, uv.vMax);
  t.vertex(box.maxX, box.minY, box.maxZ, uv.uMin, uv.vMax);
  t.vertex(box.maxX, box.maxY, box.maxZ, uv.uMin, uv.vMin);
  t.normal(-1.0f, 0.0f, 0.0f);
  t.vertex(box.minX, box.maxY, box.maxZ, uv.uMax, uv.vMin);
  t.vertex(box.minX, box.minY, box.maxZ, uv.uMax, uv.vMax);
  t.vertex(box.minX, box.minY, box.minZ, uv.uMin, uv.vMax);
  t.vertex(box.minX, box.maxY, box.minZ, uv.uMin, uv.vMin);
  t.normal(0.0f, 0.0f, 1.0f);
  t.vertex(box.maxX, box.maxY, box.maxZ, uv.uMax, uv.vMin);
  t.vertex(box.maxX, box.minY, box.maxZ, uv.uMax, uv.vMax);
  t.vertex(box.minX, box.minY, box.maxZ, uv.uMin, uv.vMax);
  t.vertex(box.minX, box.maxY, box.maxZ, uv.uMin, uv.vMin);
  t.normal(0.0f, 0.0f, -1.0f);
  t.vertex(box.minX, box.maxY, box.minZ, uv.uMax, uv.vMin);
  t.vertex(box.minX, box.minY, box.minZ, uv.uMax, uv.vMax);
  t.vertex(box.maxX, box.minY, box.minZ, uv.uMin, uv.vMax);
  t.vertex(box.maxX, box.maxY, box.minZ, uv.uMin, uv.vMin);
  t.normal(0.0f, 1.0f, 0.0f);
  t.vertex(box.minX, box.maxY, box.minZ, uv.uMin, uv.vMin);
  t.vertex(box.minX, box.maxY, box.maxZ, uv.uMin, uv.vMax);
  t.vertex(box.maxX, box.maxY, box.maxZ, uv.uMax, uv.vMax);
  t.vertex(box.maxX, box.maxY, box.minZ, uv.uMax, uv.vMin);
  t.normal(0.0f, -1.0f, 0.0f);
  t.vertex(box.minX, box.minY, box.maxZ, uv.uMin, uv.vMin);
  t.vertex(box.minX, box.minY, box.minZ, uv.uMin, uv.vMax);
  t.vertex(box.maxX, box.minY, box.minZ, uv.uMax, uv.vMax);
  t.vertex(box.maxX, box.minY, box.maxZ, uv.uMax, uv.vMin);
  t.draw();
}
} // namespace net::minecraft::client::mod
