#include "fiber.h"
#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"
#include "src/util.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <ucontext.h>

namespace cool {
static Logger::ptr g_logger = LOG_NAME("system");
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber *t_fiber = nullptr;
static thread_local Fiber::ptr t_thread_fiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::lookup<uint32_t>(
    "fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
  static void *Alloc(size_t size) { return malloc(size); }
  static void Dealloc(void *vp, size_t size) { return free(vp); }

private:
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
  m_state = State::EXEC;
  SetThis(this);

  ASSERT2(getcontext(&m_ctx) == 0, "getcontext");
  ++s_fiber_count;

  LOG_DEBUG(g_logger) << "Fiber::Fiber id=0";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
  ++s_fiber_count;
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->value();
  m_stack = StackAllocator::Alloc(m_stacksize);
  ASSERT2(getcontext(&m_ctx) == 0, "getcontext");
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  if (!use_caller) {
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
  } else {
    makecontext(&m_ctx, &Fiber::CallMainFunc, 0);
  }
  LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

Fiber::~Fiber() {
  --s_fiber_count;
  if (m_stack) {
    ASSERT(m_state == State::TERM || m_state == State::INIT ||
           m_state == State::ERROR);
    StackAllocator::Dealloc(m_stack, m_stacksize);
  } else {
    ASSERT(!m_cb);
    ASSERT(m_state == State::EXEC);
    Fiber *cur = t_fiber;
    if (cur == this) {
      SetThis(nullptr);
    }
  }
  LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id;
}

void Fiber::reset(std::function<void()> cb) {
  ASSERT(m_stack);
  ASSERT(m_state == State::TERM || m_state == State::INIT ||
         m_state == State::ERROR);
  m_cb = cb;
  ASSERT2(getcontext(&m_ctx) == 0, "getcontext");
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  m_state = State::INIT;
}

void Fiber::call () {
  SetThis(this);
  m_state = State::EXEC;
  ASSERT2(swapcontext(&t_thread_fiber->m_ctx, &m_ctx) != -1, "swapcontext");
}
void Fiber::back () {
  SetThis(t_thread_fiber.get());
  ASSERT2(swapcontext(&m_ctx, &t_thread_fiber->m_ctx) != -1,
            "swapcontext");
}
void Fiber::swapIn() {
  SetThis(this);
  ASSERT(m_state != State::EXEC);
  m_state = State::EXEC;
  ASSERT2(swapcontext(&cool::Scheduler::GetMainFiber()->m_ctx, &m_ctx) != -1, "swapcontext");
}
void Fiber::swapOut() {
  SetThis(cool::Scheduler::GetMainFiber());
  ASSERT2(swapcontext(&m_ctx, &cool::Scheduler::GetMainFiber()->m_ctx) != -1,
          "swapcontext");
}

void Fiber::SetThis(Fiber *f) { t_fiber = f; }
Fiber::ptr Fiber::GetThis() {
  if (t_fiber) {
    return t_fiber->shared_from_this();
  }
  Fiber::ptr main_fiber(new Fiber);
  ASSERT(t_fiber == main_fiber.get());
  t_thread_fiber = main_fiber;
  // t_thread_fiber = std::move(main_fiber);
  return t_fiber->shared_from_this();
}
void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThis();
  cur->m_state = State::READY;
  cur->swapOut();
}
void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThis();
  cur->m_state = State::HOLD;
  cur->swapOut();
}

uint64_t Fiber::TotalFibers() { return s_fiber_count; }

void Fiber::MainFunc() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur);
  try {
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = State::TERM;
  } catch (std::exception &ex) {
    cur->m_state = State::ERROR;
    LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << std::endl << cool::backtrace_tostring();
  } catch (...) {
    cur->m_state = State::ERROR;
    LOG_ERROR(g_logger) << "Fiber Except: " << std::endl << cool::backtrace_tostring();
  }
  // TODO: 不优雅 <29-07-21, fengyu> //
  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->swapOut();
  ASSERT2(false, "never reach here");
}
void Fiber::CallMainFunc() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur);
  try {
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = State::TERM;
  } catch (std::exception &ex) {
    cur->m_state = State::ERROR;
    LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << std::endl << cool::backtrace_tostring();
  } catch (...) {
    cur->m_state = State::ERROR;
    LOG_ERROR(g_logger) << "Fiber Except: " << std::endl << cool::backtrace_tostring();
  }
  // TODO: 不优雅 <29-07-21, fengyu> //
  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->back();
  ASSERT2(false, "never reach here");
}
uint64_t Fiber::GetFiberId() {
  if (t_fiber) {
    return t_fiber->m_id;
  }
  return 0;
}
} // namespace cool
