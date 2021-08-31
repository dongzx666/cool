#ifndef __COOL_IOMANAGER_H
#define __COOL_IOMANAGER_H

#include "fiber.h"
#include "scheduler.h"
#include "timer.h"
#include <atomic>
#include <functional>
#include <memory>
namespace cool {
class IOManager : public Scheduler, public TimerManager {
public:
  using ptr = std::shared_ptr<IOManager>;
  using RWMutexType = RWMutex;

  enum Event {
    NONE = 0x0,
    READ = 0x1, // EPOLLIN
    WRITE = 0x4 // EPOLLOUT
  };

  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string &name = "");
  ~IOManager();

  // 1 success, 0 retry, -1 error
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);
  bool cancekAll(int fd);

  static IOManager *GetThis();

protected:
  void tickle() override;
  bool stopping() override;
  bool stopping(uint64_t timeout);
  void idle() override;

  void resizeContext(size_t size);

  void onTimerInsertAtFront() override;

private:
  struct FdContext {
    using MutexType = Mutex;
    struct EventContext {
      Scheduler *scheduler;     // 待执行的scheduler
      Fiber::ptr fiber;         // 事件协程
      std::function<void()> cb; // 事件的回调函数
    };

    EventContext &getContext(Event event);
    void resetContext(EventContext &ctx);
    void triggerEvent(Event event);

    int fd;                     // 事件关联的句柄
    EventContext read;          // 读事件
    EventContext write;         // 写事件
    Event events = Event::NONE; // 已经注册的事件
    MutexType mutex;            // 锁
  };
  int m_epfd = 0;
  int m_tickleFds[2];

  std::atomic<size_t> m_pendingEventCount = {0};
  RWMutexType m_mutex;
  std::vector<FdContext *> m_fdContexts;
};
} // namespace cool

#endif /* ifndef __COOL_IOMANAGER_H */
