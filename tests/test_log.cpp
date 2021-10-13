#include "src/log.h"
#include "src/util.h"

void test_log_1() {
  cool::Logger::ptr logger(new cool::Logger("test"));
  logger->add_appender(cool::LogAppender::ptr(new cool::StdoutLogAppender));

  // [test] file appender
  cool::FileLogAppender::ptr file_appender(
      new cool::FileLogAppender("./log.txt"));
  logger->add_appender(file_appender);

  // [test] format and level
  // cool::LogFormatter::ptr fmt(new cool::LogFormatter("%d%T%m%n"));
  // file_appender->formatter(fmt);
  // file_appender->level(cool::LogLevel::ERROR);

  // [test] all level marco
  // LOG_INFO(logger) << "test macro info";
  LOG_ERROR(logger) << "test macro error";
  // LOG_FATAL(logger) << "test macro fatal";
  // LOG_WARN(logger) << "test macro warn";
  // LOG_DEBUG(logger) << "test macro debug";

  // [test] fmt
  // LOG_FMT_ERROR(logger, "test macro fmt error %s", "sss");

  // [test] log another style
  // // TODO(fengyu): 不支持行号 [09-10-21] //
  // logger->debug("test log another style");

  // [test] manager
  cool::LoggerMgr::instance()->add_logger("test", logger);
  auto l = cool::LoggerMgr::instance()->get_logger("test");
  LOG_INFO(l) << "test mgr";
}

void test_log_2() {
  cool::Logger::ptr logger(new cool::Logger("test"));
  logger->add_appender(cool::LogAppender::ptr(new cool::StdoutLogAppender));
  logger->add_appender(cool::LogAppender::ptr(new cool::FileLogAppender("./log.txt")));
  LOG_DEBUG(logger) << "test new log";
}

int main() {
  test_log_2();
  return 0;
}
