#include <array>
#include <cstdlib>
#include <vector>

#include <gtest/gtest.h>

#include "mina.h"

TEST(MinaAudioTest, DecodesAndResamplesWav) {
 const std::array<mina_u8, 48> wav = {
     'R', 'I', 'F', 'F', 40, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
     16, 0, 0, 0, 1, 0, 1, 0, 34, 86, 0, 0, 68, 172, 0, 0,
     2, 0, 16, 0, 'd', 'a', 't', 'a', 4, 0, 0, 0, 0, 0, 255, 127};
 mina_pcm pcm{};
 ASSERT_EQ(mina_decode(wav.data(), wav.size(), &pcm), MINA_OK);
 ASSERT_EQ(pcm.sample_rate, 22050u);
 ASSERT_EQ(pcm.channels, 1u);
 ASSERT_EQ(mina_u64_to_size(pcm.frames), 2u);

 float* resampled = nullptr;
 const mina_u64 outputFrames =
     mina_resample_buffer(pcm.samples, pcm.frames, pcm.sample_rate, 44100, pcm.channels, MINA_QUALITY_SINC, &resampled);
 ASSERT_NE(resampled, nullptr);
 EXPECT_GT(mina_u64_to_size(outputFrames), 2u);
 std::free(resampled);
 mina_pcm_free(&pcm);
}

TEST(MinaAudioTest, SeeksStreamsAndLoopsRequestedRegion) {
 const std::array<mina_u8, 52> wav = {
     'R', 'I', 'F', 'F', 44, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
     16, 0, 0, 0, 1, 0, 1, 0, 68, 172, 0, 0, 136, 88, 1, 0,
     2, 0, 16, 0, 'd', 'a', 't', 'a', 8, 0, 0, 0, 0, 0, 1, 0, 2, 0, 3, 0};
 mina_stream* stream = mina_stream_open(wav.data(), wav.size(), 0);
 ASSERT_NE(stream, nullptr);
 ASSERT_TRUE(mina_stream_seek(stream, static_cast<mina_u64>(2)));
 std::array<float, 2> read{};
 ASSERT_EQ(mina_stream_read(stream, read.data(), 2), 2u);
 EXPECT_LT(read[0], read[1]);
 mina_stream_close(stream);

 mina_engine* engine = mina_engine_create("", 44100, 1, 32, 2);
 ASSERT_NE(engine, nullptr);
 mina_source_params params;
 mina_source_params_init(&params);
 params.loop = 1;
 params.loop_start = static_cast<mina_u64>(1);
 params.loop_end = static_cast<mina_u64>(3);
 ASSERT_TRUE(mina_engine_play_memory(engine, "loop", wav.data(), wav.size(), &params, 1.0f, 1.0f));
 std::array<float, 6> mixed{};
 ASSERT_EQ(mina_engine_render(engine, mixed.data(), static_cast<mina_u32>(mixed.size())), mixed.size());
 EXPECT_FLOAT_EQ(mixed[0], 0.0f);
 EXPECT_LT(mixed[1], mixed[2]);
 EXPECT_LT(mixed[2], mixed[3]);
 EXPECT_FLOAT_EQ(mixed[1], mixed[3]);
 mina_engine_destroy(engine);
}

TEST(MinaAudioTest, ReportsOutputAndEngineTelemetry) {
 const mina_u32 deviceCount = mina_device_enumerate(nullptr, 0);
 EXPECT_GT(deviceCount, 0u);
 std::vector<mina_output_device> devices(deviceCount);
 EXPECT_EQ(mina_device_enumerate(devices.data(), deviceCount), deviceCount);
 mina_engine* engine = mina_engine_create("", 44100, 2, 32, 1);
 ASSERT_NE(engine, nullptr);
 mina_engine_telemetry telemetry{};
 mina_engine_get_telemetry(engine, &telemetry);
 EXPECT_EQ(telemetry.active_voices, 0u);
 mina_engine_destroy(engine);
}
