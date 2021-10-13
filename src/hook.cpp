#include "hook.h"
#include "fd_manager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include "scheduler.h"
#include "config.h"
#include "timer.h"
#include <cerrno>
#include <cstdarg>
#include <dlfcn.h>
#include <fcntl.h>
#include <functional>
#include <memory>
// #include <sys/socket.h>
#include <utility>

static cool::Logger::ptr g_logger = LOG_NAME("system");
static cool::ConfigVar<int>::ptr g_tcp_connect_timeout =
    cool::Config::lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

namespace cool {

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX)                                                           \
  XX(sleep)                                                                    \
  XX(usleep)                                                                   \
  XX(nanosleep)                                                                \
  XX(socket)                                                                   \
  XX(connect)                                                                  \
  XX(accept)                                                                   \
  XX(read)                                                                     \
  XX(readv)                                                                    \
  XX(recv)                                                                     \
  XX(recvfrom)                                                                 \
  XX(recvmsg)                                                                  \
  XX(write)                                                                    \
  XX(writev)                                                                   \
  XX(send)                                                                     \
  XX(sendto)                                                                   \
  XX(sendmsg)                                                                  \
  XX(close)                                                                    \
  XX(fcntl)                                                                    \
  XX(ioctl)                                                                    \
  XX(getsockopt)                                                               \
  XX(setsockopt)

void hook_init() {
  static bool is_inited = false;
  if (is_inited) {
    return;
  }
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
  _HookIniter() {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->get_value();
    g_tcp_connect_timeout->add_listener(
        [](const int &old_val, const int &new_val) {
          LOG_DEBUG(g_logger) << "tcp connect timeout changed from " << old_val
                              << " to " << new_val;
          s_connect_timeout = new_val;
        });
  }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }

void set_hook_enable(bool flag) { t_hook_enable = flag; }
} // namespace cool

struct timer_info {
  int cancelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args) {
  if (!cool::t_hook_enable) {
    return fun(fd, std::forward<Args>(args)...);
  }
  cool::FdCtx::ptr ctx = cool::FdMgr::instance()->get(fd);
  if (!ctx) {
    return fun(fd, std::forward<Args>(args)...);
  }
  if (ctx->isClose()) {
    errno = EBADF;
    return -1;
  }
  if (!ctx->isSocket() || ctx->getUserNonBlock()) {
    return fun(fd, std::forward<Args>(args)...);
  }
  uint64_t to = ctx->getTimeout(timeout_so);
  std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  while (n == -1 && errno == EINTR) {
    n = fun(fd, std::forward<Args>(args)...);
  }
  if (n == -1 && errno == EAGAIN) {
    cool::IOManager *iom = cool::IOManager::GetThis();
    cool::Timer::ptr timer;
    std::weak_ptr<timer_info> winfo(tinfo);

    if (to != (uint64_t)-1) {
      timer = iom->addConditionTimer(
          to,
          [winfo, fd, iom, event]() {
            auto t = winfo.lock();
            if (!t || t->cancelled) {
              return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, (cool::IOManager::Event)event);
          },
          winfo);
    }

    int rt = iom->addEvent(fd, (cool::IOManager::Event)event);
    if (rt) {
      LOG_ERROR(g_logger)
          << hook_fun_name << " addevent(" << fd << ", " << event << ")";
      if (timer) {
        timer->cancel();
      }
      return -1;
    } else {
      // LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
      cool::Fiber::YieldToHold();
      // LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
      if (timer) {
        timer->cancel();
      }
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }
      goto retry;
    }
  }
  return n;
}

extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!cool::t_hook_enable) {
    return sleep_f(seconds);
  }
  cool::Fiber::ptr fiber = cool::Fiber::GetThis();
  cool::IOManager *iom = cool::IOManager::GetThis();
  // iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber); });
  iom->addTimer(
      seconds * 1000,
      std::bind((void (cool::Scheduler::*)(cool::Fiber::ptr, int thread)) &
                    cool::IOManager::schedule,
                iom, fiber, -1));
  cool::Fiber::YieldToHold();
  return 0;
}
int usleep(useconds_t usec) {
  if (!cool::t_hook_enable) {
    return usleep_f(usec);
  }
  cool::Fiber::ptr fiber = cool::Fiber::GetThis();
  cool::IOManager *iom = cool::IOManager::GetThis();
  // iom->addTimer(usec / 1000, [iom, fiber]() { iom->schedule(fiber); });
  iom->addTimer(
      usec / 1000,
      std::bind((void (cool::Scheduler::*)(cool::Fiber::ptr, int thread)) &
                    cool::IOManager::schedule,
                iom, fiber, -1));
  cool::Fiber::YieldToHold();
  return 0;
}
int nanosleep(const struct timespec *req, struct timespec *rem) {
  if (!cool::t_hook_enable) {
    return nanosleep(req, rem);
  }
  int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
  cool::Fiber::ptr fiber = cool::Fiber::GetThis();
  cool::IOManager *iom = cool::IOManager::GetThis();
  // iom->addTimer(timeout_ms, [iom, fiber]() { iom->schedule(fiber); });
  iom->addTimer(
      timeout_ms,
      std::bind((void (cool::Scheduler::*)(cool::Fiber::ptr, int thread)) &
                    cool::IOManager::schedule,
                iom, fiber, -1));
  cool::Fiber::YieldToHold();
  return 0;
}

