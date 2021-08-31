#ifndef __COOL_UTIL_H
#define __COOL_UTIL_H
#include <cstdint>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace cool {
pid_t thread_id();
uint32_t fiber_id();
void backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
std::string backtrace_tostring(int size = 64, int skip = 2, const std::string& prefix = "");
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();
} // namespace cool

#endif /* ifndef __COOL_UTIL_H */
