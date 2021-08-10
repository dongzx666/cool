#include "scheduler.h"
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "src/thread.h"
#include "src/util.h"
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

    m_root_fiber.reset(new Fiber(std::bind(&Scheduler::run, this)));
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
  MutexType::Lock lock(m_mutex);
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
}
void Scheduler::stop() {
  m_autostop = true;
  if (m_root_fiber && m_thread_count == 0 &&
      (m_root_fiber->state() == Fiber::State::TERM ||
       m_root_fiber->state() == Fiber::State::INIT)) {
    LOG_INFO(g_logger) << this << " stop";
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
  // if (exit_on_this_fiber) {

  // }
  if (m_root_fiber) {
    tickle();
  }
  if (stopping()) {
    return;
  }
}
void Scheduler::setThis() { t_scheduler = this; }
void Scheduler::run() {
  setThis();
  if (cool::thread_id() != m_root_thread) {
    t_fiber = Fiber::GetThis().get();
  }
  Fiber::ptr idle_fiber(new Fiber());
}

Scheduler *Scheduler::GetThis() { return t_scheduler; }
Fiber *Scheduler::GetMainFiber() { return t_fiber; }
} // namespace cool
