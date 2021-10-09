#ifndef __COOL__TCP_SERVER_H
#define __COOL__TCP_SERVER_H

#include "address.h"
#include "iomanager.h"
#include "noncopyable.h"
#include "socket.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cool {

class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
  using ptr = std::shared_ptr<TcpServer>;
  TcpServer(cool::IOManager *worker = cool::IOManager::GetThis(),
            cool::IOManager *accept_worker = cool::IOManager::GetThis());
  virtual ~TcpServer();
  virtual bool bind(cool::Address::ptr addr);
  virtual bool bind(const std::vector<Address::ptr> &addrs,
                    std::vector<Address::ptr> &fails);
  virtual bool start();
  virtual void stop();

  bool is_stop() const { return m_is_stop; }

protected:
  virtual void handle_client(Socket::ptr client);
  virtual void start_accept(Socket::ptr sock);

public:
  uint64_t m_recv_timeout;
  std::string m_name;

private:
  std::vector<Socket::ptr> m_socks;
  IOManager *m_worker;
  IOManager *m_accept_worker;
  bool m_is_stop;
  bool m_is_start;
};

} /* namespace cool */

#endif /* ifndef __COOL__TCP_SERVER_H */
