#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include <algorithm>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
namespace net::minecraft::client::render {
void PackInstance::clearGpuResources() {
 colorTargets.destroy();
 publishedTextures.clear();
 images.clear();
 noiseTexture.reset();
 noiseResolution = 0;
 for(int i = 0; i < 2; ++i) {
  depthTextures[i].reset();
  depthTextureW[i] = 0;
  depthTextureH[i] = 0;
 }
 for(auto& [name, texture] : customTextures) {
  if(texture != 0 && ownedCustomTextures.contains(texture)) {
   core::deleteTexture(texture);
  }
 }
 customTextures.clear();
 worldTextures.clear();
 worldVolumeTextures.clear();
 ownedCustomTextures.clear();
 std::fill(std::begin(bufferBytes), std::end(bufferBytes), 0);
 setupWidth = 0;
 setupHeight = 0;
}
PackInstance::~PackInstance() {
 clearGpuResources();
}
void PackInstance::resetPrograms() {
 for(auto& plan : stagePlans) plan.clear();
 setupPlan.clear();
 for(auto& variants : worldPrograms) {
  for(WorldProgramRuntime& runtime : variants) runtime = {};
 }
 executionPlanReady = false;
 stagePlanValid.fill(false);
 compiledPrograms.clear();
 programDrawBuffers.clear();
 logged.clear();
 programs = shaderBinaryCache != nullptr
                ? std::make_unique<gl::ProgramCache>(shaderBinaryCache)
                : std::make_unique<gl::ProgramCache>(shaderCacheDirectory);
}
const std::vector<PackInstance::RuntimeOperation>& PackInstance::stagePlan(CompositeStage stage) const noexcept {
 return stagePlans[static_cast<std::size_t>(stage)];
}
bool PackInstance::buildExecutionPlan(std::string& error) {
 for(auto& plan : stagePlans) plan.clear();
 setupPlan.clear();
 executionPlanReady = false;
 stagePlanValid.fill(true);
 for(auto& variants : worldPrograms) {
  for(WorldProgramRuntime& runtime : variants) runtime = {};
 }
 const auto append = [&](std::vector<RuntimePass>& destination, std::size_t passIndex) -> bool {
  if(passIndex >= definition.passes.size()) return true;
  const PackPass& pass = definition.passes[passIndex];
  const auto found = compiledPrograms.find(pass.program);
  if(found == compiledPrograms.end() || found->second.failed || found->second.program == nullptr) {
   if(!error.empty()) error += "; ";
   error += "pass '" + pass.name + "' program '" + pass.program + "' was not compiled during activation";
   return false;
  }
  destination.push_back({passIndex, found->second.program});
  return true;
 };
 std::array<std::vector<RuntimePass>, static_cast<std::size_t>(CompositeStage::Count)> rasters;
 std::array<std::vector<RuntimePass>, static_cast<std::size_t>(CompositeStage::Count)> computes;
 const auto appendAll = [&](CompositeStage stage, const std::vector<std::size_t>& source) {
  auto& destination = rasters[static_cast<std::size_t>(stage)];
  destination.reserve(source.size());
  for(std::size_t passIndex : source) {
   if(!append(destination, passIndex)) stagePlanValid[static_cast<std::size_t>(stage)] = false;
  }
 };
 appendAll(CompositeStage::Begin, beginPasses);
 appendAll(CompositeStage::ShadowComposite, shadowCompositePasses);
 appendAll(CompositeStage::Prepare, preparePasses);
 appendAll(CompositeStage::Deferred, deferredPasses);
 appendAll(CompositeStage::Composite, postPasses);
 setupPlan.reserve(setupPasses.size());
 for(std::size_t passIndex : setupPasses) {
  // A setup pass that will not compile still has to enter the plan: setup runs once
  // per viewport size ahead of every stage, and dispatchSetupIfNeeded reports the
  // null program by refusing to mark the viewport as set up.
  if(!append(setupPlan, passIndex) && passIndex < definition.passes.size()) {
   setupPlan.push_back({passIndex, nullptr});
  }
 }
 for(std::size_t passIndex : computePasses) {
  if(passIndex >= definition.passes.size()) continue;
  const std::string& name = definition.passes[passIndex].name;
  CompositeStage stage;
  if(ComputeDispatcher::matchesStage(name, "begin"))
   stage = CompositeStage::Begin;
  else if(ComputeDispatcher::matchesStage(name, "shadowcomp"))
   stage = CompositeStage::ShadowComposite;
  else if(ComputeDispatcher::matchesStage(name, "prepare"))
   stage = CompositeStage::Prepare;
  else if(ComputeDispatcher::matchesStage(name, "deferred"))
   stage = CompositeStage::Deferred;
  else if(ComputeDispatcher::matchesStage(name, "composite") || ComputeDispatcher::matchesStage(name, "final")) {
   stage = CompositeStage::Composite;
  } else {
   continue;
  }
  if(!append(computes[static_cast<std::size_t>(stage)], passIndex)) {
   stagePlanValid[static_cast<std::size_t>(stage)] = false;
  }
 }
 for(auto& plan : computes) {
  std::stable_sort(plan.begin(), plan.end(), [this](const RuntimePass& a, const RuntimePass& b) {
   const PackPass& pa = definition.passes[a.passIndex];
   const PackPass& pb = definition.passes[b.passIndex];
   const std::string parentA = ComputeDispatcher::computeParentName(pa.name);
   const std::string parentB = ComputeDispatcher::computeParentName(pb.name);
   if(parentA != parentB) return ComputeDispatcher::lessComputeParent(parentA, parentB);
   return ComputeDispatcher::lessComputeOrder(pa, pb);
  });
 }
 struct ComputeGroup {
  std::string parent;
  std::vector<RuntimePass> passes;
  bool emitted = false;
 };
 for(std::size_t stageIndex = 0; stageIndex < stagePlans.size(); ++stageIndex) {
  std::vector<ComputeGroup> groups;
  for(const RuntimePass& runtime : computes[stageIndex]) {
   const std::string parent = ComputeDispatcher::computeParentName(definition.passes[runtime.passIndex].name);
   if(groups.empty() || groups.back().parent != parent) {
    groups.emplace_back();
    groups.back().parent = parent;
   }
   groups.back().passes.push_back(runtime);
  }
  auto& operations = stagePlans[stageIndex];
  const auto emit = [&](ComputeGroup& group) {
   for(std::size_t index = 0; index < group.passes.size(); ++index) {
    RuntimeOperation operation;
    operation.pass = group.passes[index];
    operation.compute = true;
    operation.groupBegin = index == 0;
    operation.groupEnd = index + 1 == group.passes.size();
    operations.push_back(std::move(operation));
   }
   group.emitted = true;
  };
  const auto& stageRasters = rasters[stageIndex];
  for(const RuntimePass& raster : stageRasters) {
   const std::string& rasterName = definition.passes[raster.passIndex].name;
   for(ComputeGroup& group : groups) {
    if(group.emitted) continue;
    const bool hasRaster = std::any_of(stageRasters.begin(), stageRasters.end(), [&](const RuntimePass& candidate) {
     return definition.passes[candidate.passIndex].name == group.parent;
    });
    if(!hasRaster && ComputeDispatcher::lessComputeParent(group.parent, rasterName)) emit(group);
   }
   for(ComputeGroup& group : groups) {
    if(!group.emitted && group.parent == rasterName) emit(group);
   }
   RuntimeOperation rasterOperation;
   rasterOperation.pass = raster;
   operations.push_back(std::move(rasterOperation));
  }
  for(ComputeGroup& group : groups) {
   if(!group.emitted) emit(group);
  }
 }
 for(std::size_t index = 0; index < static_cast<std::size_t>(WorldProgramId::Count); ++index) {
  const std::string requested(worldProgramKey(static_cast<WorldProgramId>(index)));
  for(std::size_t shadow = 0; shadow < 2; ++shadow) {
   std::string candidate = shadow != 0 ? irisShadowProgramForGbuffers(requested) : requested;
   if(candidate.empty()) continue;
   candidate = resolveProgramKey(definition, settings, candidate, programEnabledCache);
   if(candidate.empty()) continue;
   const auto found = compiledPrograms.find(candidate);
   if(found == compiledPrograms.end() || found->second.failed || found->second.program == nullptr) continue;
   worldPrograms[index][shadow] = {found->second.program, std::move(candidate)};
  }
 }
 executionPlanReady = true;
 return error.empty();
}
bool PackInstance::rebuildRuntime(std::string& error) {
 customUniforms.setOptions(settings);
 const bool compiled = customUniforms.compile(definition.customUniforms, error);
 resetPrograms();
 programEnabledCache.clear();
 includedSourceCache.clear();
 preparedSourceCache.clear();
 PackPassBuckets buckets;
 indexPackPasses(definition, settings, buckets, programEnabledCache);
 postPasses = std::move(buckets.postPasses);
 deferredPasses = std::move(buckets.deferredPasses);
 computePasses = std::move(buckets.computePasses);
 beginPasses = std::move(buckets.beginPasses);
 shadowCompositePasses = std::move(buckets.shadowCompositePasses);
 preparePasses = std::move(buckets.preparePasses);
 setupPasses = std::move(buckets.setupPasses);
 return compiled;
}
} // namespace net::minecraft::client::render
