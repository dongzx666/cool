# COOL

## 日志系统

```
Logger:log(level, event)[循环遍历LogAppender,调用log(logger, level, event)]
-> 1. StdoutLogAppender:m_formatter->format(logger, level, event)
-> 1.1 LogFormatter: init，也就是生成m_items
-> 1.1.1 能识别%d %p等格式与XXxFormatItem对应
-> 1.1.2 能识别%xxx{%xxx}, 也就是分为str, format, type解析，比如能给date用
-> 1.2 LogFormatter:i->format(ss, logger, level, event); ->XXXFormatItem: os << xxx
-> 2. FileLogAppender:m_filestream << m_formatter->format(logger, level, event);
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

```
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

// 容器片特化，目前支持vector, list, set, map, unordered_map, unordered_set
// map/unordered_map: key = std::string
// Config::Lookup(key), same key, different value, throw error
```

自定义类型：

需要实现 cool::LexicalCast, 片特化。然后就可以支持 Config 解析自定义类型，自定义类型可以与常规 STL 容器一起使用 1

配置事件机制：

当一个配置项发生修改的时候，可以反向通知对应的代码（即回调）

日志与配置整合：

```yaml
logs:
  name: root
  level: debug, info, warn, error, fatal
  formatter: "%d%T%p%T%t%T%m%n"
  appender:
    - type: StdoutLogAppender, FileLogAppender
      level: debug...
      file: /logs/xxx.log
```

```cpp
cool::Logger g_logger = cool::LoggerMgr::instance()->logger(name);
LOG_INFO(g_logger) << "xxxx log";
```

```cpp
static Logger::ptr g_log = LOG_NAME("system");
// 当logger的appenders为空时，使用root写logger
// 定义LogDefine LogAppenderDefine, 偏特化LexicalCast，实现日志配置解析
```

## 线程库

### Thread:

`Pthread`: `pthread_create` `pthread_join` `pthread_detach`

### Mutex:

互斥量：`mutex`（Mutex, ReadMutex, WriteMutex, SpinMutex(最终采用这个在日志中加锁), CASMutex）
信号量：`semaphore`

## 协程库封装

定义协程接口: `ucontext_t`, `macro`

```
Fiber::GetThis()
Thread -> main_fiber <----> sub_fiber
              ^
              |
              |
              v
           sub_fiber
```

### 协程调度模块：

```
N  -----------   M
1 --- N    1 --- M
scheduler --> thread --> fiber

1. 线程池， 分配一组线程
2. 协程调度器， 将协程指定到相应的线程上去执行

m_threads
m_fibers: std::function<void()>, fiber, thread_id
schedule: func / fiber

start()
stop()
run()
  1. 设置当前线程的scheduler
  2. 设置当前线程的run， fiber
  3. 协程调度循环while(true)
    3.1 协程消息队列里是否有任务
    3.2 无任务执行，执行idle
```

整合 epoll

```
IOManager(epoll) ---> Scheduler
      |
      |
      v
idle(epoll_wait)

信号量
PutMessage(msg,) +信号量1,single()
  message_queue
  |
  |----Thread
  |----Thread
wait()-信号量1，RecvMessage(msg,)
异步IO，等待数据返回。epoll_wait

epoll_create, epoll_ctl, epoll_wait
```

```
Timer -> addTimer() -> cancel()
获取当前的定时器触发离现在的时间差
返回当前需要触发的定时器
```

IOManager 继承了 Scheduler 和 TimerManager

```
[Fiber]             [Timer]
  ^                   ^
  |                   |
  |                   |
[Thread]          [TimerManager]
  ^                   ^
  |                   |
  |                   |
[Scheduler] <---- [IOManager(epoll)]
```

## HOOK

sleep
usleep

socket 相关的(socket, connect, accept)
io 相关 (read, write, send, recv, ...)
fd 相关 (fcntl, ioctl, ...)

## socket 函数库

```
[UnixAddress]
  |                           |-[IPv4Address]
|Address| --- [IPAddress] --- |
  |                           |-[IPv6Address]
  |
--------
|Socket|
--------

connect
accept
read/write/close
```

## 序列化 bytearray

write(int, float, int64, ...)
read(int, float, int64, ...)

## http 协议开发

HTTP/1.1 - API

1. HTTPRequest
2. HTTPResponse

HttpRequest:

GET / HTTP/1.1
Host: www.baidu.com

HTTPResponse:

HTTP/1.1 200 OK
Accept-Ranges: bytes
Cache-Control: no-cache
Connection: keep-alive
Content-Length: 14615
Content-Type: text/html

uri: http://www.baidu.com/page/xxx?id=10&val=20#fr

1. http: protocol
2. www.baidu.com: host
3. 80: port
4. /page/xxx: path
5. id=10&val=20: param
6. fr: fragment

ragel

## TcpServer封装
基于TcpServer实现了一个EchoServer

## Stream 针对文件/socket封装
read/write/readFixSize/writeFixSize

HttpSession/HttpConnection:

Server accept, socket -> session
Client connect, socket -> connection

HttpServer : TcpServer

```
    Servlet <-- FunctionServlet
      |
      |
      v
    ServletDispatch
```
## 分布协议

## 推荐系统
