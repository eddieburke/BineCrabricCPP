#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/ViewDistance.hpp"

#include <cassert>
#include <cmath>

namespace {

using net::minecraft::client::option::GameOptions;
using net::minecraft::client::render::ViewDistance;

void assertNear(float actual, float expected)
{
    assert(std::fabs(actual - expected) < 0.0001f);
}

} // namespace

int main()
{
    GameOptions options;

    options.viewDistance = 0;
    options.ofRenderScale = 5.0f;
    ViewDistance far = ViewDistance::fromOptions(options);
    assert(far.baseDistanceBlocks() == 256);
    assertNear(far.renderScale(), 5.0f);
    assertNear(far.renderDistanceBlocks(), 1280.0f);
    assert(far.chunkGridDiameterBlocks() == 2000);
    assert(far.chunkGridRadiusChunks() == 63);
    assertNear(far.fogDistanceBlocks(), 1280.0f);
    assertNear(far.projectionNearClipBlocks(), 0.05f);
    assertNear(far.projectionFarClipBlocks(), 2560.0f);
    assertNear(far.skyFogEndBlocks(), 1024.0f);
    assertNear(far.worldFogStartBlocks(0.25f), 320.0f);
    assertNear(far.worldFogEndBlocks(1.0f), 1280.0f);
    assert(far.shouldRenderSky());

    options.ofRenderScale = 1.0f;
    ViewDistance farDefaultScale = ViewDistance::fromOptions(options);
    assert(farDefaultScale.baseDistanceBlocks() == far.baseDistanceBlocks());
    assertNear(farDefaultScale.renderScale(), 1.0f);
    assertNear(farDefaultScale.renderDistanceBlocks(), 256.0f);
    assert(farDefaultScale.chunkGridDiameterBlocks() == 400);
    assert(farDefaultScale.chunkGridRadiusChunks() == 13);
    assert(farDefaultScale.worldChunkResidentRadiusChunks() == 15);
    assertNear(farDefaultScale.fogDistanceBlocks(), 256.0f);
    assertNear(farDefaultScale.projectionFarClipBlocks(), 512.0f);

    options.viewDistance = 3;
    options.ofPreloadedChunks = 6;
    ViewDistance tiny = ViewDistance::fromOptions(options);
    assert(tiny.baseDistanceBlocks() == 32);
    assertNear(tiny.renderDistanceBlocks(), 32.0f);
    assert(tiny.chunkGridDiameterBlocks() == 64);
    assert(tiny.chunkGridRadiusChunks() == 2);
    assert(tiny.chunkPreloadMarginChunks() == 5);
    assert(tiny.worldChunkResidentRadiusChunks() == 7);
    assert(tiny.worldChunkPreloadRadiusChunks() == 7);
    assert(!tiny.shouldRenderSky());

    options.ofRenderScale = 5.0f;
    ViewDistance scaledTiny = ViewDistance::fromOptions(options);
    assert(scaledTiny.baseDistanceBlocks() == 32);
    assertNear(scaledTiny.renderDistanceBlocks(), 160.0f);
    assert(scaledTiny.chunkGridDiameterBlocks() == 320);
    assert(scaledTiny.chunkGridRadiusChunks() == 10);
    assert(scaledTiny.worldChunkResidentRadiusChunks() == 15);
    assert(scaledTiny.worldChunkPreloadRadiusChunks() == 15);
    assertNear(scaledTiny.fogDistanceBlocks(), 160.0f);
    assertNear(scaledTiny.projectionFarClipBlocks(), 320.0f);
    assertNear(scaledTiny.worldFogEndBlocks(1.0f), 160.0f);
}
