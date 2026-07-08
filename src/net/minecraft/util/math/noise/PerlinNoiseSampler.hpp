#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft {
class PerlinNoiseSampler {
   public:
    explicit PerlinNoiseSampler(JavaRandom& random) {
        offsetX_ = random.nextDouble() * 256.0;
        offsetY_ = random.nextDouble() * 256.0;
        offsetZ_ = random.nextDouble() * 256.0;
        for (int i = 0; i < 256; ++i) {
            perm_[i] = i;
        }
        for (int i = 0; i < 256; ++i) {
            const int swapIndex = random.nextInt(256 - i) + i;
            std::swap(perm_[i], perm_[swapIndex]);
            perm_[i + 256] = perm_[i];
        }
    }

    [[nodiscard]] double sample(double x, double y, double z = 0.0) const {
        double px = x + offsetX_;
        double py = y + offsetY_;
        double pz = z + offsetZ_;
        int ix = static_cast<int>(px);
        int iy = static_cast<int>(py);
        int iz = static_cast<int>(pz);
        if (px < static_cast<double>(ix)) {
            --ix;
        }
        if (py < static_cast<double>(iy)) {
            --iy;
        }
        if (pz < static_cast<double>(iz)) {
            --iz;
        }
        const int xMask = ix & 0xFF;
        const int yMask = iy & 0xFF;
        const int zMask = iz & 0xFF;
        px -= static_cast<double>(ix);
        py -= static_cast<double>(iy);
        pz -= static_cast<double>(iz);
        const double fadeX = fade(px);
        const double fadeY = fade(py);
        const double fadeZ = fade(pz);
        const int a = perm_[xMask] + yMask;
        const int aa = perm_[a] + zMask;
        const int ab = perm_[a + 1] + zMask;
        const int b = perm_[xMask + 1] + yMask;
        const int ba = perm_[b] + zMask;
        const int bb = perm_[b + 1] + zMask;
        return lerp(
            fadeZ,
            lerp(fadeY,
                 lerp(fadeX, grad(perm_[aa], px, py, pz), grad(perm_[ba], px - 1.0, py, pz)),
                 lerp(fadeX, grad(perm_[ab], px, py - 1.0, pz), grad(perm_[bb], px - 1.0, py - 1.0, pz))),
            lerp(fadeY,
                 lerp(fadeX, grad(perm_[aa + 1], px, py, pz - 1.0), grad(perm_[ba + 1], px - 1.0, py, pz - 1.0)),
                 lerp(fadeX,
                      grad(perm_[ab + 1], px, py - 1.0, pz - 1.0),
                      grad(perm_[bb + 1], px - 1.0, py - 1.0, pz - 1.0))));
    }

