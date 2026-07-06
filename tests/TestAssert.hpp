#pragma once
#include <cstdlib>
#include <iostream>
struct TestContext {
  int failures = 0;
  void expect(bool condition, const char* message) {
    if(!condition) {
      ++failures;
      std::cerr << "FAIL: " << message << '\n';
    }
  }
};
#define EXPECT_TRUE(ctx, condition, message) (ctx).expect((condition), (message))
#define EXPECT_EQ(ctx, left, right, message) (ctx).expect((left) == (right), (message))
#define RUN_TEST(ctx, name)             \
  do {                                  \
    const int before = (ctx).failures;  \
    name(ctx);                          \
    if((ctx).failures == before) {      \
      std::cout << "PASS: " #name "\n"; \
    }                                   \
  } while(false)
