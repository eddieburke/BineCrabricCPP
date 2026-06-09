#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/TreeFeatureHelpers.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <vector>

namespace net::minecraft {

class World;

class LargeOakTreeFeature : public Feature {
public:
    static constexpr std::array<std::int8_t, 6> MINOR_AXES = {2, 0, 0, 1, 2, 1};

    JavaRandom random;
    World* world = nullptr;
    std::array<int, 3> origin{};
    int height = 0;
    int trunkHeight = 0;
    double trunkScale = 0.618;
    double branchDensity = 1.0;
    double branchSlope = 0.381;
    double branchLengthScale = 1.0;
    double foliageDensity = 1.0;
    int trunkWidth = 1;
    int maxTrunkHeight = 12;
    int foliageClusterHeight = 4;
    std::vector<std::array<int, 4>> branches;

    void makeBranches()
    {
        trunkHeight = static_cast<int>(static_cast<double>(height) * trunkScale);
        if (trunkHeight >= height) {
            trunkHeight = height - 1;
        }
        int branchCount = static_cast<int>(1.382 + std::pow(foliageDensity * static_cast<double>(height) / 13.0, 2.0));
        if (branchCount < 1) {
            branchCount = 1;
        }
        std::vector<std::array<int, 4>> branchScratch(static_cast<std::size_t>(branchCount * height));
        int leafBaseY = origin[1] + height - foliageClusterHeight;
        int usedBranches = 1;
        const int trunkTopY = origin[1] + trunkHeight;
        int heightFromOrigin = leafBaseY - origin[1];
        branchScratch[0] = {origin[0], leafBaseY--, origin[2], trunkTopY};
        while (heightFromOrigin >= 0) {
            const float shape = getTreeShape(heightFromOrigin);
            if (shape < 0.0f) {
                --leafBaseY;
                --heightFromOrigin;
                continue;
            }
            constexpr double centerOffset = 0.5;
            for (int i = 0; i < branchCount; ++i) {
                const double length = branchLengthScale * (static_cast<double>(shape) * (static_cast<double>(random.nextFloat()) + 0.328));
                const double angle = static_cast<double>(random.nextFloat()) * 2.0 * 3.14159;
                const int branchX = MathHelper::floor(length * std::sin(angle) + static_cast<double>(origin[0]) + centerOffset);
                const int branchZ = MathHelper::floor(length * std::cos(angle) + static_cast<double>(origin[2]) + centerOffset);
                const std::array<int, 3> branchStart = {branchX, leafBaseY, branchZ};
                const std::array<int, 3> branchEnd = {branchX, leafBaseY + foliageClusterHeight, branchZ};
                if (tryBranch(branchStart, branchEnd) != -1) {
                    continue;
                }
                std::array<int, 3> trunkStart = {origin[0], origin[1], origin[2]};
                const double horizontalDistance = std::sqrt(std::pow(std::abs(origin[0] - branchStart[0]), 2.0)
                    + std::pow(std::abs(origin[2] - branchStart[2]), 2.0));
                const double yDrop = horizontalDistance * branchSlope;
                trunkStart[1] = static_cast<double>(branchStart[1]) - yDrop > static_cast<double>(trunkTopY)
                    ? trunkTopY
                    : static_cast<int>(static_cast<double>(branchStart[1]) - yDrop);
                if (tryBranch(trunkStart, branchStart) != -1) {
                    continue;
                }
                branchScratch[static_cast<std::size_t>(usedBranches)] = {branchX, leafBaseY, branchZ, trunkStart[1]};
                ++usedBranches;
            }
            --leafBaseY;
            --heightFromOrigin;
        }
        branches.assign(branchScratch.begin(), branchScratch.begin() + usedBranches);
    }

    void placeCluster(int x, int y, int z, float shape, std::int8_t majorAxis, int clusterBlock)
    {
        if (world == nullptr) {
            return;
        }
        const int radius = static_cast<int>(static_cast<double>(shape) + 0.618);
        const std::int8_t minorAxisA = MINOR_AXES[static_cast<std::size_t>(majorAxis)];
        const std::int8_t minorAxisB = MINOR_AXES[static_cast<std::size_t>(majorAxis + 3)];
        const std::array<int, 3> center = {x, y, z};
        std::array<int, 3> pos = {0, 0, 0};
        pos[static_cast<std::size_t>(majorAxis)] = center[static_cast<std::size_t>(majorAxis)];
        for (int i = -radius; i <= radius; ++i) {
            pos[static_cast<std::size_t>(minorAxisA)] = center[static_cast<std::size_t>(minorAxisA)] + i;
            for (int j = -radius; j <= radius; ++j) {
                const double distance = std::sqrt(std::pow(static_cast<double>(std::abs(i)) + 0.5, 2.0)
                    + std::pow(static_cast<double>(std::abs(j)) + 0.5, 2.0));
                if (distance > static_cast<double>(shape)) {
                    continue;
                }
                pos[static_cast<std::size_t>(minorAxisB)] = center[static_cast<std::size_t>(minorAxisB)] + j;
                const int blockId = world->getBlockId(pos[0], pos[1], pos[2]);
                if (blockId != 0 && blockId != tree_feature::id(Block::LEAVES)) {
                    continue;
                }
                world->setBlockWithoutNotifyingNeighbors(pos[0], pos[1], pos[2], clusterBlock);
            }
        }
    }

