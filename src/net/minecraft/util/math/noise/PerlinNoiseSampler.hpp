#pragma once

#include "net/minecraft/util/math/Types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace net::minecraft {

class PerlinNoiseSampler {
public:
    explicit PerlinNoiseSampler(JavaRandom& random)
    {
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

    [[nodiscard]] double sample(double x, double y, double z = 0.0) const
    {
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

        return lerp(fadeZ,
            lerp(fadeY,
                lerp(fadeX, grad(perm_[aa], px, py, pz), grad(perm_[ba], px - 1.0, py, pz)),
                lerp(fadeX, grad(perm_[ab], px, py - 1.0, pz), grad(perm_[bb], px - 1.0, py - 1.0, pz))),
            lerp(fadeY,
                lerp(fadeX, grad(perm_[aa + 1], px, py, pz - 1.0), grad(perm_[ba + 1], px - 1.0, py, pz - 1.0)),
                lerp(fadeX, grad(perm_[ab + 1], px, py - 1.0, pz - 1.0), grad(perm_[bb + 1], px - 1.0, py - 1.0, pz - 1.0))));
    }

    // Faithful 1:1 port of Java's create(). The height==1 branch uses the 2D
    // gradient (grad2) and a distinct index path; the 3D branch caches the four
    // edge lerps across the y loop, recomputing only when the y cell changes.
    // Do NOT replace this with a sample() loop: the 2D branch would diverge.
    void create(std::vector<double>& map, double x, double y, double z, int width, int height, int depth, double scaleX, double scaleY, double scaleZ, double amplitude) const
    {
        if (height == 1) {
            int n = 0;
            int n2 = 0;
            int n3 = 0;
            int n4 = 0;
            double dEdge0 = 0.0;
            double dEdge1 = 0.0;
            int outIndex = 0;
            const double invAmplitude = 1.0 / amplitude;
            for (int i = 0; i < width; ++i) {
                double d4 = (x + static_cast<double>(i)) * scaleX + offsetX_;
                int n6 = static_cast<int>(d4);
                if (d4 < static_cast<double>(n6)) {
                    --n6;
                }
                int n7 = n6 & 0xFF;
                d4 -= static_cast<double>(n6);
                double d5 = d4 * d4 * d4 * (d4 * (d4 * 6.0 - 15.0) + 10.0);
                for (int j = 0; j < depth; ++j) {
                    double d6 = (z + static_cast<double>(j)) * scaleZ + offsetZ_;
                    int n8 = static_cast<int>(d6);
                    if (d6 < static_cast<double>(n8)) {
                        --n8;
                    }
                    int n9 = n8 & 0xFF;
                    d6 -= static_cast<double>(n8);
                    double d7 = d6 * d6 * d6 * (d6 * (d6 * 6.0 - 15.0) + 10.0);
                    n = perm_[n7] + 0;
                    n2 = perm_[n] + n9;
                    n3 = perm_[n7 + 1] + 0;
                    n4 = perm_[n3] + n9;
                    dEdge0 = lerp(d5, grad2(perm_[n2], d4, d6), grad(perm_[n4], d4 - 1.0, 0.0, d6));
                    dEdge1 = lerp(d5, grad(perm_[n2 + 1], d4, 0.0, d6 - 1.0), grad(perm_[n4 + 1], d4 - 1.0, 0.0, d6 - 1.0));
                    double d8 = lerp(d7, dEdge0, dEdge1);
                    map[static_cast<std::size_t>(outIndex++)] += d8 * invAmplitude;
                }
            }
            return;
        }

        int n = 0;
        const double d = 1.0 / amplitude;
        int n11 = -1;
        int n12 = 0;
        int n13 = 0;
        int n14 = 0;
        int n15 = 0;
        int n16 = 0;
        int n17 = 0;
        double d9 = 0.0;
        double d10 = 0.0;
        double d11 = 0.0;
        double d12 = 0.0;
        for (int i = 0; i < width; ++i) {
            double d13 = (x + static_cast<double>(i)) * scaleX + offsetX_;
            int n18 = static_cast<int>(d13);
            if (d13 < static_cast<double>(n18)) {
                --n18;
            }
            int n19 = n18 & 0xFF;
            d13 -= static_cast<double>(n18);
            double d14 = d13 * d13 * d13 * (d13 * (d13 * 6.0 - 15.0) + 10.0);
            for (int j = 0; j < depth; ++j) {
                double d15 = (z + static_cast<double>(j)) * scaleZ + offsetZ_;
                int n20 = static_cast<int>(d15);
                if (d15 < static_cast<double>(n20)) {
                    --n20;
                }
                int n21 = n20 & 0xFF;
                d15 -= static_cast<double>(n20);
                double d16 = d15 * d15 * d15 * (d15 * (d15 * 6.0 - 15.0) + 10.0);
                for (int k = 0; k < height; ++k) {
                    double d17 = (y + static_cast<double>(k)) * scaleY + offsetY_;
                    int n22 = static_cast<int>(d17);
                    if (d17 < static_cast<double>(n22)) {
                        --n22;
                    }
                    int n23 = n22 & 0xFF;
                    d17 -= static_cast<double>(n22);
                    double d18 = d17 * d17 * d17 * (d17 * (d17 * 6.0 - 15.0) + 10.0);
                    if (k == 0 || n23 != n11) {
                        n11 = n23;
                        n12 = perm_[n19] + n23;
                        n13 = perm_[n12] + n21;
                        n14 = perm_[n12 + 1] + n21;
                        n15 = perm_[n19 + 1] + n23;
                        n16 = perm_[n15] + n21;
                        n17 = perm_[n15 + 1] + n21;
                        d9 = lerp(d14, grad(perm_[n13], d13, d17, d15), grad(perm_[n16], d13 - 1.0, d17, d15));
                        d10 = lerp(d14, grad(perm_[n14], d13, d17 - 1.0, d15), grad(perm_[n17], d13 - 1.0, d17 - 1.0, d15));
                        d11 = lerp(d14, grad(perm_[n13 + 1], d13, d17, d15 - 1.0), grad(perm_[n16 + 1], d13 - 1.0, d17, d15 - 1.0));
                        d12 = lerp(d14, grad(perm_[n14 + 1], d13, d17 - 1.0, d15 - 1.0), grad(perm_[n17 + 1], d13 - 1.0, d17 - 1.0, d15 - 1.0));
                    }
                    double d19 = lerp(d18, d9, d10);
                    double d20 = lerp(d18, d11, d12);
                    double d21 = lerp(d16, d19, d20);
                    map[static_cast<std::size_t>(n++)] += d21 * d;
                }
            }
        }
    }

private:
    static double fade(double value)
    {
        return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
    }

    static double lerp(double amount, double from, double to)
    {
        return from + amount * (to - from);
    }

    static double grad(int hash, double x, double y, double z)
    {
        const int h = hash & 0xF;
        const double u = h < 8 ? x : y;
        const double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    // Java's 2-arg gradCoord, used by the height==1 path.
    static double grad2(int hash, double x, double y)
    {
        const int n = hash & 0xF;
        const double d = static_cast<double>(1 - ((n & 8) >> 3)) * x;
        const double d2 = n < 4 ? 0.0 : (n == 12 || n == 14 ? x : y);
        return ((n & 1) == 0 ? d : -d) + ((n & 2) == 0 ? d2 : -d2);
    }

    std::array<int, 512> perm_ {};
    double offsetX_ = 0.0;
    double offsetY_ = 0.0;
    double offsetZ_ = 0.0;
};

} // namespace net::minecraft
