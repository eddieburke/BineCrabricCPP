// nbt_roundtrip_test.cpp — binary parity and round-trip checks for NBT I/O.

#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/nbt/NbtList.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

using net::minecraft::Nbt;
using net::minecraft::NbtCompound;
using net::minecraft::NbtIo;
using net::minecraft::NbtList;

static bool bytesEqual(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b)
{
    return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size()) == 0;
}

static void test_root_compound_golden_bytes()
{
    // Root compound { "foo": 42 } with empty root name (Java NbtIo format).
    static const std::uint8_t golden[] = {
        0x0A, 0x00, 0x00,
        0x03, 0x00, 0x03, 'f', 'o', 'o',
        0x00, 0x00, 0x00, 0x2A,
        0x00,
    };

    NbtCompound compound;
    compound.putInt("foo", 42);
    const std::vector<std::uint8_t> encoded = compound.storage().toBytes();
    if (!bytesEqual(encoded, std::vector<std::uint8_t>(std::begin(golden), std::end(golden)))) {
        std::fprintf(stderr, "FAIL test_root_compound_golden_bytes: encoded bytes mismatch\n");
        std::exit(1);
    }

    const Nbt decoded = Nbt::read(encoded);
    if (!decoded.isCompound() || decoded.getInt("foo") != 42) {
        std::fprintf(stderr, "FAIL test_root_compound_golden_bytes: decode mismatch\n");
        std::exit(1);
    }
}

static void test_byte_array_bulk_roundtrip()
{
    NbtCompound compound;
    std::vector<std::uint8_t> payload(4096);
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i & 0xFFU);
    }
    compound.putByteArray("Blocks", payload);

    const std::vector<std::uint8_t> encoded = compound.storage().toBytes();
    const NbtCompound roundtrip = NbtIo::read(encoded);
    const std::vector<std::uint8_t> decoded = roundtrip.getByteArray("Blocks");
    if (!bytesEqual(payload, decoded)) {
        std::fprintf(stderr, "FAIL test_byte_array_bulk_roundtrip\n");
        std::exit(1);
    }
}

static void test_empty_list_item_type()
{
    NbtCompound compound;
    compound.put("Entities", Nbt::list());

    const std::vector<std::uint8_t> encoded = compound.storage().toBytes();
    const NbtCompound roundtrip = NbtIo::read(encoded);
    const NbtList entities = roundtrip.getList("Entities");
    if (entities.size() != 0) {
        std::fprintf(stderr, "FAIL test_empty_list_item_type: expected empty list\n");
        std::exit(1);
    }

    // Re-encode and confirm the list header still uses TAG_Byte for empty lists.
    const std::vector<std::uint8_t> reencoded = roundtrip.storage().toBytes();
    if (!bytesEqual(encoded, reencoded)) {
        std::fprintf(stderr, "FAIL test_empty_list_item_type: empty list re-encode mismatch\n");
        std::exit(1);
    }
}

static void test_ascii_and_utf8_strings()
{
    {
        NbtCompound compound;
        compound.putString("id", "Creeper");
        const std::vector<std::uint8_t> encoded = compound.storage().toBytes();
        const NbtCompound roundtrip = NbtIo::read(encoded);
        if (roundtrip.getString("id") != "Creeper") {
            std::fprintf(stderr, "FAIL test_ascii_and_utf8_strings: ascii value mismatch\n");
            std::exit(1);
        }
        if (!bytesEqual(encoded, roundtrip.storage().toBytes())) {
            std::fprintf(stderr, "FAIL test_ascii_and_utf8_strings: ascii re-encode mismatch\n");
            std::exit(1);
        }
    }

    {
        NbtCompound compound;
        compound.putString("text", "caf\xc3\xa9"); // UTF-8 non-ASCII (café)
        const NbtCompound roundtrip = NbtIo::read(compound.storage().toBytes());
        if (roundtrip.getString("text") != "caf\xc3\xa9") {
            std::fprintf(stderr, "FAIL test_ascii_and_utf8_strings: utf8 value mismatch\n");
            std::exit(1);
        }
    }
}

static void test_java_put_then_mutate_level()
{
    // Mirrors AlphaChunkStorage.saveChunk: put Level first, then fill via the child handle.
    NbtCompound root;
    NbtCompound level;
    root.put("Level", level);
    level.putInt("xPos", 12);
    level.putInt("zPos", -4);
    level.putByteArray("Blocks", std::vector<std::uint8_t>(32768, 1));
    level.putBoolean("TerrainPopulated", true);

    const NbtCompound loaded = NbtIo::read(root.storage().toBytes());
    const NbtCompound loadedLevel = loaded.getCompound("Level");
    if (!loaded.contains("Level") || !loadedLevel.contains("Blocks") || loadedLevel.getInt("xPos") != 12
        || loadedLevel.getInt("zPos") != -4 || loadedLevel.getByteArray("Blocks").size() != 32768
        || !loadedLevel.getBoolean("TerrainPopulated")) {
        std::fprintf(stderr, "FAIL test_java_put_then_mutate_level\n");
        std::exit(1);
    }
}

static void test_int_array_roundtrip()
{
    NbtCompound compound;
    std::vector<std::int32_t> values = {1, -2, 0x7FFFFFFF, static_cast<std::int32_t>(0x80000000U)};
    compound.put("coords", Nbt(std::move(values)));

    const std::vector<std::uint8_t> encoded = compound.storage().toBytes();
    const NbtCompound roundtrip = NbtIo::read(encoded);
    const Nbt* tag = roundtrip.storage().get("coords");
    if (tag == nullptr || tag->type() != Nbt::Type::IntArray) {
        std::fprintf(stderr, "FAIL test_int_array_roundtrip: missing int array\n");
        std::exit(1);
    }
    const auto& decoded = tag->asIntArray();
    if (decoded.size() != 4 || decoded[0] != 1 || decoded[1] != -2 || decoded[2] != 0x7FFFFFFF
        || decoded[3] != static_cast<std::int32_t>(0x80000000U)) {
        std::fprintf(stderr, "FAIL test_int_array_roundtrip: value mismatch\n");
        std::exit(1);
    }
}

int main()
{
    test_root_compound_golden_bytes();
    test_byte_array_bulk_roundtrip();
    test_empty_list_item_type();
    test_java_put_then_mutate_level();
    test_ascii_and_utf8_strings();
    test_int_array_roundtrip();
    std::fprintf(stdout, "All NBT roundtrip tests passed.\n");
    return 0;
}
