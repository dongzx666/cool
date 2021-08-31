#ifndef __COOL_TIMER_H
#define __COOL_TIMER_H

#include "thread.h"
#include <cstdint>
#include <functional>
#include <set>
#include <memory>
#include <vector>

namespace cool {

class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> {
  friend class TimerManager;

public:
  using ptr = std::shared_ptr<Timer>;
  bool cancel();
  bool refresh();
  bool reset(uint64_t ms, bool from_now);

private:
  Timer(uint64_t ms, std::function<void()> cb, bool recurring,
        TimerManager *manager);
  Timer(uint64_t next);

  bool m_recurring = false; // 是否循环定时器
  uint64_t m_ms = 0;        // 执行周期
  uint64_t m_next = 0;      // 精确的执行时间
  std::function<void()> m_cb;
  TimerManager* m_manager = nullptr;

  struct Comparator {
    bool operator () (const Timer::ptr& lhs, const Timer::ptr& rhs) const;
  };
};

class TimerManager {
  friend class Timer;
  public:
    using RWMutexType = RWMutex;
    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);
    uint64_t getNextTimer();
    void listExpiredCb(std::vector<std::function<void()>>& cbs);
  protected:
    virtual void onTimerInsertAtFront() = 0;
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
    bool hasTimer();
  private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    bool m_tickled = false;
    uint64_t m_previous_time;
    bool detectClockRollover(uint64_t now_ms);
};
} // namespace cool
#endif /* ifndef __COOL_TIMER_H */
