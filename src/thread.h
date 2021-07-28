#ifndef COOL_THREAD_H
#define COOL_THREAD_H

#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <string>
#include <thread>

namespace cool {
class Semaphore {
public:
  Semaphore(uint32_t count = 0);
  ~Semaphore();

  void wait();
  void notify();

private:
  Semaphore(const Semaphore &) = delete;
  Semaphore(const Semaphore &&) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

  sem_t m_semaphore;
};

template <class T> struct ScopedLockImpl {
public:
  ScopedLockImpl(T &mutex) : m_mutex(mutex) { lock(); }
  ~ScopedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T &m_mutex;
  bool m_locked = false;
};

template <class T> struct ReadScopedLockImpl {
public:
  ReadScopedLockImpl(T &mutex) : m_mutex(mutex) { lock(); }
  ~ReadScopedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T &m_mutex;
  bool m_locked = false;
};

template <class T> struct WriteScopedLockImpl {
public:
  WriteScopedLockImpl(T &mutex) : m_mutex(mutex) { lock(); }
  ~WriteScopedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T &m_mutex;
  bool m_locked = false;
};

class Mutex {
public:
  using Lock = ScopedLockImpl<Mutex>;
  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
  ~Mutex() { pthread_mutex_destroy(&m_mutex); }
  void lock() { pthread_mutex_lock(&m_mutex); }
  void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
  pthread_mutex_t m_mutex;
};

class NullMutex {
public:
  using Lock = ScopedLockImpl<NullMutex>;
  NullMutex() {}
  ~NullMutex() {}
  void lock() {}
  void unlock() {}
};

class RWMutex {
public:
  using ReadLock = ReadScopedLockImpl<RWMutex>;
  using WriteLock = WriteScopedLockImpl<RWMutex>;
  RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }
  ~RWMutex() { pthread_rwlock_destroy(&m_lock); }
  void rdlock() { pthread_rwlock_rdlock(&m_lock); }
  void wrlock() { pthread_rwlock_wrlock(&m_lock); }
  void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
  pthread_rwlock_t m_lock;
};

class NullRWMutex {
public:
  using ReadLock = ReadScopedLockImpl<NullRWMutex>;
  using WriteLock = WriteScopedLockImpl<NullRWMutex>;
  NullRWMutex() {}
  ~NullRWMutex() {}
  void rdlock() {}
  void wrlock() {}
  void unlock() {}
};

class SpinMutex {
public:
  using Lock = ScopedLockImpl<SpinMutex>;
  SpinMutex() { pthread_spin_init(&m_mutex, 0); }
  ~SpinMutex() { pthread_spin_destroy(&m_mutex); }
  void lock() { pthread_spin_lock(&m_mutex); }
  void unlock() { pthread_spin_unlock(&m_mutex); }

private:
  pthread_spinlock_t m_mutex;
};

class CASMutex {
public:
  using Lock = ScopedLockImpl<CASMutex>;
  CASMutex () {
    m_mutex.clear();
  }
  ~CASMutex () {}
  void lock () {
    while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
  }
  void unlock () {
    std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
  }
private:
  volatile std::atomic_flag m_mutex;
};

class Thread {
public:
  using ptr = std::shared_ptr<Thread>;
  Thread(std::function<void()> cb, const std::string &name);
  ~Thread();

  const std::string name() const { return m_name; }
  pid_t id() const { return m_id; }

  void join();
  static Thread *GetThis();
  static const std::string &GetName();
  static void SetName(const std::string &name);

private:
  Thread(const Thread &) = delete;
  Thread(const Thread &&) = delete;
  Thread &operator=(const Thread &) = delete;

  static void *run(void *arg);

  pid_t m_id = -1;
  pthread_t m_thread = 0;
  std::function<void()> m_cb;
  std::string m_name;
  Semaphore m_semaphore;
};
} // namespace cool

#endif /* ifndef COOL_THREAD_H */