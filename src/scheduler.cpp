#include "scheduler.h"
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include "thread.h"
#include "util.h"
#include <cstddef>
#include <functional>
#include <string>

namespace cool {

static cool::Logger::ptr g_logger = LOG_NAME("system");
static thread_local cool::Scheduler *t_scheduler = nullptr;
static thread_local cool::Fiber *t_fiber = nullptr;

Scheduler::Scheduler(size_t thread_size, bool use_caller,
                     const std::string &name)
    : m_name(name) {
  ASSERT(thread_size > 0);
  if (use_caller) {
    cool::Fiber::GetThis();
    --thread_size;

    ASSERT(GetThis() == nullptr);
    t_scheduler = this;

    m_root_fiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    cool::Thread::SetName(m_name);

    t_fiber = m_root_fiber.get();
    m_root_thread = cool::thread_id();
    m_thread_ids.push_back(m_root_thread);
  } else {
    m_root_thread = -1;
  }
  m_thread_count = thread_size;
}
Scheduler::~Scheduler() {
  ASSERT(m_stop);
  if (GetThis() == this) {
    t_scheduler = nullptr;
  }
}

void Scheduler::start() {
  MutexType::Lock lock{m_mutex};
  if (!m_stop) {
    return;
  }
  m_stop = false;
  ASSERT(m_threads.empty());

  m_threads.resize(m_thread_count);
  for (size_t i = 0; i < m_thread_count; ++i) {
    m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                  m_name + "_" + std::to_string(i)));
    m_thread_ids.push_back(m_threads[i]->id());
  }
  lock.unlock();
  // if (m_root_fiber) {
  //   m_root_fiber->call();
  //   LOG_DEBUG(g_logger) << "call out";
  // }
}
void Scheduler::stop() {
  m_autostop = true;
  if (m_root_fiber && m_thread_count == 0 &&
      (m_root_fiber->state() == Fiber::State::TERM ||
       m_root_fiber->state() == Fiber::State::INIT)) {
    LOG_DEBUG(g_logger) << this << " stop";
    m_stop = true;

    if (stopping()) {
      return;
    }
  }
  // bool exit_on_this_fiber = false;
  if (m_root_thread != -1) {
    ASSERT(GetThis() == this);
  } else {
    ASSERT(GetThis() != this);
  }
  m_stop = true;
  for (size_t i = 0; i < m_thread_count; ++i) {
    tickle();
  }
  if (m_root_fiber) {
    tickle();
  }
  if (m_root_fiber) {
    // while (!stopping()) {
    //   if (m_root_fiber->state() == Fiber::State::TERM ||
    //   m_root_fiber->state() == Fiber::State::ERROR) {
    //     m_root_fiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0,
    //     true)); t_fiber = m_root_fiber.get(); LOG_DEBUG(g_logger) << "root
    //     fiber is term, reset";
    //   }
    //   m_root_fiber->call();
    // }
    if (!stopping()) {
      m_root_fiber->call();
    }
  }
  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock{m_mutex};
    thrs.swap(m_threads);
  }
  for (auto &i : thrs) {
    i->join();
  }
  // if (exit_on_this_fiber) {
  // }
}
void Scheduler::setThis() { t_scheduler = this; }

void Scheduler::run() {
  LOG_DEBUG(g_logger) << "run";
  set_hook_enable(true);
  setThis();
  if (cool::thread_id() != m_root_thread) {
    t_fiber = Fiber::GetThis().get();
  }
  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;

  FiberAndThread ft;
  while (true) {
    ft.reset();
    bool tickle_me = false;
    bool is_active = false;
    {
      MutexType::Lock lock(m_mutex);
      auto it = m_fibers.begin();
      while (it != m_fibers.end()) {
        if (it->thread_id != -1 && it->thread_id != cool::thread_id()) {
          ++it;
          tickle_me = true;
          continue;
        }
        ASSERT(it->fiber || it->cb);
        if (it->fiber && it->fiber->state() == Fiber::State::EXEC) {
          ++it;
          continue;
        }
        ft = *it;
        m_fibers.erase(it);
        ++m_active_thread_count;
        is_active = true;
        break;
      }
    }
    if (tickle_me) {
      tickle();
    }
    if (ft.fiber && (ft.fiber->state() != Fiber::State::TERM ||
                     ft.fiber->state() != Fiber::State::ERROR)) {
      ft.fiber->swapIn();
      --m_active_thread_count;
      if (ft.fiber->state() == Fiber::State::READY) {
        schedule(ft.fiber);
        cb_fiber.reset();
      } else if (ft.fiber->state() != Fiber::State::TERM &&
                 ft.fiber->state() != Fiber::State::ERROR) {
        ft.fiber->state(Fiber::State::HOLD);
      }
      ft.reset();
    } else if (ft.cb) {
      if (cb_fiber) {
        cb_fiber->reset(ft.cb);
      } else {
        cb_fiber.reset(new Fiber(ft.cb));
        ft.cb = nullptr;
      }
      ft.reset();
      cb_fiber->swapIn();
      --m_active_thread_count;
      if (cb_fiber->state() == Fiber::State::READY) {
        schedule(cb_fiber);
        cb_fiber.reset();
      } else if (cb_fiber->state() == Fiber::State::ERROR ||
                 cb_fiber->state() == Fiber::State::TERM) {
        cb_fiber->reset(nullptr);
      } else { // if (cb_fiber->state() != Fiber::State::TERM) {
        cb_fiber->state(Fiber::State::HOLD);
        cb_fiber.reset();
      }
    } else {
      if (is_active) {
        --m_active_thread_count;
        continue;
      }
      if (idle_fiber->state() == Fiber::State::TERM) {
        LOG_DEBUG(g_logger) << "idle fiber term";
        // TODO(fengyu): 后续完善idle [27-08-21] //
        break;
        // continue;
      }
      ++m_idle_thread_count;
      idle_fiber->swapIn();
      --m_idle_thread_count;
      if (idle_fiber->state() != Fiber::State::TERM &&
          idle_fiber->state() != Fiber::State::ERROR) {
        idle_fiber->state(Fiber::State::HOLD);
      }
    }
  }
}

void Scheduler::tickle() { LOG_DEBUG(g_logger) << "tickle"; }
void Scheduler::idle() {
  LOG_DEBUG(g_logger) << "idle";
  while (!stopping()) {
    cool::Fiber::YieldToHold();
  }
}
bool Scheduler::stopping() {
  MutexType::Lock lock(m_mutex);
  return m_autostop && m_stop && m_fibers.empty() && m_active_thread_count == 0;
}

Scheduler *Scheduler::GetThis() { return t_scheduler; }
Fiber *Scheduler::GetMainFiber() { return t_fiber; }
} // namespace cool
