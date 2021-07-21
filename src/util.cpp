#include "util.h"

namespace cool {
  pid_t thread_id() {
   return syscall(SYS_gettid);
  }

  uint32_t fiber_id() {
    return 0;
  }
  
}
