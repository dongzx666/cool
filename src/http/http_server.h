#ifndef __COOL_HTTP_HTTP_SERVER
#define __COOL_HTTP_HTTP_SERVER

#include "http_session.h"
#include "servlet.h"
#include "src/tcp_server.h"
#include <memory>

namespace cool {
namespace http {

class HttpServer : public TcpServer {
public:
  using ptr = std::shared_ptr<HttpServer>;
  HttpServer(bool keepalive = false,
             cool::IOManager *worker = cool::IOManager::GetThis(),
             cool::IOManager *accept_worker = cool::IOManager::GetThis());
  ServletDispatch::ptr get_servlet_dispatch() const { return m_dispatch; }
  void set_servlet_dispatch(ServletDispatch::ptr v) {m_dispatch = v;}

protected:
  virtual void handle_client(Socket::ptr client) override;

private:
  bool m_is_keep_alive;
  ServletDispatch::ptr m_dispatch;
};

} /* namespace http */
} /* namespace cool */

#endif /* ifndef __COOL_HTTP_HTTP_SERVER */
