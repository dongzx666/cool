#include "http_server.h"
#include "src/http/http.h"
#include "src/http/http_session.h"
#include "src/log.h"
#include <cerrno>
#include <cstring>

namespace cool {
LOGGER_DEF(g_logger, "system");
namespace http {

HttpServer::HttpServer(bool keepalive, cool::IOManager *worker,
                       cool::IOManager *accept_worker)
    : TcpServer(worker, accept_worker), m_is_keep_alive(keepalive) {
  m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handle_client(Socket::ptr client) {
  HttpSession::ptr session(new HttpSession(client));
  do {
    auto req = session->recvRequest();
    if (!req) {
      LOG_WARN(g_logger) << "recv http request fail, client = " << *client;
      break;
    }
    HttpResponse::ptr rsp(
        new HttpResponse(req->version(), req->isClose() || !m_is_keep_alive));
    m_dispatch->handle(req, rsp, session);
    // rsp->body("hello world");

    // LOG_DEBUG(g_logger) << "request: " << std::endl << *req;
    // LOG_DEBUG(g_logger) << "response: " << std::endl << *rsp;

    session->sendResponse(rsp);
  } while (m_is_keep_alive);
  session->close();
}

} /* namespace http */
} /* namespace cool */
