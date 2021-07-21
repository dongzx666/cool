# COOL

## Log

Logger:`log(level, event)`[循环遍历LogAppender,调用log(logger, level, event)]
  -> 1. StdoutLogAppender:`m_formatter->format(logger, level, event)`
    -> 1.1 LogFormatter: `init`，也就是生成`m_items`
      -> 1.1.1 能识别%d %p等格式与XXxFormatItem对应
      -> 1.1.2 能识别%xxx{%xxx}, 也就是分为str, format, type解析，比如能给date用
    -> 1.2 LogFormatter:`i->format(ss, logger, level, event)`; ->XXXFormatItem: `os << xxx`
  -> 2. FileLogAppender:`m_filestream << m_formatter->format(logger, level, event);`

```
Logger(日志器)
  |
  |-------Formatter(日志格式)
  |
Appender(日志输出地)
```

## 配置系统

Config --> Yaml


