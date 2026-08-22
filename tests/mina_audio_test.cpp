#include <array>
#include <cstdlib>

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
