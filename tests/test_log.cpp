#include "src/log.h"
#include "src/util.h"

int main() {
  cool::Logger::ptr logger(new cool::Logger("test"));
  logger->addAppender(cool::LogAppender::ptr(new cool::StdoutLogAppender));

  // cool::LogEvent::ptr event1(new cool::LogEvent(__FILE__, __LINE__, 0,
  // (uint32_t)cool::getThreadId(), cool::getFiberId(), time(0)));
  // event1->getSS() << "test log 1";
  // logger->log(cool::LogLevel::DEBUG, event1);
  // std::cout << "hello cool!" << std::endl;
  cool::FileLogAppender::ptr file_appender(
      new cool::FileLogAppender("./log.txt"));
  logger->addAppender(file_appender);
  cool::LogFormatter::ptr fmt(new cool::LogFormatter("%d%T%m%n"));
  file_appender->formatter(fmt);
  // file_appender->level(cool::LogLevel::ERROR);

  // LOG_INFO(logger) << "test macro info";
  LOG_ERROR(logger) << "test macro error";
  // LOG_FATAL(logger) << "test macro fatal";
  // LOG_WARN(logger) << "test macro warn";
  // LOG_DEBUG(logger) << "test macro debug";

  // LOG_FMT_ERROR(logger, "test macro fmt error %s", "sss");

  cool::LoggerMgr::instance()->addLogger("test", logger);
  auto l = cool::LoggerMgr::instance()->logger("test");
  LOG_INFO(l) << "test mgr";
  return 0;
}
