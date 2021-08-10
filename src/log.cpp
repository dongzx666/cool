#include "log.h"
#include "config.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
namespace cool {
// LogLevel
LogLevel::Level LogLevel::from_string(const std::string &val) {
#define XX(level, str)                                                         \
  if (val == #str) {                                                           \
    return LogLevel::level;                                                    \
  }
  XX(DEBUG, debug);
  XX(INFO, info);
  XX(WARN, warn);
  XX(ERROR, error);
  XX(FATAL, fatal);
  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);
  return LogLevel::UNKNOWN;
#undef XX
}
const char *LogLevel::to_string(LogLevel::Level level) {
  switch (level) {
#define XX(name)                                                               \
  case LogLevel::name:                                                         \
    return #name;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
  default:
    return "UNKNOWN";
  }
  return "UNKNOWN";
}
// LogEvent
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char *file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, std::string thread_name,
                   uint32_t fiber_id, uint64_t time)
    : m_file(file), m_line(line), m_elapse(elapse), m_thread_id(thread_id),
      m_thread_name(thread_name), m_fiber_id(fiber_id), m_time(time),
      m_logger(logger), m_level(level) {}
LogEvent::~LogEvent() {}
/**
 * @brief 格式化输出，使用不定参数
 *
 * @param fmt, 参数为字符串
 * @param ...
 */
// TODO: 格式化输出和event貌似没什么关系，可以调整？ <20-07-21, fengyu> //
void LogEvent::format(const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  format(fmt, al);
  va_end(al);
}
void LogEvent::format(const char *fmt, va_list al) {
  char *buf = nullptr;
  int len = vasprintf(&buf, fmt, al);
  if (len != -1) {
    m_ss << std::string(buf, len);
    free(buf);
  }
}
// LogEventWrap
LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {}
LogEventWrap::~LogEventWrap() {
  m_event->logger()->log(m_event->level(), m_event);
}
std::stringstream &LogEventWrap::ss() { return m_event->ss(); }
Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::DEBUG) {
  // %m -- message
  // %p -- level
  // %r -- start time
  // %c -- name of log
  // %t -- thread id
  // %T -- fiber id
  // %n -- <CR>
  // %d -- timestmp
  // %f -- filename
  // %l -- line number
  // %N -- thread name
  m_formatter.reset(
      new LogFormatter("%d{%Y-%m-%d %H:%M:%S} %t(%N):%F [%p] [%c] %f:%l %m %n"));
  // if (name == "root") {
  //   m_appenders.push_back(LogAppender::ptr(new StdoutLogAppender));
  // }
}
// LogFormatter::FormatItem
class MessageFormatItem : public LogFormatter::FormatItem {
public:
  MessageFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->content();
  }
};
class LevelFormatItem : public LogFormatter::FormatItem {
public:
  LevelFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << LogLevel::to_string(level);
  }
};
class ElapseFormatItem : public LogFormatter::FormatItem {
public:
  ElapseFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->elapse();
  }
};
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
  ThreadIdFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->thread_id();
  }
};
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
  ThreadNameFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->thread_name();
  }
};
class NameFormatItem : public LogFormatter::FormatItem {
public:
  NameFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->logger()->name();
  }
};
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
  FiberIdFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->fiber_id();
  }
};
class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
  DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
      : m_format(format) {
    if (m_format.empty()) {
      m_format = "%Y-%m-%d %H:%M:%S";
    }
  }
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    struct tm tm;
    time_t time = event->time();
    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), m_format.c_str(), &tm);
    os << buf;
  }

