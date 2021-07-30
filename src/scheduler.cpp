#include "scheduler.h"
#include "log.h"

namespace cool {
static cool::Logger::ptr g_logger = LOG_NAME("system");
Scheduler::Scheduler(size_t thread_size = 1, bool use_caller = true,
                     const std::string &name = "") {}
Scheduler::~Scheduler() {}

void Scheduler::start() {}
void Scheduler::stop() {}

Scheduler *Scheduler::GetThis() {}
Fiber *Scheduler::GetMainFiber() {}
} // namespace cool
