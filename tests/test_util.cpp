#include "src/cool.h"
#include "src/util.h"
#include "src/macro.h"
#include <assert.h>

cool::Logger::ptr g_logger = LOG_ROOT();

void test_assert () {
  // assert(0);
  LOG_INFO(g_logger) << cool::backtrace_tostring(10, 2, "  ");
  // ASSERT(false);
  ASSERT2(0 == 1, "test assert2");
}

int main(int argc, char* argv[]) {
  test_assert();
  return 0;
}
