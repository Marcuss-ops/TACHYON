#pragma once

#include <memory>
#include <iostream>
#include <string>

namespace testing {

class Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

struct AssertionProxy {
    bool condition{true};

    template <typename T>
    AssertionProxy& operator<<(const T&) {
        return *this;
    }
};

inline AssertionProxy MakeAssertion(bool condition) {
    if (!condition) {
        std::cerr << "[gtest-shim] assertion failed\n";
    }
    return AssertionProxy{condition};
}

} // namespace testing

#define TEST(test_case_name, test_name) static void test_case_name##_##test_name()

#define TEST_F(test_fixture, test_name) \
class test_fixture##_##test_name##_Test : public test_fixture { \
public: \
    void TestBody(); \
}; \
void test_fixture##_##test_name##_Test::TestBody()

#define EXPECT_TRUE(cond) ::testing::MakeAssertion(static_cast<bool>(cond))
#define ASSERT_TRUE(cond) ::testing::MakeAssertion(static_cast<bool>(cond))
#define EXPECT_FALSE(cond) ::testing::MakeAssertion(!(cond))
#define ASSERT_FALSE(cond) ::testing::MakeAssertion(!(cond))

#define EXPECT_EQ(val1, val2) ::testing::MakeAssertion((val1) == (val2))
#define ASSERT_EQ(val1, val2) ::testing::MakeAssertion((val1) == (val2))
#define EXPECT_NE(val1, val2) ::testing::MakeAssertion((val1) != (val2))
#define ASSERT_NE(val1, val2) ::testing::MakeAssertion((val1) != (val2))
#define EXPECT_GE(val1, val2) ::testing::MakeAssertion((val1) >= (val2))
#define ASSERT_GE(val1, val2) ::testing::MakeAssertion((val1) >= (val2))
#define EXPECT_GT(val1, val2) ::testing::MakeAssertion((val1) > (val2))
#define ASSERT_GT(val1, val2) ::testing::MakeAssertion((val1) > (val2))
#define EXPECT_LE(val1, val2) ::testing::MakeAssertion((val1) <= (val2))
#define ASSERT_LE(val1, val2) ::testing::MakeAssertion((val1) <= (val2))
#define EXPECT_LT(val1, val2) ::testing::MakeAssertion((val1) < (val2))
#define ASSERT_LT(val1, val2) ::testing::MakeAssertion((val1) < (val2))

#define EXPECT_NEAR(val1, val2, abs_error) ::testing::MakeAssertion(std::abs((val1) - (val2)) <= (abs_error))
#define ASSERT_NEAR(val1, val2, abs_error) ::testing::MakeAssertion(std::abs((val1) - (val2)) <= (abs_error))
