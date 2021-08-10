#ifndef __COOL_SCHEDULER_H
#define __COOL_SCHEDULER_H

#include "fiber.h"
#include "thread.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <list>

namespace cool {
class Scheduler {
public:
  using ptr = std::shared_ptr<Scheduler>;
  using MutexType = Mutex;
  // Scheduler();
  Scheduler(size_t thread_size = 1, bool use_caller = true,
            const std::string &name = "");
  virtual ~Scheduler();

  const std::string &name() const { return m_name; }

  void start();
  void stop();

  static Scheduler *GetThis();
  static Fiber *GetMainFiber();

  template <class FiberOrCb> void schedule(FiberOrCb fc, int thread_id = -1) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      need_tickle = scheduleNoLock(fc, thread_id);
    }
    if (need_tickle) {
      tickle();
    }
  }
  template <class InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      while (begin != end) {
        need_tickle = scheduleNoLock(&*begin) || need_tickle;
      }
    }
    if (need_tickle) {
      tickle();
    }
  }

protected:
  virtual void tickle();
  void run();
  virtual bool stopping();
  void setThis();

  std::vector<int> m_thread_ids;
  size_t m_thread_count = 0;
  size_t m_active_thread_count = 0;
  size_t m_idle_thread_count = 0;
  bool m_stop = true;
  bool m_autostop = false;
  int m_root_thread = 0;

private:
  template <class FiberOrCb> bool scheduleNoLock(FiberOrCb fc, int thread_id) {
    bool need_tickle = m_fibers.empty();
    FiberAndThread ft(fc, thread_id);
    if (ft.fiber || ft.cb) {
      m_fibers.push_back(ft);
    }
    return need_tickle;
  }
  struct FiberAndThread {
    Fiber::ptr fiber;
    std::function<void()> cb;
    int thread_id;
    FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread_id(thr) {}
    FiberAndThread(Fiber::ptr *f, int thr) : thread_id(thr) { fiber.swap(*f); }
    FiberAndThread(std::function<void()> f, int thr) : cb(f), thread_id(thr) {}
    FiberAndThread(std::function<void()> *f, int thr) : thread_id(thr) {
      cb.swap(*f);
    }
    FiberAndThread() : thread_id(-1) {}
    void reset() {
      fiber = nullptr;
      cb = nullptr;
      thread_id = -1;
    }
  };
  MutexType m_mutex;
  std::vector<Thread::ptr> m_threads;
  std::string m_name;
  std::list<FiberAndThread> m_fibers;
  Fiber::ptr m_root_fiber;
};

} // namespace cool

#endif /* __COOL_SCHEDULER_H */
