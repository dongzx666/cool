#ifndef __COOL_FD_MANAGER_H
#define __COOL_FD_MANAGER_H

#include "iomanager.h"
#include "singleton.h"
#include "thread.h"
#include <memory>
#include <vector>

namespace cool {
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
  using ptr = std::shared_ptr<FdCtx>;
  FdCtx(int fd);
  ~FdCtx();

  bool init();
  bool isInit() const { return m_isInit; }
  bool isSocket() const { return m_isSocket; }
  bool isClose() const { return m_isClosed; }
  bool clost();

  void setUserNonBlock(bool v) { m_userNonblock = v; }
  bool getUserNonBlock() const { return m_userNonblock; }

  void setSysNonBlock(bool v) { m_sysNonblock = v; }
  bool getSysNonBlock() const { return m_sysNonblock; }

  void setTimeout(int type, uint64_t v);
  uint64_t getTimeout(int type);

private:
  bool m_isInit;
  bool m_isSocket;
  bool m_isClosed;
  bool m_sysNonblock;
  bool m_userNonblock;
  int m_fd;
  uint64_t m_recvTimeout;
  uint64_t m_sendTimeout;
  // cool::IOManager *m_iomanager;
};

class FdManager {
public:
  using RWMutexType = RWMutex;
  FdManager();

  FdCtx::ptr get(int fd, bool auto_create = false);
  void del(int fd);
private:
  RWMutexType m_mutex;
  std::vector<FdCtx::ptr> m_datas;
};

using FdMgr = Singleton<FdManager>;
} // namespace cool
#endif /* ifndef __COOL_FD_MANAGER_H */
