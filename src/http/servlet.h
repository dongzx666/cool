#ifndef __COOL_HTTP_SERVLET_H
#define __COOL_HTTP_SERVLET_H

#include "http.h"
#include "http_session.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "src/thread.h"

namespace cool {
namespace http {

class Servlet {
public:
  using ptr = std::shared_ptr<Servlet>;
  Servlet(const std::string &name) : m_name(name) {}
  virtual ~Servlet() {}
  // TODO(fengyu): session感觉这里没必要作参数 [09-10-21] //
  virtual int32_t handle(cool::http::HttpRequest::ptr request,
                         cool::http::HttpResponse::ptr response,
                         cool::http::HttpSession::ptr session) = 0;
  const std::string &name() const { return m_name; }

protected:
  std::string m_name;
};

class FunctionServlet : public Servlet {
public:
  using ptr = std::shared_ptr<FunctionServlet>;
  using callback = std::function<int32_t (cool::http::HttpRequest::ptr request,
                                         cool::http::HttpResponse::ptr response,
                                         cool::http::HttpSession::ptr session)>;
  FunctionServlet(callback cb);
  virtual int32_t handle(cool::http::HttpRequest::ptr request,
                         cool::http::HttpResponse::ptr response,
                         cool::http::HttpSession::ptr session) override;

private:
  callback m_cb;
};

class ServletDispatch : public Servlet {
public:
  using ptr = std::shared_ptr<ServletDispatch>;
  using RWMutexType = RWMutex;

  ServletDispatch ();
  virtual int32_t handle(cool::http::HttpRequest::ptr request,
                         cool::http::HttpResponse::ptr response,
                         cool::http::HttpSession::ptr session) override;

  void add_servlet(const std::string &uri, Servlet::ptr slt);
  void add_servlet(const std::string &uri, FunctionServlet::callback cb);
  void add_glob_servlet(const std::string &uri, Servlet::ptr slt);
  void add_glob_servlet(const std::string &uri, FunctionServlet::callback cb);

  void del_servlet(const std::string &uri);
  void del_glob_servlet(const std::string &uri);

  Servlet::ptr get_servlet(const std::string &uri);
  Servlet::ptr get_glob_servlet(const std::string &uri);

  Servlet::ptr get_default_servlet() const { return m_default; }
  void set_default_servlet(Servlet::ptr v) { m_default = v; }

  Servlet::ptr get_matched_servlet(const std::string &uri);

private:
  // uri(/xxx) -> servlet
  std::unordered_map<std::string, Servlet::ptr> m_datas;
  // uri(/*) -> servlet
  std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
  // default servlet, 所有路径都没匹配到时使用
  Servlet::ptr m_default;

  RWMutexType m_mutex;
};

class NotFoundServlet : public Servlet {
public:
  using ptr = std::shared_ptr<NotFoundServlet>;
  NotFoundServlet ();
  virtual int32_t handle(cool::http::HttpRequest::ptr request,
                         cool::http::HttpResponse::ptr response,
                         cool::http::HttpSession::ptr session) override;
};

} /* namespace http */
} /* namespace cool */

#endif /* end of include guard: __COOL_HTTP_SERVLET_H */
