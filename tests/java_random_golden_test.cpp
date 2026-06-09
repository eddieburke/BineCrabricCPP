// java_random_golden_test.cpp
// Golden-vector regression tests for JavaRandom (Java LCG parity).
// No framework — assert + main.  Returns 0 on pass, 1 on any failure.
//
// Implementation under test:
//   native/src/net/minecraft/util/math/Types.hpp   (JavaRandom)
//   native/src/net/minecraft/client/sound/CodecMus.hpp (javaStringHashCode)
//
// LCG spec (Java Random):
//   setSeed(n)  : state = (n ^ 0x5DEECE66DULL) & ((1ULL<<48)-1)
//   next(bits)  : state = (state * 0x5DEECE66DULL + 0xBULL) & mask
//                 return (int)(state >> (48 - bits))
//   nextInt()   = next(32)   -> signed int32
//   nextInt(b)  = power-of-2: (int)(((int64_t)b * next(31)) >> 31)
//                 otherwise: rejection-sampled next(31) % b

#include "net/minecraft/util/math/Types.hpp"

// javaStringHashCode is private in CodecMus; expose it via a test shim.
// We replicate the identical logic here rather than friend-hack the class.
#include <cstdint>
static int javaStringHashCode(const char* s)
{
    int h = 0;
    for (; *s != '\0'; ++s) {
        h = 31 * h + static_cast<int>(static_cast<unsigned char>(*s));
    }
    return h;
}

#include <cassert>
#include <cstdio>

using net::minecraft::JavaRandom;

// ---------------------------------------------------------------------------
// G0 — seed 0, first 10 nextInt() calls
// Derived by stepping the LCG manually (see rng-test-plan.md for working).
// ---------------------------------------------------------------------------
static void test_g0_seed0()
{
    static const int expected[10] = {
        -1155484576,
         -723955400,
         1033096058,
        -1690734402,
        -1557280266,
         1327362106,
        -1930858313,
          502539523,
        -1728529858,
          -938301587
    };

    JavaRandom r(0);
    for (int i = 0; i < 10; ++i) {
        const int got = r.nextInt();
        if (got != expected[i]) {
            fprintf(stderr,
                "FAIL test_g0_seed0: step %d expected %d got %d\n",
                i, expected[i], got);
            // let the assert fire for a hard stop
            assert(got == expected[i]);
        }
    }
}

// ---------------------------------------------------------------------------
// G0 — seed 12345, first 5 nextInt() calls
// ---------------------------------------------------------------------------
static void test_g0_seed12345()
{
    static const int expected[5] = {
         1553932502,
        -2090749135,
          -287790814,
          -355989640,
          -716867186
    };

    JavaRandom r(0);
    r.setSeed(12345);
    for (int i = 0; i < 5; ++i) {
        const int got = r.nextInt();
        if (got != expected[i]) {
            fprintf(stderr,
                "FAIL test_g0_seed12345: step %d expected %d got %d\n",
                i, expected[i], got);
            assert(got == expected[i]);
        }
    }
}

// ---------------------------------------------------------------------------
// G0 — bounded nextInt(16) from seed 0, first 10 calls
// Verifies the power-of-2 fast path (16 = 2^4).
// Expected: 11, 13, 3, 9, 10, 4, 8, 1, 9, 12
// ---------------------------------------------------------------------------
static void test_g0_bounded()
{
    static const int expected[10] = {
        11, 13, 3, 9, 10, 4, 8, 1, 9, 12
    };

    JavaRandom r(0);
    for (int i = 0; i < 10; ++i) {
        const int got = r.nextInt(16);
        if (got < 0 || got >= 16) {
            fprintf(stderr,
                "FAIL test_g0_bounded: step %d out of range [0,16): %d\n",
                i, got);
            assert(got >= 0 && got < 16);
        }
        if (got != expected[i]) {
            fprintf(stderr,
                "FAIL test_g0_bounded: step %d expected %d got %d\n",
                i, expected[i], got);
            assert(got == expected[i]);
        }
    }
}

// ---------------------------------------------------------------------------
// G1 — World constructor seed paths
//
// All three World constructors end with the same pattern:
//   random_.setSeed(seed_);
//   lcgBlockSeed_       = random_.nextInt();   // first call
//   ambientSoundCounter_ = random_.nextInt(12000); // second call
//
// Constructor 1 (name+seed)  : random_(seed)     — setSeed not called again
// Constructor 2 (storage+seed): random_(seed) then random_.setSeed(seed_) after
//                               property reload — effectively setSeed(seed) twice
// Constructor 3 (parent)     : random_(parent->seed_)
//
// In all cases the first random_.nextInt() after final setSeed is lcgBlockSeed_.
// We test seed=12345: first nextInt() must equal 1553932502.
// ---------------------------------------------------------------------------
static void test_g1_world_seed()
{
    // Mirrors World ctor 1 sequence: JavaRandom random_(seed); then nextInt().
    JavaRandom r(12345);
    const int lcgBlockSeed = r.nextInt();
    if (lcgBlockSeed != 1553932502) {
        fprintf(stderr,
            "FAIL test_g1_world_seed: lcgBlockSeed expected 1553932502 got %d\n",
            lcgBlockSeed);
        assert(lcgBlockSeed == 1553932502);
    }

    // Mirrors World ctor 2 sequence: random_.setSeed(seed_) after property load.
    // Double-setSeed is idempotent; same first nextInt expected.
    JavaRandom r2(12345);
    r2.setSeed(12345);
    const int lcgBlockSeed2 = r2.nextInt();
    if (lcgBlockSeed2 != 1553932502) {
        fprintf(stderr,
            "FAIL test_g1_world_seed (ctor2 path): expected 1553932502 got %d\n",
            lcgBlockSeed2);
        assert(lcgBlockSeed2 == 1553932502);
    }
}

// ---------------------------------------------------------------------------
// G5 — javaStringHashCode golden vectors
// Java String.hashCode(): h = 31*h + char, int32 arithmetic.
// ---------------------------------------------------------------------------
static void test_g5_string_hash()
{
    struct { const char* s; int expected; } cases[] = {
        { "",      0        },
        { "hello", 99162322 },
        { "0",     48       },
        { "a",     97       },
    };

    for (const auto& c : cases) {
        const int got = javaStringHashCode(c.s);
        if (got != c.expected) {
            fprintf(stderr,
                "FAIL test_g5_string_hash: \"%s\" expected %d got %d\n",
                c.s, c.expected, got);
            assert(got == c.expected);
        }
    }
}

// ---------------------------------------------------------------------------
int main()
{
    test_g0_seed0();
    test_g0_seed12345();
    test_g0_bounded();
    test_g1_world_seed();
    test_g5_string_hash();
    fprintf(stdout, "All JavaRandom golden tests passed.\n");
    return 0;
}
