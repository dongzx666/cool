#include "servlet.h"
#include "src/log.h"
#include <fnmatch.h>
#include <string>
#include <utility>

namespace cool {
LOGGER_DEF(g_logger, "system");
namespace http {

FunctionServlet::FunctionServlet(callback cb)
    : Servlet("FunctionServlet"), m_cb(cb) {}

int32_t FunctionServlet::handle(cool::http::HttpRequest::ptr request,
                                cool::http::HttpResponse::ptr response,
                                cool::http::HttpSession::ptr session) {
  return m_cb(request, response, session);
}

ServletDispatch::ServletDispatch() : Servlet("ServletDispatch") {
  m_default.reset(new NotFoundServlet);
}

int32_t ServletDispatch::handle(cool::http::HttpRequest::ptr request,
                                cool::http::HttpResponse::ptr response,
                                cool::http::HttpSession::ptr session) {
  auto slt = get_matched_servlet(request->path());
  if (slt) {
    slt->handle(request, response, session);
  }
  return 0;
}

void ServletDispatch::add_servlet(const std::string &uri, Servlet::ptr slt) {
  RWMutexType::WriteLock lock{m_mutex};
  m_datas[uri] = slt;
}

void ServletDispatch::add_servlet(const std::string &uri,
                                  FunctionServlet::callback cb) {
  RWMutexType::WriteLock lock{m_mutex};
  m_datas[uri].reset(new FunctionServlet(cb));
}

void ServletDispatch::add_glob_servlet(const std::string &uri,
                                       Servlet::ptr slt) {
  RWMutexType::WriteLock lock{m_mutex};
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    if (it->first == uri) {
      m_globs.erase(it);
      break;
    }
  }
  m_globs.push_back(std::make_pair(uri, slt));
}

void ServletDispatch::add_glob_servlet(const std::string &uri,
                                       FunctionServlet::callback cb) {
  return add_glob_servlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispatch::del_servlet(const std::string &uri) {
  RWMutexType::WriteLock lock{m_mutex};
  m_datas.erase(uri);
}

void ServletDispatch::del_glob_servlet(const std::string &uri) {
  RWMutexType::WriteLock lock{m_mutex};
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    if (it->first == uri) {
      m_globs.erase(it);
      break;
    }
  }
}

Servlet::ptr ServletDispatch::get_servlet(const std::string &uri) {
  RWMutexType::ReadLock lock{m_mutex};
  auto it = m_datas.find(uri);
  return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatch::get_glob_servlet(const std::string &uri) {
  RWMutexType::ReadLock lock{m_mutex};
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    if (it->first == uri) {
      return it->second;
    }
  }
  return nullptr;
}

Servlet::ptr ServletDispatch::get_matched_servlet(const std::string &uri) {
  RWMutexType::ReadLock lock{m_mutex};
  auto mit = m_datas.find(uri);
  if (mit != m_datas.end()) {
    return mit->second;
  }
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    // LOG_DEBUG(g_logger) << "first :" << it->first << " uri :" << uri;
    if (0 == fnmatch(it->first.c_str(), uri.c_str(), 0)) {
      return it->second;
    }
  }
  return m_default;
}

NotFoundServlet::NotFoundServlet() : Servlet("NotFoundServlet") {}

int32_t NotFoundServlet::handle(cool::http::HttpRequest::ptr request,
                                cool::http::HttpResponse::ptr response,
                                cool::http::HttpSession::ptr session) {
  static const std::string &RSP_BODY =
      "<html><head><title>404 Not Found</title></head><body>"
      "<center><h1>404 Not Found</h1></center>"
      "<hr><center>cool/1.0.0</center>"
      "</body></html>";
  response->status(cool::http::http_status::NOT_FOUND);
  response->setHeader("Server", "cool/1.0.0");
  response->setHeader("Content-Type", "text/html");
  response->body(RSP_BODY);
  return 0;
}

} /* namespace http */
} /* namespace cool */