private:
  std::string m_format;
};
class FilenameFormatItem : public LogFormatter::FormatItem {
public:
  FilenameFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->file();
  }
};
class LineFormatItem : public LogFormatter::FormatItem {
public:
  LineFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << event->line();
  }
};
class NewLineFormatItem : public LogFormatter::FormatItem {
public:
  NewLineFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << std::endl;
  }
};
class TabFormatItem : public LogFormatter::FormatItem {
public:
  TabFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << "\t";
  }
};
class StringFormatItem : public LogFormatter::FormatItem {
public:
  StringFormatItem(const std::string &str) : m_string(str) {}
  virtual void format(std::ostream &os, Logger::ptr logger,
                      LogLevel::Level level, LogEvent::ptr event) override {
    os << m_string;
  }

private:
  std::string m_string;
};
// LogFormatter
LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) {
  init();
}
std::string LogFormatter::format(Logger::ptr logger, LogLevel::Level level,
                                 LogEvent::ptr event) {
  std::stringstream ss;
  for (auto &i : m_items) {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}
// %xxx %xxx{xxx} %%
void LogFormatter::init() {
  // str, format, type
  // TODO: 不优雅，有没有其他更好的解析方法？<20-07-21, fengyu> //
  std::vector<std::tuple<std::string, std::string, int>> vec;
  // size_t last_pos = 0;
  std::string nstr;
  for (size_t i = 0; i < m_pattern.size(); ++i) {
    if (m_pattern[i] != '%') {
      nstr.append(1, m_pattern[i]);
      continue;
    }
    if ((i + 1) < m_pattern.size()) {
      if (m_pattern[i + 1] == '%') {
        nstr.append(1, '%');
        continue;
      }
    }
    size_t n = i + 1;
    int fmt_status = 0;
    size_t fmt_begin = 0;
    std::string str;
    std::string fmt;
    while (n < m_pattern.size()) {
      if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                          m_pattern[n] != '}')) {
        str = m_pattern.substr(i + 1, n - i - 1);
        break;
      }
      if (fmt_status == 0) {
        if (m_pattern[n] == '{') {
          str = m_pattern.substr(i + 1, n - i - 1);
          fmt_status = 1; // 解析格式
          fmt_begin = n;
          ++n;
          continue;
        }
      } else if (fmt_status == 1) {
        if (m_pattern[n] == '}') {
          fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          fmt_status = 0;
          ++n;
          break;
        }
      }
      ++n;
      if (n == m_pattern.size()) {
        if (str.empty()) {
          str = m_pattern.substr(i + 1);
        }
      }
    }
    if (fmt_status == 0) {
      if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
        nstr.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n - 1;
    } else if (fmt_status == 1) {
      std::cout << "pattern parse error: " << m_pattern << " - "
                << m_pattern.substr(i) << std::endl;
      vec.push_back(std::make_tuple("<<parse_error>>", fmt, 0));
      m_error = true;
    }
  }
  if (!nstr.empty()) {
    vec.push_back(std::make_tuple(nstr, "", 0));
  }
  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string &str)>>
      s_format_items = {
#define XX(str, C)                                                             \
  {                                                                            \
    str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); }    \
  }
          XX("m", MessageFormatItem),
          XX("p", LevelFormatItem),
          XX("r", ElapseFormatItem),
          XX("c", NameFormatItem),
          XX("t", ThreadIdFormatItem),
          XX("n", NewLineFormatItem),
          XX("d", DateTimeFormatItem),
          XX("f", FilenameFormatItem),
          XX("l", LineFormatItem),
          XX("T", TabFormatItem),
          XX("F", FiberIdFormatItem),
          XX("N", ThreadNameFormatItem)
#undef XX
      };
  for (auto &i : vec) {
    if (std::get<2>(i) == 0) {
      m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    } else {
      auto it = s_format_items.find(std::get<0>(i));
      if (it == s_format_items.end()) {
        m_items.push_back(FormatItem::ptr(
            new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
        m_error = true;
      } else {
        m_items.push_back(it->second(std::get<1>(i)));
      }
    }
    // std::cout << "{" << std::get<0>(i) << "} - {" << std::get<1>(i) << "} -
    // {" << std::get<2>(i) << "}" << std::endl;
  }
  // std::cout << m_items.size() << std::endl;
}
// LogAppender
void LogAppender::formatter(LogFormatter::ptr val) {
  MutexType::Lock lock(m_mutex);
  m_formatter = val;
}
LogFormatter::ptr LogAppender::formatter() {
  MutexType::Lock lock(m_mutex);
  return m_formatter;
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
  m_filestream.open(m_filename, std::ios_base::app);
}
void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level,
                          LogEvent::ptr event) {
  // TODO: 不禁让人想问真的会有人在写入过程中把文件删除么？ <27-07-21, fengyu>
  MutexType::Lock lock(m_mutex);
  m_filestream << m_formatter->format(logger, level, event);
}
bool FileLogAppender::reopen() {
  MutexType::Lock lock(m_mutex);
  if (m_filestream) {
    m_filestream.close();
  }
  m_filestream.open(m_filename);
  return !!m_filestream;
}
std::string FileLogAppender::to_yaml_string() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "FileLogAppender";
  node["file"] = m_filename;
  // node["level"] = LogLevel::to_string(m_level);
  if (m_formatter) {
    node["formatter"] = m_formatter->pattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}
