#ifndef __COOL_MACRO_H
#define __COOL_MACRO_H

#include "log.h"
#include "util.h"
#include <assert.h>
#include <string>

#define ASSERT(x)                                                              \
  if (!(x)) {                                                                  \
    LOG_ERROR(LOG_ROOT()) << "ASSERTION: " << #x << "\nbacktrace:\n"           \
                          << cool::backtrace_tostring(100, 2, "  ");                 \
    assert(x);                                                                 \
  }

#define ASSERT2(x, w)                                                          \
  if (!(x)) {                                                                  \
    LOG_ERROR(LOG_ROOT()) << "ASSERTION: " << #x << "\n"                       \
                          << w << "\nbacktrace:\n"                             \
                          << cool::backtrace_tostring(100, 2, "  ");                 \
    assert(x);                                                                 \
  }

#endif /* ifndef __COOL_MACRO_H */
