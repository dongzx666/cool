#include "src/config.h"
#include "src/log.h"
#include "src/thread.h"
#include "yaml.h"
#include <string>
#include <unistd.h>

#define POOL_SIZE 5

cool::Logger::ptr g_logger = LOG_ROOT();

int count = 0;
// cool::RWMutex s_mutex;
cool::Mutex s_mutex;

void func1() {
  LOG_INFO(g_logger) << "name=" << cool::Thread::GetName()
                     << ",this.name=" << cool::Thread::GetThis()->name()
                     << ",id=" << cool::thread_id()
                     << ",this.id=" << cool::Thread::GetThis()->id();
  // sleep(20);
  for (int i = 0; i < 100000; ++i) {
    // cool::RWMutex::WriteLock lock(s_mutex);
    cool::Mutex::Lock lock(s_mutex);
    ++count;
  }
}

void func2() {
  while (true) {
    LOG_INFO(g_logger) << "************************************";
  }
}

void func3() {
  while (true) {
    LOG_INFO(g_logger) << "====================================";
  }
}

int main(int argc, char *argv[]) {
  LOG_INFO(g_logger) << "thread test begin";

  YAML::Node root = YAML::LoadFile("../config/log2.yml");
  cool::Config::load_from_yaml(root);

  std::vector<cool::Thread::ptr> thrs;
  for (int i = 0; i < 2; ++i) {
    cool::Thread::ptr thr1(
        new cool::Thread(&func2, "name_" + std::to_string(i * 2)));
    cool::Thread::ptr thr2(
        new cool::Thread(&func3, "name_" + std::to_string(i * 2 + 1)));
    thrs.push_back(thr1);
    thrs.push_back(thr2);
  }
  for (size_t i = 0; i < thrs.size(); ++i) {
    thrs[i]->join();
  }
  LOG_INFO(g_logger) << "thread test end";
  LOG_INFO(g_logger) << "count: " << count;

  return 0;
}
