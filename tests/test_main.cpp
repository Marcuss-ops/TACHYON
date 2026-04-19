#include <gtest/gtest.h>
#include "tachyon/version.h"

TEST(TachyonCoreTests, VersionCheck) {
    EXPECT_STREQ(TACHYON_VERSION_STR, "0.1.0");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
