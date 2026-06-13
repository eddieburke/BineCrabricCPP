// generator_rng_parity_test.cpp — parity for Generator coordinator RNG (cave carver seeds).
#include "net/minecraft/util/math/Types.hpp"

#include <cstdio>
#include <cstdlib>

using net::minecraft::JavaRandom;

static int failures = 0;

static void check(bool ok, const char* message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

static void probeWorldSeed(std::int64_t worldSeed)
{
    JavaRandom random(static_cast<std::uint64_t>(worldSeed));
    random.setSeed(static_cast<std::uint64_t>(worldSeed));
    const std::int64_t l = random.nextLong() / 2LL * 2LL + 1LL;
    const std::int64_t l2 = random.nextLong() / 2LL * 2LL + 1LL;

    std::printf("worldSeed=%lld\n", static_cast<long long>(worldSeed));
    std::printf("l=%lld\n", static_cast<long long>(l));
    std::printf("l2=%lld\n", static_cast<long long>(l2));

    constexpr int chunkX = 0;
    constexpr int chunkZ = 0;
    constexpr int range = 8;
    int calls = 0;
    int nonZero = 0;
    for (int i = chunkX - range; i <= chunkX + range; ++i) {
        for (int j = chunkZ - range; j <= chunkZ + range; ++j) {
            const std::uint64_t cellSeed =
                (static_cast<std::uint64_t>(static_cast<std::int64_t>(i)) * static_cast<std::uint64_t>(l)
                    + static_cast<std::uint64_t>(static_cast<std::int64_t>(j)) * static_cast<std::uint64_t>(l2))
                ^ static_cast<std::uint64_t>(worldSeed);
            random.setSeed(cellSeed);
            int caveCount = random.nextInt(random.nextInt(random.nextInt(40) + 1) + 1);
            if (random.nextInt(15) != 0) {
                caveCount = 0;
            }
            ++calls;
            if (caveCount > 0) {
                ++nonZero;
                if (i == 0 && j == 0) {
                    std::printf("caveCount at start (0,0)=%d\n", caveCount);
                }
            }
        }
    }
    std::printf("cave_calls=%d nonZero=%d\n", calls, nonZero);
}

int main(int argc, char** argv)
{
    const std::int64_t seed = argc > 1 ? static_cast<std::int64_t>(std::stoll(argv[1])) : 12345LL;
    probeWorldSeed(seed);

    if (seed == 12345LL) {
        // Values captured from tools/GeneratorRngProbe.java with seed 12345.
        JavaRandom random(12345ULL);
        random.setSeed(12345ULL);
        const std::int64_t l = random.nextLong() / 2LL * 2LL + 1LL;
        const std::int64_t l2 = random.nextLong() / 2LL * 2LL + 1LL;
        check(l == 6674089274190705457LL, "l mismatch for seed 12345");
        check(l2 == -1236052134575208583LL, "l2 mismatch for seed 12345");
    }

    if (failures == 0) {
        std::printf("generator_rng_parity_test: passed\n");
        return 0;
    }
    std::fprintf(stderr, "generator_rng_parity_test: %d failure(s)\n", failures);
    return 1;
}
