#include "config.h"
#include "log.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
namespace cool {
// LogLevel
LogLevel::Level LogLevel::from_string(const std::string &v) {
#define XX(level, str)      \
  if (v == #str) {          \
    return LogLevel::level; \
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
#define XX(name)       \
  case LogLevel::name: \
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
}
// LogEvent
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file,
                   uint32_t line, uint32_t elapse, uint32_t thread_id,
                   std::string thread_name, uint32_t fiber_id, uint64_t time)
    : m_file(file), m_line(line), m_elapse(elapse), m_thread_id(thread_id),
      m_thread_name(thread_name), m_fiber_id(fiber_id), m_time(time),
      m_logger(logger), m_level(level) {}

/**
 * @brief 格式化输出，使用不定参数
 *
 * @param fmt, 参数为字符串
 * @param ...
 */
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
LogEventWrap::~LogEventWrap() { m_event->get_logger()->log(m_event); }
std::stringstream &LogEventWrap::get_ss() { return m_event->get_ss(); }

// LogFormatter::FormatItem
class MessageFormatItem : public LogFormatter::FormatItem {
public:
  MessageFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_content();
  }
};
class LevelFormatItem : public LogFormatter::FormatItem {
public:
  LevelFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    // os << LogLevel::to_string(level);
    os << LogLevel::to_string(event->get_level());
  }
};
class ElapseFormatItem : public LogFormatter::FormatItem {
public:
  ElapseFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_elapse();
  }
};
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
  ThreadIdFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_thread_id();
  }
};
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
  ThreadNameFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_thread_name();
  }
};
class NameFormatItem : public LogFormatter::FormatItem {
public:
  NameFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_logger()->get_name();
  }
};
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
  FiberIdFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_fiber_id();
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
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    struct tm tm;
    time_t time = event->get_time();
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
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_file();
  }
};
class LineFormatItem : public LogFormatter::FormatItem {
public:
  LineFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->get_line();
  }
};
class NewLineFormatItem : public LogFormatter::FormatItem {
public:
  NewLineFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << std::endl;
  }
};
class TabFormatItem : public LogFormatter::FormatItem {
public:
  TabFormatItem(const std::string &str = "") {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << "\t";
  }
};
class StringFormatItem : public LogFormatter::FormatItem {
public:
  StringFormatItem(const std::string &str) : m_string(str) {}
  virtual void format(std::ostream &os, LogEvent::ptr event) override {
    os << m_string;
  }

private:
  std::string m_string;
};

// LogFormatter
LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) {
  init();
}
std::string LogFormatter::format(LogEvent::ptr event) {
  std::stringstream ss;
  for (auto &i : m_items) {
    i->format(ss, event);
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
#define XX(str, C)                                                          \
  {                                                                         \
    str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
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
// get and set
void LogAppender::set_formatter(LogFormatter::ptr val) {
  MutexType::Lock lock(m_mutex);
  m_formatter = val;
}
LogFormatter::ptr LogAppender::get_formatter() {
  MutexType::Lock lock(m_mutex);
  return m_formatter;
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
  m_filestream.open(m_filename, std::ios_base::app);
}
void FileLogAppender::log(LogEvent::ptr event) {
  MutexType::Lock lock(m_mutex);
  m_filestream << m_formatter->format(event) << std::flush;
}

void StdoutLogAppender::log(LogEvent::ptr event) {
  MutexType::Lock lock(m_mutex);
  std::cout << m_formatter->format(event);
}

// Logger
Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::DEBUG) {
  // %m -- message
  // %p -- level
  // %r -- start time
  // %c -- name of log
  // %t -- thread id
  // %F -- fiber id
  // %n -- <CR>
  // %d -- timestmp
  // %f -- filename
  // %l -- line number
  // %N -- thread name
  m_formatter.reset(new LogFormatter(
      "%d{%Y-%m-%d %H:%M:%S} %t(%N):%F [%p] [%c] %f:%l %m %n"));
}

void Logger::add_appender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  if (!appender->get_formatter()) {
    appender->set_formatter(m_formatter);
  }
  m_appenders.push_back(appender);
}

void Logger::del_appender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
    if (*it == appender) {
      m_appenders.erase(it);
      break;
    }
  }
}

void Logger::clear_appenders() {
  MutexType::Lock lock(m_mutex);
  m_appenders.clear();
}

// get and set
LogFormatter::ptr Logger::get_formatter() {
  MutexType::Lock lock(m_mutex);
  return m_formatter;
}

void Logger::set_formatter(LogFormatter::ptr val) {
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
void Logger::set_formatter(const std::string &val) {
  cool::LogFormatter::ptr new_val(new cool::LogFormatter(val));
  if (new_val->get_error()) {
    // TODO(fengyu): cout不优雅 [09-10-21] //
    std::cout << "Logger formatter name=" << m_name << "value=" << val
              << " invalid formatter" << std::endl;
    return;
  }
  set_formatter(new_val);
}

void Logger::log(LogEvent::ptr event) {
  if (event->get_level() >= m_level) {
    MutexType::Lock lock(m_mutex);
    if (!m_appenders.empty()) {
      for (auto &i : m_appenders) {
        i->log(event);
      }
    } else {
      LOG_ROOT()->log(event);
    }
  }
}

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

template <>
class LexicalCast<std::string, std::set<LogDefine>> {
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
template <>
class LexicalCast<std::set<LogDefine>, std::string> {
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
    g_log_defines->add_listener([](const std::set<LogDefine> &old_value,
                                   const std::set<LogDefine> &new_value) {
      // std::cout << g_log_defines->to_string() << std::endl;
      LOG_DEBUG(LOG_ROOT()) << "on_logger_conf_changed";
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
        logger->set_level(i.level);
        logger->clear_appenders();
        if (!i.formatter.empty()) {
          logger->set_formatter(i.formatter);
        }
        for (auto &a : i.appenders) {
          cool::LogAppender::ptr ap;
          if (a.type == 1) {
            ap.reset(new FileLogAppender(a.file));
          } else if (a.type == 2) {
            ap.reset(new StdoutLogAppender);
          }
          if (!a.formatter.empty()) {
            ap->set_formatter(LogFormatter::ptr(new LogFormatter(a.formatter)));
          }
          // ap->level(a.level);
          logger->add_appender(ap);
        }
      }
      for (auto &i : old_value) {
        auto it = new_value.find(i);
        if (it == new_value.end()) {
          // delete logger
          auto logger = LOG_NAME(i.name);
          logger->set_level(static_cast<LogLevel::Level>(100));
          logger->clear_appenders();
        }
      }
    });
  }
};
static LogIniter __log_init;
// LoggerManager
LoggerManager::LoggerManager() {
  m_root.reset(new Logger);
  m_root->add_appender(LogAppender::ptr(new StdoutLogAppender));
  m_loggers[m_root->m_name] = m_root;
}
void LoggerManager::add_logger(std::string name, Logger::ptr logger) {
  MutexType::Lock lock(m_mutex);
  m_loggers[name] = logger;
}
Logger::ptr LoggerManager::get_logger(const std::string &name) {
  MutexType::Lock lock(m_mutex);
  auto it = m_loggers.find(name);
  if (it != m_loggers.end()) {
    return it->second;
  }
  // TODO(fengyu): have some problems [11-10-21] //
  // LOG_WARN(LOG_ROOT()) << "LoggerManager doesn't have this logger, please check your input, " << name;
  Logger::ptr logger(new Logger(name));
  m_loggers[name] = logger;
  return logger;
}
} // namespace cool
