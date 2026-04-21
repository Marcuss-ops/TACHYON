#include "gtest/gtest.h"
#include "tachyon/runtime/core/math_contract.h"
#include "tachyon/runtime/execution/frame_hasher.h"
#include <vector>

namespace tachyon {

TEST(DeterminismTests, SplitMix64BitIdentity) {
    // Known values for SplitMix64
    EXPECT_EQ(math_contract::splitmix64(0), 0x981b1ed92040089cULL);
    EXPECT_EQ(math_contract::splitmix64(12345), 0xce54a57f8976b3f9ULL);
}

TEST(DeterminismTests, SpringClosedFormStateless) {
    const double from = 0.0;
    const double to = 100.0;
    const double freq = 5.0;
    const double damping = 0.5;

    // Value at t=0 must be exactly 'from'
    EXPECT_DOUBLE_EQ(math_contract::spring(0.0, from, to, freq, damping), from);
    
    // Value at t=Large must be near 'to'
    EXPECT_NEAR(math_contract::spring(10.0, from, to, freq, damping), to, 0.001);
}

TEST(DeterminismTests, FrameHashingConsistency) {
    std::vector<std::byte> pixels(1024, std::byte{0xAA});
    FrameHasher hasher;
    
    uint64_t h1 = hasher.hash_pixels(pixels);
    uint64_t h2 = hasher.hash_pixels(pixels);
    
    EXPECT_EQ(h1, h2);
    
    pixels[512] = std::byte{0xBB};
    uint64_t h3 = hasher.hash_pixels(pixels);
    EXPECT_NE(h1, h3);
}

} // namespace tachyon
