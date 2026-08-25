#include <gtest/gtest.h>
#include "net/minecraft/client/particle/FlameParticle.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::particle::test {
namespace {
class ParticleProbe final : public Particle {
 public:
 ParticleProbe() : Particle(nullptr, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0) {}
 float getBrightnessAtEyes(float) const override {
  return 1.0f;
 }
};
}

TEST(ParticleLighting, RenderDoesNotInheritStalePackedLight) {
 render::Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 tessellator.light(0.0f, 0.0f);
 tessellator.startQuads();
 ParticleProbe particle;
 particle.render(tessellator, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
 const render::TessellatorMesh mesh = tessellator.takeMesh();
 ASSERT_EQ(mesh.vertices.size(), 4U);
 for(const render::TessellatorVertex& vertex : mesh.vertices) {
  EXPECT_EQ(vertex.light, 0x00F000F0);
 }
}

TEST(ParticleLighting, FlameParticlesStayFullBright) {
 FlameParticle particle(nullptr, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
 particle.particleAge = particle.maxParticleAge;
 EXPECT_FLOAT_EQ(particle.getBrightnessAtEyes(0.5f), 1.0f);
}
}
