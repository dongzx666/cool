#ifndef __COOL_MACRO_H
#define __COOL_MACRO_H

#include "log.h"
#include "util.h"
#include <assert.h>
#include <string>

#if defined __GGNUC__ || defined __llvm__
#define LIKEY(x) __builtin_expect(!!(x), 1)
#define UNLIKEY(x) __builtin_expect(!!(x), 0)
#else
#define LIKEY(x) (x)
#define UNLIKEY(x) (x)
#endif

#define ASSERT(x)                                                    \
  if (UNLIKEY(!(x))) {                                               \
    LOG_ERROR(LOG_ROOT()) << "ASSERTION: " << #x << "\nbacktrace:\n" \
                          << cool::backtrace_tostring(100, 2, "  "); \
    assert(x);                                                       \
  }

#define ASSERT2(x, w)                                                \
  if (UNLIKEY(!(x))) {                                               \
    LOG_ERROR(LOG_ROOT()) << "ASSERTION: " << #x << "\n"             \
                          << w << "\nbacktrace:\n"                   \
                          << cool::backtrace_tostring(100, 2, "  "); \
    assert(x);                                                       \
  }

#endif /* ifndef __COOL_MACRO_H */
