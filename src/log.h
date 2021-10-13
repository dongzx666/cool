#ifndef __COOL_LOG_H
#define __COOL_LOG_H

#include "singleton.h"
#include "thread.h"
#include "util.h"
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// 普通封装，用法：LOG_ERROR(logger) << "test macro error";
#define LOG_LEVEL(logger, level_)                                   \
  if (logger->get_level() <= level_)                                    \
  cool::LogEventWrap(                                               \
      cool::LogEvent::ptr(new cool::LogEvent(                       \
          logger, level_, __FILE__, __LINE__, 0, cool::thread_id(), \
          cool::Thread::GetName(), cool::fiber_id(), time(0))))     \
      .get_ss()
#define LOG_DEBUG(logger) LOG_LEVEL(logger, cool::LogLevel::DEBUG)
#define LOG_ERROR(logger) LOG_LEVEL(logger, cool::LogLevel::ERROR)
#define LOG_INFO(logger) LOG_LEVEL(logger, cool::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, cool::LogLevel::WARN)
#define LOG_FATAL(logger) LOG_LEVEL(logger, cool::LogLevel::FATAL)
// FMT封装，用法：LOG_FMT_ERROR(logger, "test macro fmt error %s", "sss");
#define LOG_FMT_LEVEL(logger, level_, fmt, ...)                     \
  if (logger->get_level() <= level_)                                    \
  cool::LogEventWrap(                                               \
      cool::LogEvent::ptr(new cool::LogEvent(                       \
          logger, level_, __FILE__, __LINE__, 0, cool::thread_id(), \
          cool::Thread::GetName(), cool::fiber_id(), time(0))))     \
      .get_event()                                                  \
      ->format(fmt, __VA_ARGS__)
#define LOG_FMT_DEBUG(logger, fmt, ...) \
  LOG_FMT_LEVEL(logger, cool::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) \
  LOG_FMT_LEVEL(logger, cool::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) \
  LOG_FMT_LEVEL(logger, cool::LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) \
  LOG_FMT_LEVEL(logger, cool::LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) \
  LOG_FMT_LEVEL(logger, cool::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT() cool::LoggerMgr::instance()->get_root()
#define LOG_NAME(name) cool::LoggerMgr::instance()->get_logger(name);

#define LOGGER_DEF(variable, name) \
  static cool::Logger::ptr variable = LOG_NAME(name);

namespace cool {
// 日志级别
class LogLevel {
public:
  enum Level {
    UNKNOWN = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };
  // TODO(fengyu): 是不是可以参照http_status_map宏的写法更优雅呢 [09-10-21] //
  static const char *to_string(LogLevel::Level level);
  static LogLevel::Level from_string(const std::string &v);
};
class Logger;
class LoggerManager;
// 日志事件
class LogEvent {
public:
  using ptr = std::shared_ptr<LogEvent>;
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char *file, uint32_t line, uint32_t elapse, uint32_t thread_id,
           std::string thread_name, uint32_t fiber_id, uint64_t time);
  ~LogEvent() {}

  const char *get_file() const { return m_file; }
  uint32_t get_line() const { return m_line; }
  uint32_t get_elapse() const { return m_elapse; }
  uint32_t get_thread_id() const { return m_thread_id; }
  const std::string &get_thread_name() const { return m_thread_name; }
  uint32_t get_fiber_id() const { return m_fiber_id; }
  uint32_t get_time() const { return m_time; }
  std::string get_content() const { return m_ss.str(); }
  std::shared_ptr<Logger> get_logger() const { return m_logger; }
  LogLevel::Level get_level() const { return m_level; }
  std::stringstream &get_ss() { return m_ss; }

  void format(const char *fmt, ...);
  void format(const char *fmt, va_list al);

private:
  const char *m_file = nullptr; //文件名
  uint32_t m_line = 0;        //行号
  uint32_t m_elapse = 0;     //运行时间
  uint32_t m_thread_id = 0;  //线程id
  std::string m_thread_name; //线程名称
  uint32_t m_fiber_id = 0;   //携程id
  uint64_t m_time = 0;       //时间戳
  std::stringstream m_ss;

  std::shared_ptr<Logger> m_logger;
  LogLevel::Level m_level;
};
class LogEventWrap {
public:
  LogEventWrap(LogEvent::ptr e);
  ~LogEventWrap();
  std::stringstream &get_ss();
  LogEvent::ptr get_event() const { return m_event; }

private:
  LogEvent::ptr m_event;
};
// 日志格式
class LogFormatter {
public:
  using ptr = std::shared_ptr<LogFormatter>;
  LogFormatter(const std::string &pattern);
  std::string format(LogEvent::ptr event);
  bool get_error() const { return m_error; }
  const std::string set_pattern() const { return m_pattern; }

public:
  class FormatItem {
  public:
    using ptr = std::shared_ptr<FormatItem>;
    // FormatItem(const std::string& fmt = "") {};
    virtual ~FormatItem() {}
    virtual void format(std::ostream &os,
                        LogEvent::ptr event) = 0;
  };
  void init();

private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
  bool m_error = false;
};
// 日志输出
class LogAppender {
  friend class Logger;

public:
  using ptr = std::shared_ptr<LogAppender>;
  using MutexType = SpinLock;
  virtual ~LogAppender() {}

  virtual void log(LogEvent::ptr event) = 0;

  void set_formatter(LogFormatter::ptr val);
  LogFormatter::ptr get_formatter();

protected:
  LogFormatter::ptr m_formatter;
  MutexType m_mutex;
};
// 日志器
class Logger {
  friend class LoggerManager;

public:
  using ptr = std::shared_ptr<Logger>;
  using MutexType = SpinLock;
  Logger(const std::string &name = "root");

  void log(LogEvent::ptr event);

  void add_appender(LogAppender::ptr appender);
  void del_appender(LogAppender::ptr appender);
  void clear_appenders();
  LogLevel::Level get_level() const { return m_level; }
  void set_level(LogLevel::Level level) { m_level = level; }
  const std::string &get_name() const { return m_name; }
  void set_formatter(LogFormatter::ptr val);
  void set_formatter(const std::string &val);
  LogFormatter::ptr get_formatter();


private:
  std::string m_name;
  LogLevel::Level m_level;
  std::list<LogAppender::ptr> m_appenders;
  LogFormatter::ptr m_formatter;
  MutexType m_mutex;
};
// 输出到命令行的输出器
class StdoutLogAppender : public LogAppender {
public:
  using ptr = std::shared_ptr<StdoutLogAppender>;
  void log(LogEvent::ptr event) override;
};
// 输出到文件的输出器
class FileLogAppender : public LogAppender {
public:
  using ptr = std::shared_ptr<FileLogAppender>;
  void log(LogEvent::ptr event) override;
  FileLogAppender(const std::string &filename);
  ~FileLogAppender() {m_filestream.close();}


private:
  std::string m_filename;
  std::ofstream m_filestream;
};

class LoggerManager {
public:
  // TODO(fengyu): don't use spinlock in user mode [09-10-21] //
  using MutexType = SpinLock;
  LoggerManager();
  Logger::ptr get_logger(const std::string &name);
  void add_logger(std::string name, Logger::ptr logger);
  Logger::ptr get_root() const { return m_root; }

private:
  MutexType m_mutex;
  std::map<std::string, Logger::ptr> m_loggers;
  Logger::ptr m_root;
};

using LoggerMgr = cool::Singleton<LoggerManager>;

} // namespace cool
#endif /* ifndef __COOL_LOG_H */
