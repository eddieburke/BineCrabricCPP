#pragma once
#include <iostream>
#include <stdexcept>
#include <string>
namespace server_test {
inline int& failureCount() {
  static int failures = 0;
  return failures;
}
class TestFailure : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};
} // namespace server_test
#define EXPECT_TRUE(expr)                                                                     \
  do {                                                                                        \
    if(!(expr)) {                                                                             \
      throw server_test::TestFailure(std::string("EXPECT_TRUE failed at ") + __FILE__ + ":" + \
                                     std::to_string(__LINE__));                               \
    }                                                                                         \
  } while(false)
#define EXPECT_EQ(a, b) EXPECT_TRUE((a) == (b))
#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))
#define RUN_SERVER_TEST(name)                                    \
  do {                                                           \
    try {                                                        \
      name();                                                    \
      std::cout << "[PASS] " #name "\n";                         \
    } catch(const std::exception& error) {                       \
      ++server_test::failureCount();                             \
      std::cout << "[FAIL] " #name ": " << error.what() << '\n'; \
    }                                                            \
  } while(false)