void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level,
                            LogEvent::ptr event) {
  // if (level >= m_level) {
  MutexType::Lock lock(m_mutex);
  std::cout << m_formatter->format(logger, level, event);
  // }
}
std::string StdoutLogAppender::to_yaml_string() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "StdoutLogAppender";
  // node["level"] = LogLevel::to_string(m_level);
  if (m_formatter) {
    node["formatter"] = m_formatter->pattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}
// Logger
std::string Logger::to_yaml_string() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["name"] = m_name;
  node["level"] = LogLevel::to_string(m_level);
  if (m_formatter) {
    node["formatter"] = m_formatter->pattern();
  }
  for (auto &i : m_appenders) {
    node["appenders"].push_back(YAML::Load(i->to_yaml_string()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}
void Logger::addAppender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  if (!appender->formatter()) {
    // TODO: appender的formatter内部已经加锁，这里无需加了 <27-07-21, fengyu> //
    // MutexType::Lock ll(appender->m_mutex);
    appender->formatter(m_formatter);
  }
  m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
    if (*it == appender) {
      m_appenders.erase(it);
    }
  }
}

void Logger::clearAppenders() {
  MutexType::Lock lock(m_mutex);
  m_appenders.clear();
}

LogFormatter::ptr Logger::formatter() {
  MutexType::Lock lock(m_mutex);
  return m_formatter;
}

