#ifndef __COOL_UTIL_H
#define __COOL_UTIL_H
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

namespace cool {

  pid_t thread_id();
  uint32_t fiber_id();
}


#endif /* ifndef __COOL_UTIL_H */
