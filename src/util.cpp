#include "util.h"
#include "fiber.h"
#include "log.h"
#include <cstdlib>
#include <execinfo.h>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <vector>

namespace cool {

static Logger::ptr g_logger = LOG_NAME("system");

pid_t thread_id() { return syscall(SYS_gettid); }

uint32_t fiber_id() { return cool::Fiber::GetFiberId(); }

void backtrace(std::vector<std::string> &bt, int size, int skip) {
  void **array = (void **)malloc((sizeof(void *) * size));
  size_t s = ::backtrace(array, size);

  char **strings = backtrace_symbols(array, s);
  if (strings == NULL) {
    LOG_ERROR(g_logger) << "backtrace_symbols error";
    // TODO: free? <28-07-21, fengyu> //
    return;
  }
  for (size_t i = skip; i < s; ++i) {
    bt.push_back(strings[i]);
  }
  free(strings);
  free(array);
}
std::string backtrace_tostring(int size, int skip, const std::string &prefix) {
  std::vector<std::string> bt;
  backtrace(bt, size, skip);
  std::stringstream ss;
  for (size_t i = 0; i < bt.size(); ++i) {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}
uint64_t GetCurrentMS() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}
uint64_t GetCurrentUS() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}
} // namespace cool
