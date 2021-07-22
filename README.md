# COOL

## Log
```
Logger:`log(level, event)`[循环遍历LogAppender,调用log(logger, level, event)]
  -> 1. StdoutLogAppender:`m_formatter->format(logger, level, event)`
    -> 1.1 LogFormatter: `init`，也就是生成`m_items`
      -> 1.1.1 能识别%d %p等格式与XXxFormatItem对应
      -> 1.1.2 能识别%xxx{%xxx}, 也就是分为str, format, type解析，比如能给date用
    -> 1.2 LogFormatter:`i->format(ss, logger, level, event)`; ->XXXFormatItem: `os << xxx`
  -> 2. FileLogAppender:`m_filestream << m_formatter->format(logger, level, event);`
```

```
Logger(日志器)
  |
  |-------Formatter(日志格式)
  |
Appender(日志输出地)
```



## 配置系统

Config --> Yaml

yaml-cpp: mkdir build && cd build && cmake .. && sudo make install

```cpp
YAML::Node node = YAML::LoadFile(filename);
node.IsMap()
node.IsSequence()
node.IsNull()
node.IsScalar()
```

配置系统的原则是，约定优于配置。减少配置量，大量配置保持不动，少量通过约定调整。

```cpp
template<T, FromStr, ToStr>
class ConfigVar;

template<F, T>
LexicalCast;

// 容器片特化，目前支持vector
```

## 协程库封装
## socket函数库
## http协议开发
## 分布协议
## 推荐系统