void Logger::formatter(LogFormatter::ptr val) {
  MutexType::Lock lock(m_mutex);
  // for (auto& i : m_appenders) {
  // TODO: 粗糙的写法,如果formatter修改，需要与appender的formatter同步
  // // if (i->formatter()->format() == m_formatter) {
  //   MutexType::Lock ll(i->m_mutex);
  //   i->formatter(val);
  // }
  // }
  m_formatter = val;
}
void Logger::formatter(const std::string &val) {
  cool::LogFormatter::ptr new_val(new cool::LogFormatter(val));
  if (new_val->error()) {
    std::cout << "Logger formatter name=" << m_name << "value=" << val
              << " invalid formatter" << std::endl;
    return;
  }
  formatter(new_val);
}
// void Logger::formatter(LogFormatter::ptr val) {
// }
void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    auto self = shared_from_this();
    MutexType::Lock lock(m_mutex);
    if (!m_appenders.empty()) {
      for (auto &i : m_appenders) {
        i->log(self, level, event);
      }
    } else if (m_root) {
      m_root->log(level, event);
    }
  }
}
void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }
void Logger::info(LogEvent::ptr event) { log(LogLevel::INFO, event); }
void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }
void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }
void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }
// LogAppenderDefine LogDefine LogIniter
struct LogAppenderDefine {
  int type = 0; // 1 file, 2 stdout
  // LogLevel::Level level = LogLevel::UNKNOWN;
  std::string formatter;
  std::string file;
  bool operator==(const LogAppenderDefine &oth) const {
    return type == oth.type && formatter == oth.formatter && file == oth.file;
  }
};
struct LogDefine {
  std::string name;
  LogLevel::Level level = LogLevel::UNKNOWN;
  std::string formatter;
  std::vector<LogAppenderDefine> appenders;
  bool operator==(const LogDefine &oth) const {
    return name == oth.name && level == oth.level &&
           formatter == oth.formatter && appenders == oth.appenders;
  }
  bool operator<(const LogDefine &oth) const { return name < oth.name; }
};
template <> class LexicalCast<std::string, std::set<LogDefine>> {
public:
  std::set<LogDefine> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    std::set<LogDefine> vec;
    for (size_t i = 0; i < node.size(); ++i) {
      auto n = node[i];
      if (!n["name"].IsDefined()) {
        std::cout << "log conig error: name is null, " << n << std::endl;
        continue;
      }
      LogDefine ld;
      // if (n["name"].IsScalar())
      ld.name = n["name"].as<std::string>();
      ld.level = LogLevel::from_string(
          n["level"].IsDefined() ? n["level"].as<std::string>() : "");
      if (n["formatter"].IsDefined()) {
        ld.formatter = n["formatter"].as<std::string>();
      }
      if (n["appenders"].IsDefined()) {
        for (size_t x = 0; x < n["appenders"].size(); ++x) {
          auto a = n["appenders"][x];
          LogAppenderDefine lad;
          if (!a["type"].IsDefined()) {
            std::cout << "log config error: appender type is null," << a
                      << std::endl;
            continue;
          }
          std::string type = a["type"].as<std::string>();
          if (type == "FileLogAppender") {
            lad.type = 1;
            if (!a["file"].IsDefined()) {
              std::cout << "log config error: fileappender file is null," << a
                        << std::endl;
              continue;
            }
            lad.file = a["file"].as<std::string>();
            if (a["formatter"].IsDefined()) {
              lad.formatter = a["formatter"].as<std::string>();
            }
          } else if (type == "StdoutLogAppender") {
            lad.type = 2;
          } else {
            std::cout << "log config error: appender type is null," << a
                      << std::endl;
            continue;
          }
          ld.appenders.push_back(lad);
        }
      }
      vec.insert(ld);
    }
    return vec;
  }
};
template <> class LexicalCast<std::set<LogDefine>, std::string> {
public:
  std::string operator()(const std::set<LogDefine> &v) {
    YAML::Node node;
    for (auto &i : v) {
      YAML::Node n;
      n["name"] = i.name;
      n["level"] = LogLevel::to_string(i.level);
      if (!i.formatter.empty()) {
        n["formatter"] = i.formatter;
      }
      for (auto &a : i.appenders) {
        YAML::Node na;
        if (a.type == 1) {
          na["type"] = "FileLogAppender";
          na["file"] = a.file;
        } else if (a.type == 2) {
          na["type"] = "StdoutLogAppender";
        }
        // na["level"] = LogLevel::to_string(a.level);
        if (!a.formatter.empty()) {
          na["formatter"] = a.formatter;
        }
        n["appenders"].push_back(na);
      }
      node.push_back(n);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
cool::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    cool::Config::lookup("logs", std::set<LogDefine>(), "logs config");
struct LogIniter {
  LogIniter() {
    g_log_defines->addListener([](const std::set<LogDefine> &old_value,
                                  const std::set<LogDefine> &new_value) {
      // std::cout << g_log_defines->to_string() << std::endl;
      LOG_INFO(LOG_ROOT()) << "on_logger_conf_changed";
      for (auto &i : new_value) {
        auto it = old_value.find(i);
        cool::Logger::ptr logger;
        if (it == old_value.end()) {
          // add logger
          // logger.reset(new cool::Logger(i.name));
          logger = LOG_NAME(i.name);
        } else {
          if (!(i == *it)) {
            // edit logger
            logger = LOG_NAME(i.name);
          }
        }
        logger->level(i.level);
        logger->clearAppenders();
        if (!i.formatter.empty()) {
          logger->formatter(i.formatter);
        }
        for (auto &a : i.appenders) {
          cool::LogAppender::ptr ap;
          if (a.type == 1) {
            ap.reset(new FileLogAppender(a.file));
          } else if (a.type == 2) {
            ap.reset(new StdoutLogAppender);
          }
          if (!a.formatter.empty()) {
            ap->formatter(LogFormatter::ptr(new LogFormatter(a.formatter)));
          }
          // ap->level(a.level);
          logger->addAppender(ap);
        }
      }
      for (auto &i : old_value) {
        auto it = new_value.find(i);
        if (it == new_value.end()) {
          // delete logger
          auto logger = LOG_NAME(i.name);
          logger->level(static_cast<LogLevel::Level>(100));
          logger->clearAppenders();
        }
      }
    });
  }
};
static LogIniter __log_init;
// LoggerManager
LoggerManager::LoggerManager() {
  m_root.reset(new Logger);
  m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
  m_loggers[m_root->m_name] = m_root;
  init();
}
void LoggerManager::addLogger(std::string name, Logger::ptr logger) {
  MutexType::Lock lock(m_mutex);
  m_loggers[name] = logger;
}
Logger::ptr LoggerManager::logger(const std::string &name) {
  MutexType::Lock lock(m_mutex);
  // TODO: 或许可以把map改成vector避免重复的string？ <20-07-21, fengyu> //
  auto it = m_loggers.find(name);
  if (it != m_loggers.end()) {
    return it->second;
  }
  Logger::ptr logger(new Logger(name));
  logger->m_root = m_root;
  m_loggers[name] = logger;
  return logger;
}
std::string LoggerManager::to_yaml_string() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  for (auto &i : m_loggers) {
    // std::cout << i.first << std::endl;
    node.push_back(YAML::Load(i.second->to_yaml_string()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}
void LoggerManager::init() {}
} // namespace cool
