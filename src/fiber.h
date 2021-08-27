#ifndef __COOL_FIBER_H
#define __COOL_FIBER_H

#include "thread.h"
#include <functional>
#include <memory>
#include <sys/ucontext.h>
#include <ucontext.h>

namespace cool {
class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;
public:
  using ptr = std::shared_ptr<Fiber>;

  enum class State { INIT, HOLD, EXEC, TERM, READY, ERROR };

  Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
  ~Fiber();
  const State state() const { return m_state; }
  void state(Fiber::State s) { m_state = s; }

  void reset(std::function<void()> cb); // 重置协程函数和状态(INIT, TERM)
  void swapIn();                        // 切换到当前协程执行
  void swapOut();                       // 切换到后台执行
  void call();
  void back();

  static Fiber::ptr GetThis();
  static void SetThis(Fiber *f);
  static void YieldToReady();    // 协程切换到后台，并设置为Ready
  static void YieldToHold();     // 协程切换到后台，并设置为Hold
  static uint64_t TotalFibers(); // 总协程数
  static void MainFunc();
  static void CallMainFunc();
  static uint64_t GetFiberId();

private:
  Fiber();
  uint64_t m_id = 0;
  uint32_t m_stacksize = 0;
  State m_state = State::INIT;
  ucontext_t m_ctx;
  void *m_stack = nullptr;
  std::function<void()> m_cb;
};
} // namespace cool

#endif /* ifndef __COOL_FIBER_H */