int socket(int domain, int type, int protocol) {
  if (!cool::t_hook_enable) {
    return socket_f(domain, type, protocol);
  }
  int fd = socket_f(domain, type, protocol);
  if (fd == -1) {
    return fd;
  }
  cool::FdMgr::instance()->get(fd, true);
  return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
  if (!cool::t_hook_enable) {
    return connect_f(sockfd, addr, addrlen);
  }
  cool::FdCtx::ptr ctx = cool::FdMgr::instance()->get(sockfd);
  if (!ctx || ctx->isClose()) {
    errno = EBADF;
    return -1;
  }
  if (!ctx->isSocket()) {
    return connect_f(sockfd, addr, addrlen);
  }
  int n = connect_f(sockfd, addr, addrlen);
  if (n == 0) {
    return 0;
  } else if (n != -1 || errno != EINPROGRESS) {
    return n;
  }
  cool::IOManager* iom = cool::IOManager::GetThis();
  cool::Timer::ptr timer;
  std::shared_ptr<timer_info> tinfo(new timer_info);
  std::weak_ptr<timer_info> winfo(tinfo);

  if (timeout_ms != (uint64_t)-1) {
    timer = iom->addConditionTimer(
        timeout_ms,
        [winfo, sockfd, iom]() {
          auto t = winfo.lock();
          if (!t || t->cancelled) {
            return;
          }
          t->cancelled = ETIMEDOUT;
          iom->cancelEvent(sockfd, cool::IOManager::WRITE);
        },
        winfo);
  }
  int rt = iom->addEvent(sockfd, cool::IOManager::WRITE);
  if (rt == 0) {
    cool::Fiber::YieldToHold();
    if (timer) {
      timer->cancel();
    }
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  } else {
    if (timer) {
      timer->cancel();
    }
    LOG_ERROR(g_logger) << "connect addEvent (" << sockfd <<", WRITE) error";
  }
  int error = 0;
  socklen_t len = sizeof(int);
  if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  if (!error) {
    return 0;
  } else {
    errno = error;
    return -1;
  }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return connect_with_timeout(sockfd, addr, addrlen, cool::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int fd = do_io(sockfd, accept_f, "accept", cool::IOManager::READ, SO_RCVTIMEO,
                 addr, addrlen);
  if (fd >= 0) {
    cool::FdMgr::instance()->get(fd, true);
  }
  return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
  return do_io(fd, read_f, "read", cool::IOManager::READ, SO_RCVTIMEO, buf,
               count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, readv_f, "readv", cool::IOManager::READ, SO_RCVTIMEO, iov,
               iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return do_io(sockfd, recv_f, "recv", cool::IOManager::READ, SO_RCVTIMEO, buf,
               len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {

  return do_io(sockfd, recvfrom_f, "recvfrom", cool::IOManager::READ,
               SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return do_io(sockfd, recvmsg_f, "recvmsg", cool::IOManager::READ, SO_RCVTIMEO,
               msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return do_io(fd, write_f, "write", cool::IOManager::WRITE, SO_SNDTIMEO, buf,
               count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, writev_f, "writev", cool::IOManager::WRITE, SO_SNDTIMEO, iov,
               iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  return do_io(sockfd, send_f, "send", cool::IOManager::WRITE, SO_SNDTIMEO, buf,
               len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  return do_io(sockfd, sendto_f, "sendto", cool::IOManager::WRITE, SO_SNDTIMEO,
               buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  return do_io(sockfd, sendmsg_f, "sendmsg", cool::IOManager::WRITE,
               SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
  if (!cool::t_hook_enable) {
    return close_f(fd);
  }
  cool::FdCtx::ptr ctx = cool::FdMgr::instance()->get(fd);
  if (ctx) {
    auto iom = cool::IOManager::GetThis();
    if (iom) {
      iom->cancekAll(fd);
    }
    cool::FdMgr::instance()->del(fd);
  }
  return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */) {
  va_list va;
  va_start(va, cmd);
  switch (cmd) {
  case F_SETFL:
    {
      int arg = va_arg(va, int);
      va_end(va);
      cool::FdCtx::ptr ctx = cool::FdMgr::instance()->get(fd);
      if (!ctx || ctx->isClose()) {
        return fcntl_f(fd, cmd, arg);
      }
      ctx->setUserNonBlock(arg & O_NONBLOCK);
      if (ctx->getSysNonBlock()) {
        arg |= O_NONBLOCK;
      } else {
        arg &= ~O_NONBLOCK;
      }
      return fcntl_f(fd, cmd, arg);
    }
    break;
  case F_GETFL:
    {
      va_end(va);
      int arg = fcntl_f(fd, cmd);
      cool::FdCtx::ptr ctx = cool::FdMgr::instance()->get(fd);
      if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return arg;
      }
      if (ctx->getUserNonBlock()) {
        return arg | O_NONBLOCK;
      } else {
        return arg & ~O_NONBLOCK;
      }
    }
    break;
  case F_DUPFD:
  case F_DUPFD_CLOEXEC:
  case F_SETFD:
  case F_SETOWN:
  case F_SETSIG:
  case F_SETLEASE:
  case F_NOTIFY:
  case F_SETPIPE_SZ:
    {
      int arg = va_arg(va, int);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    }
    break;
  case F_GETFD:
  case F_GETOWN:
  case F_GETSIG:
  case F_GETLEASE:
  case F_GETPIPE_SZ:
    {
      va_end(va);
      return fcntl_f(fd, cmd);
    }
    break;
  case F_SETLK:
  case F_SETLKW:
  case F_GETLK:
    {
      struct flock* arg = va_arg(va, struct flock*);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    }
    break;
  case F_GETOWN_EX:
  case F_SETOWN_EX:
    {
      struct f_owner_ex* arg = va_arg(va, struct f_owner_ex*);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    }
    break;
  default:
    va_end(va);
    return fcntl_f(fd, cmd);
    break;
  }
}

int ioctl(int fd, unsigned long request, ...) {
  va_list va;
  va_start(va, request);
  void* arg = va_arg(va, void*);
  va_end(va);

  if (FIONBIO == request) {
    bool user_nonblock = !!*(int*)arg;
    cool::FdCtx::ptr ctx = cool::FdMgr::instance()->get(fd);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
      return ioctl_f(fd, request, arg);
    }
    ctx->setUserNonBlock(user_nonblock);
  }
  return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
  if (!cool::t_hook_enable) {
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }
  if (level == SOL_SOCKET) {
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      cool::FdCtx::ptr ctx = cool::FdMgr::instance()->get(sockfd);
      if (ctx) {
        const timeval* tv = (const timeval*)optval;
        ctx->setTimeout(optname, tv->tv_sec*1000+tv->tv_usec/1000);
      }
    }
  }
  return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}

