#ifndef __COOL_LOG_H
#define __COOL_LOG_H

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <cctype>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <tuple>
#include <utility>
#include <map>
#include <functional>
#include "singleton.h"

// 普通封装，用法：COOL_LOG_ERROR(logger) << "test macro error";
#define COOL_LOG_LEVEL(logger, level_) \
  if (logger->level() <= level_) \
    cool::LogEventWrap(cool::LogEvent::ptr(new cool::LogEvent(logger, level_, \
    __FILE__, __LINE__, 0, cool::thread_id(), \
    cool::fiber_id(), time(0)))).ss()
#define COOL_LOG_DEBUG(logger) COOL_LOG_LEVEL(logger, cool::LogLevel::DEBUG)
#define COOL_LOG_ERROR(logger) COOL_LOG_LEVEL(logger, cool::LogLevel::ERROR)
#define COOL_LOG_INFO(logger) COOL_LOG_LEVEL(logger, cool::LogLevel::INFO)
#define COOL_LOG_WARN(logger) COOL_LOG_LEVEL(logger, cool::LogLevel::WARN)
#define COOL_LOG_FATAL(logger) COOL_LOG_LEVEL(logger, cool::LogLevel::FATAL)
// FMT封装，用法：COOL_LOG_FMT_ERROR(logger, "test macro fmt error %s", "sss");
#define COOL_LOG_FMT_LEVEL(logger, level_, fmt, ...) \
  if (logger->level() <= level_) \
    cool::LogEventWrap(cool::LogEvent::ptr(new cool::LogEvent(logger, level_, \
    __FILE__, __LINE__, 0, cool::thread_id(), \
    cool::fiber_id(), time(0)))).event()->format(fmt, __VA_ARGS__)
#define COOL_LOG_FMT_DEBUG(logger, fmt, ...) COOL_LOG_FMT_LEVEL(logger, cool::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define COOL_LOG_FMT_ERROR(logger, fmt, ...) COOL_LOG_FMT_LEVEL(logger, cool::LogLevel::ERROR, fmt, __VA_ARGS__)
#define COOL_LOG_FMT_INFO(logger, fmt, ...) COOL_LOG_FMT_LEVEL(logger, cool::LogLevel::INFO, fmt, __VA_ARGS__)
#define COOL_LOG_FMT_WARN(logger, fmt, ...) COOL_LOG_FMT_LEVEL(logger, cool::LogLevel::WARN, fmt, __VA_ARGS__)
#define COOL_LOG_FMT_FATAL(logger, fmt, ...) COOL_LOG_FMT_LEVEL(logger, cool::LogLevel::FATAL, fmt, __VA_ARGS__)

#define COOL_LOG_ROOT() cool::LoggerMgr::instance()->root()

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
      static const char* toString(LogLevel::Level level);
  };
  class Logger;
  // 日志事件
  class LogEvent {
    public:
      using ptr = std::shared_ptr<LogEvent>;
      LogEvent (std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line
          , uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);
      ~LogEvent ();

      const char* file() const {return m_file;}
      int32_t line() const {return m_line;}
      uint32_t elapse() const {return m_elapse;}
      uint32_t thread_id() const {return m_threadId;}
      uint32_t fiber_id() const {return m_fiberId;}
      uint32_t time() const {return m_time;}
      std::string content () const {return m_ss.str();}
      std::shared_ptr<Logger> logger() const {return m_logger;}
      LogLevel::Level level() const {return m_level;}

      std::stringstream& ss() {return m_ss;}
      void format(const char* fmt, ...);
      void format(const char* fmt, va_list al);
    private:
      const char* m_file = nullptr;    //文件名
      int32_t m_line = 0;              //行号
      uint32_t m_elapse = 0;           //运行时间
      uint32_t m_threadId = 0;         //线程id
      uint32_t m_fiberId = 0;          //携程id
      uint64_t m_time = 0;             //时间戳
      std::stringstream m_ss;

      std::shared_ptr<Logger> m_logger;
      // TODO: event,logger都设置level冗余  <20-07-21, fengyu> //
      LogLevel::Level m_level;
      /* data */
  };
  class LogEventWrap {
    public:
      LogEventWrap(LogEvent::ptr e);
      ~LogEventWrap();
      std::stringstream& ss();
      LogEvent::ptr event() const {return m_event;}
    private:
      LogEvent::ptr m_event;
  };
  // 日志格式
  class LogFormatter {
    public:
      using ptr = std::shared_ptr<LogFormatter>;
      LogFormatter(const std::string& pattern);
      std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    public:
      class FormatItem {
        public:
          using ptr = std::shared_ptr<FormatItem>;
          // FormatItem(const std::string& fmt = "") {};
          virtual ~FormatItem() {}
          virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
      };
      void init();
    private:
      std::string m_pattern;
      std::vector<FormatItem::ptr> m_items;
  };
  // 日志输出
  class LogAppender {
    public:
      using ptr = std::shared_ptr<LogAppender>;
      virtual ~LogAppender() {}

      virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

      void formatter(LogFormatter::ptr val) {m_formatter = val;}
      LogFormatter::ptr formatter() const {return m_formatter;}
      void level(LogLevel::Level level) {m_level = level;}
      LogLevel::Level level() const {return m_level;}
    protected:
      LogLevel::Level m_level = LogLevel::DEBUG;
      LogFormatter::ptr m_formatter;
  };
  // 日志器
  class Logger : public std::enable_shared_from_this<Logger>{
    public:
      using ptr = std::shared_ptr<Logger>;
      Logger(const std::string& name = "root");

      void log(LogLevel::Level level, LogEvent::ptr event);
      void debug(LogEvent::ptr event);
      void info(LogEvent::ptr event);
      void warn(LogEvent::ptr event);
      void error(LogEvent::ptr event);
      void fatal(LogEvent::ptr event);

      void addAppender(LogAppender::ptr appender);
      void delAppender(LogAppender::ptr appender);
      LogLevel::Level level() const {return m_level;}
      void level(LogLevel::Level level) {m_level = level;}
      const std::string& name() const {return m_name;}
    private:
      std::string m_name;
      LogLevel::Level m_level;
      std::list<LogAppender::ptr> m_appenders;
      LogFormatter::ptr m_formatter;
  };
  // 输出到命令行的输出器
  class StdoutLogAppender : public LogAppender {
    public:
      using ptr = std::shared_ptr<StdoutLogAppender>;
      void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

  };
  // 输出到文件的输出器
  class FileLogAppender : public LogAppender {
    public:
      using ptr = std::shared_ptr<FileLogAppender>;
      void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
      FileLogAppender(const std::string& filename);

      bool reopen();
    private:
      std::string m_filename;
      std::ofstream m_filestream;
  };

  class LoggerManager {
    public:
      LoggerManager();
      Logger::ptr logger(const std::string& name);
      void addLogger(std::string name, Logger::ptr logger);
      void init();
      Logger::ptr root() const {return m_root;}
   private:
      std::map<std::string, Logger::ptr> m_loggers;
      Logger::ptr m_root;
  };

  using LoggerMgr = cool::Singleton<LoggerManager>;

} /*  cool */
#endif /* ifndef __COOL_LOG_H */