    // Faithful 1:1 port of Java's create(). The height==1 branch uses the 2D
    // gradient (grad2) and a distinct index path; the 3D branch caches the four
    // edge lerps across the y loop, recomputing only when the y cell changes.
    // Do NOT replace this with a sample() loop: the 2D branch would diverge.
    void create(std::vector<double>& map,
                double x,
                double y,
                double z,
                int width,
                int height,
                int depth,
                double scaleX,
                double scaleY,
                double scaleZ,
                double amplitude) const {
        if (height == 1) {
            int permA = 0;
            int permAA = 0;
            int permB = 0;
            int permBB = 0;
            double edgeLerp0 = 0.0;
            double edgeLerp1 = 0.0;
            int outIndex = 0;
            const double invAmplitude = 1.0 / amplitude;
            for (int i = 0; i < width; ++i) {
                double fracX = (x + static_cast<double>(i)) * scaleX + offsetX_;
                int cellX = static_cast<int>(fracX);
                if (fracX < static_cast<double>(cellX)) {
                    --cellX;
                }
                int xMask = cellX & 0xFF;
                fracX -= static_cast<double>(cellX);
                double fadeX = fracX * fracX * fracX * (fracX * (fracX * 6.0 - 15.0) + 10.0);
                for (int j = 0; j < depth; ++j) {
                    double fracZ = (z + static_cast<double>(j)) * scaleZ + offsetZ_;
                    int cellZ = static_cast<int>(fracZ);
                    if (fracZ < static_cast<double>(cellZ)) {
                        --cellZ;
                    }
                    int zMask = cellZ & 0xFF;
                    fracZ -= static_cast<double>(cellZ);
                    double fadeZ = fracZ * fracZ * fracZ * (fracZ * (fracZ * 6.0 - 15.0) + 10.0);
                    permA = perm_[xMask] + 0;
                    permAA = perm_[permA] + zMask;
                    permB = perm_[xMask + 1] + 0;
                    permBB = perm_[permB] + zMask;
                    edgeLerp0 =
                        lerp(fadeX, grad2(perm_[permAA], fracX, fracZ), grad(perm_[permBB], fracX - 1.0, 0.0, fracZ));
                    edgeLerp1 = lerp(fadeX,
                                     grad(perm_[permAA + 1], fracX, 0.0, fracZ - 1.0),
                                     grad(perm_[permBB + 1], fracX - 1.0, 0.0, fracZ - 1.0));
                    double noiseValue = lerp(fadeZ, edgeLerp0, edgeLerp1);
                    map[static_cast<std::size_t>(outIndex++)] += noiseValue * invAmplitude;
                }
            }
            return;
        }
        int outIndex = 0;
        const double invAmplitude = 1.0 / amplitude;
        int prevYMask = -1;
        int permAY = 0;
        int permAAY = 0;
        int permABY = 0;
        int permBY = 0;
        int permBAY = 0;
        int permBBY = 0;
        double edgeLerp00 = 0.0;
        double edgeLerp01 = 0.0;
        double edgeLerp10 = 0.0;
        double edgeLerp11 = 0.0;
        for (int i = 0; i < width; ++i) {
            double fracX = (x + static_cast<double>(i)) * scaleX + offsetX_;
            int cellX = static_cast<int>(fracX);
            if (fracX < static_cast<double>(cellX)) {
                --cellX;
            }
            int xMask = cellX & 0xFF;
            fracX -= static_cast<double>(cellX);
            double fadeX = fracX * fracX * fracX * (fracX * (fracX * 6.0 - 15.0) + 10.0);
            for (int j = 0; j < depth; ++j) {
                double fracZ = (z + static_cast<double>(j)) * scaleZ + offsetZ_;
                int cellZ = static_cast<int>(fracZ);
                if (fracZ < static_cast<double>(cellZ)) {
                    --cellZ;
                }
                int zMask = cellZ & 0xFF;
                fracZ -= static_cast<double>(cellZ);
                double fadeZ = fracZ * fracZ * fracZ * (fracZ * (fracZ * 6.0 - 15.0) + 10.0);
                for (int k = 0; k < height; ++k) {
                    double fracY = (y + static_cast<double>(k)) * scaleY + offsetY_;
                    int cellY = static_cast<int>(fracY);
                    if (fracY < static_cast<double>(cellY)) {
                        --cellY;
                    }
                    int yMask = cellY & 0xFF;
                    fracY -= static_cast<double>(cellY);
                    double fadeY = fracY * fracY * fracY * (fracY * (fracY * 6.0 - 15.0) + 10.0);
                    if (k == 0 || yMask != prevYMask) {
                        prevYMask = yMask;
                        permAY = perm_[xMask] + yMask;
                        permAAY = perm_[permAY] + zMask;
                        permABY = perm_[permAY + 1] + zMask;
                        permBY = perm_[xMask + 1] + yMask;
                        permBAY = perm_[permBY] + zMask;
                        permBBY = perm_[permBY + 1] + zMask;
                        edgeLerp00 = lerp(fadeX,
                                          grad(perm_[permAAY], fracX, fracY, fracZ),
                                          grad(perm_[permBAY], fracX - 1.0, fracY, fracZ));
                        edgeLerp01 = lerp(fadeX,
                                          grad(perm_[permABY], fracX, fracY - 1.0, fracZ),
                                          grad(perm_[permBBY], fracX - 1.0, fracY - 1.0, fracZ));
                        edgeLerp10 = lerp(fadeX,
                                          grad(perm_[permAAY + 1], fracX, fracY, fracZ - 1.0),
                                          grad(perm_[permBAY + 1], fracX - 1.0, fracY, fracZ - 1.0));
                        edgeLerp11 = lerp(fadeX,
                                          grad(perm_[permABY + 1], fracX, fracY - 1.0, fracZ - 1.0),
                                          grad(perm_[permBBY + 1], fracX - 1.0, fracY - 1.0, fracZ - 1.0));
                    }
                    double lerpY0 = lerp(fadeY, edgeLerp00, edgeLerp01);
                    double lerpY1 = lerp(fadeY, edgeLerp10, edgeLerp11);
                    double noiseValue = lerp(fadeZ, lerpY0, lerpY1);
                    map[static_cast<std::size_t>(outIndex++)] += noiseValue * invAmplitude;
                }
            }
        }
    }

   private:
    static double fade(double value) {
        return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
    }

    static double lerp(double amount, double from, double to) {
        return from + amount * (to - from);
    }

    static double grad(int hash, double x, double y, double z) {
        const int h = hash & 0xF;
        const double u = h < 8 ? x : y;
        const double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    // Java's 2-arg gradCoord, used by the height==1 path.
    static double grad2(int hash, double x, double y) {
        const int hashBits = hash & 0xF;
        const double u = static_cast<double>(1 - ((hashBits & 8) >> 3)) * x;
        const double v = hashBits < 4 ? 0.0 : (hashBits == 12 || hashBits == 14 ? x : y);
        return ((hashBits & 1) == 0 ? u : -u) + ((hashBits & 2) == 0 ? v : -v);
    }

    std::array<int, 512> perm_{};
    double offsetX_ = 0.0;
    double offsetY_ = 0.0;
    double offsetZ_ = 0.0;
};
}  // namespace net::minecraft