    [[nodiscard]] float getTreeShape(int layer) const
    {
        if (static_cast<double>(layer) < static_cast<double>(height) * 0.3) {
            return -1.618f;
        }
        const float halfHeight = static_cast<float>(height) / 2.0f;
        const float centeredLayer = static_cast<float>(height) / 2.0f - static_cast<float>(layer);
        float shape = centeredLayer == 0.0f
            ? halfHeight
            : (MathHelper::abs(centeredLayer) >= halfHeight
                  ? 0.0f
                  : static_cast<float>(std::sqrt(std::pow(MathHelper::abs(halfHeight), 2.0f)
                      - std::pow(MathHelper::abs(centeredLayer), 2.0f))));
        return shape *= 0.5f;
    }

    [[nodiscard]] float getClusterShape(int layer) const
    {
        if (layer < 0 || layer >= foliageClusterHeight) {
            return -1.0f;
        }
        if (layer == 0 || layer == foliageClusterHeight - 1) {
            return 2.0f;
        }
        return 3.0f;
    }

    void placeFoliageCluster(int x, int baseY, int z)
    {
        const int topY = baseY + foliageClusterHeight;
        for (int y = baseY; y < topY; ++y) {
            placeCluster(x, y, z, getClusterShape(y - baseY), 1, tree_feature::id(Block::LEAVES));
        }
    }

    void placeBranch(const std::array<int, 3>& from, const std::array<int, 3>& to, int log)
    {
        if (world == nullptr) {
            return;
        }
        std::array<int, 3> delta = {0, 0, 0};
        int majorAxis = 0;
        for (int axis = 0; axis < 3; ++axis) {
            delta[axis] = to[axis] - from[axis];
            if (std::abs(delta[axis]) > std::abs(delta[majorAxis])) {
                majorAxis = axis;
            }
        }
        if (delta[majorAxis] == 0) {
            return;
        }
        const std::int8_t minorAxisA = MINOR_AXES[static_cast<std::size_t>(majorAxis)];
        const std::int8_t minorAxisB = MINOR_AXES[static_cast<std::size_t>(majorAxis + 3)];
        const int step = delta[majorAxis] > 0 ? 1 : -1;
        const double ratioA = static_cast<double>(delta[static_cast<std::size_t>(minorAxisA)]) / static_cast<double>(delta[majorAxis]);
        const double ratioB = static_cast<double>(delta[static_cast<std::size_t>(minorAxisB)]) / static_cast<double>(delta[majorAxis]);
        std::array<int, 3> pos = {0, 0, 0};
        const int endStep = delta[majorAxis] + step;
        for (int i = 0; i != endStep; i += step) {
            pos[static_cast<std::size_t>(majorAxis)] = MathHelper::floor(static_cast<double>(from[majorAxis] + i) + 0.5);
            pos[static_cast<std::size_t>(minorAxisA)] = MathHelper::floor(static_cast<double>(from[static_cast<std::size_t>(minorAxisA)])
                + static_cast<double>(i) * ratioA + 0.5);
            pos[static_cast<std::size_t>(minorAxisB)] = MathHelper::floor(static_cast<double>(from[static_cast<std::size_t>(minorAxisB)])
                + static_cast<double>(i) * ratioB + 0.5);
            world->setBlockWithoutNotifyingNeighbors(pos[0], pos[1], pos[2], log);
        }
    }

    void placeFoliage()
    {
        for (const auto& branch : branches) {
            placeFoliageCluster(branch[0], branch[1], branch[2]);
        }
    }

    [[nodiscard]] bool shouldPlaceBranch(int branchHeight) const
    {
        return !(static_cast<double>(branchHeight) < static_cast<double>(height) * 0.2);
    }

