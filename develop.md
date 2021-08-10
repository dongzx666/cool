# 项目

## 日志模块

### 结构
```
Logger： 日志器，负责输出日志
  LogForMatter: 日志器的输出格式
  LogAppender： 日志器的输出目的地
  LogEvent: 日志器的输出内容
  LogEventWrap: 将LogEvent与Logger捆绑
LogManager: 日志管理，单例模式，统一管理所有的日志器
```

### 改进

1. 源码函数名`__FUNCTION__`
2. 日志输出的定向可以设置打开还是关闭
3. crash自动trace技术，可以在崩溃时，自动记录最后32次函数调用，直接定位故障点
4. 感觉四个级别够了？INFO，DEBUG，WARN，ERROR
5. 服务端的每个请求都应该有一个唯一的消息id, 每条日志要携带
6. 日志保留的概念，切分文件？清理文件？

## 配置模块

### 描述

一个配置类应该包括：
1. 名称
2. 类型，可以是基本类型，stl类型，复杂自定义类型
3. 值，应该设置默认值
4. 监听器，当配置更改时有一个回调函数

### 结构

`ConfigVarBase`: 配置虚基类，定义公共成员（名称，描述）和纯虚函数（`to_string`, `from_string`）
`ConfigVar`: 配置类，且为模板类(`T`：类型；`FromStr`：仿函数，yaml字符串转类型`T`；`ToStr`：`T`转为yaml字符串)。

`Config`: 配置管理类，单例模式，统一管理所有的`ConfigVar`。`lookup`：查询配置，同时负责创新新配置赋予默认值。
`load_from_yaml`：从yaml中加载配置。

### 改进

1. 支持命令行启动携带参数
2. 配置参数的校验

## 线程模块

### 结构

`Semaphore`: 计数信号量，基于`sem_t`实现
`Mutex`: 互斥锁，基于`pthread_mutex_t`实现
`RWMutex`: 读写锁，基于`pthread_rwlock_t`实现
`SpinMutex`: 自旋锁，基于`pthread_spinlock_t`实现
`CASMutex`: 原子锁，基于`std::atomic_flag`实现

### 要点

1. 使用信号量保证线程类构造函数执行后，线程函数一定处于执行状态。（构造函数中阻塞，线程函数启动时释放）
2. `CASMutex`效果不明显，在日志中使用`SpinMutex`，泛泛的地方使用`Mutex`，读写各有侧重的时候使用`RWMutex`
3. 所有锁都利用了C++的RAII，即在构造函数中加锁，在析构函数中释放。

### 改进

1. 只是对pthread的各函数做了简单的RAII封装，改进空间暂时没思路
2. 检查日志和配置中对锁使用的粒度情况
3. 学习pthread的使用
