#include "src/cool.h"
#include "src/log.h"
#include "src/thread.h"
#include <string>
#include <iostream>

cool::Logger::ptr g_logger = LOG_ROOT();

void run_in_fiber() {
  LOG_INFO(g_logger) << "run in fiber begin";
  // cool::Fiber::GetThis()->swapOut();
  cool::Fiber::YieldToHold();
  LOG_INFO(g_logger) << "run in fiber end";
  cool::Fiber::YieldToHold();
  LOG_INFO(g_logger) << "run in fiber end 2";
}

void test_fiber() {
  LOG_INFO(g_logger) << "test_fiber func begin";
  {
    cool::Fiber::GetThis();
    cool::Fiber::ptr fiber(new cool::Fiber(run_in_fiber));
    LOG_INFO(g_logger) << "create fiber success";
    fiber->swapIn();
    LOG_INFO(g_logger) << "test_fiber func after swapIn 1";
    fiber->swapIn();
    LOG_INFO(g_logger) << "test_fiber func after swapIn 2";
    fiber->swapIn();
    LOG_INFO(g_logger) << "test_fiber func after swapIn 3";
  }
  LOG_INFO(g_logger) << "test_fiber func end";
}

int main(int argc, char *argv[]) {
  cool::Thread::SetName("main");
  std::vector<cool::Thread::ptr> thrs;
  for (int i = 0; i < 3; ++i) {
    thrs.push_back(cool::Thread::ptr(new cool::Thread(&test_fiber, "name_"+std::to_string(i))));
  }
  for (auto i : thrs) {
    i->join();
  }
  return 0;
}