    void PlaceTrunk()
    {
        std::array<int, 3> from = {origin[0], origin[1], origin[2]};
        std::array<int, 3> to = {origin[0], origin[1] + trunkHeight, origin[2]};
        placeBranch(from, to, tree_feature::id(Block::LOG));
        if (trunkWidth == 2) {
            ++from[0];
            ++to[0];
            placeBranch(from, to, tree_feature::id(Block::LOG));
            ++from[2];
            ++to[2];
            placeBranch(from, to, tree_feature::id(Block::LOG));
            --from[0];
            --to[0];
            placeBranch(from, to, tree_feature::id(Block::LOG));
        }
    }

    void placeBranches()
    {
        std::array<int, 3> trunkStart = {origin[0], origin[1], origin[2]};
        for (const auto& branch : branches) {
            const std::array<int, 3> branchStart = {branch[0], branch[1], branch[2]};
            trunkStart[1] = branch[3];
            const int branchHeight = trunkStart[1] - origin[1];
            if (!shouldPlaceBranch(branchHeight)) {
                continue;
            }
            placeBranch(trunkStart, branchStart, tree_feature::id(Block::LOG));
        }
    }

    [[nodiscard]] int tryBranch(const std::array<int, 3>& from, const std::array<int, 3>& to)
    {
        if (world == nullptr) {
            return 0;
        }
        std::array<int, 3> delta = {0, 0, 0};
        int majorAxis = 0;
        for (int axis = 0; axis < 3; ++axis) {
            delta[axis] = to[axis] - from[axis];
            if (std::abs(delta[axis]) > std::abs(delta[majorAxis])) {
                majorAxis = axis;
            }
        }
        if (delta[majorAxis] == 0) {
            return -1;
        }
        const std::int8_t minorAxisA = MINOR_AXES[static_cast<std::size_t>(majorAxis)];
        const std::int8_t minorAxisB = MINOR_AXES[static_cast<std::size_t>(majorAxis + 3)];
        const int step = delta[majorAxis] > 0 ? 1 : -1;
        const double ratioA = static_cast<double>(delta[static_cast<std::size_t>(minorAxisA)]) / static_cast<double>(delta[majorAxis]);
        const double ratioB = static_cast<double>(delta[static_cast<std::size_t>(minorAxisB)]) / static_cast<double>(delta[majorAxis]);
        std::array<int, 3> pos = {0, 0, 0};
        const int endStep = delta[majorAxis] + step;
        int i = 0;
        for (; i != endStep; i += step) {
            pos[static_cast<std::size_t>(majorAxis)] = from[static_cast<std::size_t>(majorAxis)] + i;
            pos[static_cast<std::size_t>(minorAxisA)] = MathHelper::floor(static_cast<double>(from[static_cast<std::size_t>(minorAxisA)])
                + static_cast<double>(i) * ratioA);
            pos[static_cast<std::size_t>(minorAxisB)] = MathHelper::floor(static_cast<double>(from[static_cast<std::size_t>(minorAxisB)])
                + static_cast<double>(i) * ratioB);
            const int blockId = world->getBlockId(pos[0], pos[1], pos[2]);
            if (blockId != 0 && blockId != tree_feature::id(Block::LEAVES)) {
                break;
            }
        }
        if (i == endStep) {
            return -1;
        }
        return std::abs(i);
    }

    [[nodiscard]] bool canPlace()
    {
        if (world == nullptr) {
            return false;
        }
        const std::array<int, 3> from = {origin[0], origin[1], origin[2]};
        const std::array<int, 3> to = {origin[0], origin[1] + height - 1, origin[2]};
        const int ground = world->getBlockId(origin[0], origin[1] - 1, origin[2]);
        if (!tree_feature::isGrassOrDirt(ground)) {
            return false;
        }
        const int branchCheck = tryBranch(from, to);
        if (branchCheck == -1) {
            return true;
        }
        if (branchCheck < 6) {
            return false;
        }
        height = branchCheck;
        return true;
    }

    void prepare(double heightScale, double branchScale, double leafDensity) override
    {
        maxTrunkHeight = static_cast<int>(heightScale * 12.0);
        if (heightScale > 0.5) {
            foliageClusterHeight = 5;
        }
        branchLengthScale = branchScale;
        foliageDensity = leafDensity;
    }


    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        this->world = world;
        this->random.setSeed(static_cast<std::uint64_t>(random.nextLong()));
        origin[0] = x;
        origin[1] = y;
        origin[2] = z;
        if (height == 0) {
            height = 5 + this->random.nextInt(maxTrunkHeight);
        }
        if (!canPlace()) {
            return false;
        }
        makeBranches();
        placeFoliage();
        PlaceTrunk();
        placeBranches();
        return true;
    }
};

} // namespace net::minecraft
