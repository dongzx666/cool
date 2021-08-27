#include "src/cool.h"
#include <unistd.h>

cool::Logger::ptr g_logger = LOG_ROOT();

void test_fiber();
int main(int argc, char* argv[]) {
  LOG_DEBUG(g_logger) << "main start";
  cool::Scheduler sc{3, false, "test"};
  sc.start();
  LOG_DEBUG(g_logger) << "schedule";
  sc.schedule(&test_fiber);
  sc.stop();
  LOG_DEBUG(g_logger) << "main over";
  return 0;
}

void test_fiber() {
  static int s_count = 5;
  LOG_DEBUG(g_logger) << "test in fiber s_count=" << s_count;
  sleep(1);
  if (--s_count >= 0) {
    cool::Scheduler::GetThis()->schedule(&test_fiber, cool::thread_id());
  }
}
