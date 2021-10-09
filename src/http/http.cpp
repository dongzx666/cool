#include "http.h"
#include <cstdint>
#include <cstring>
#include <sstream>
#include <strings.h>

namespace cool {
namespace http {

http_method string_to_http_method(const std::string &v) {
#define XX(num, name, string)            \
  if (strcmp(#string, v.c_str()) == 0) { \
    return http_method::name;            \
  }
  HTTP_METHOD_MAP(XX)
#undef XX
  return http_method::INVALID_METHOD;
}
http_method chars_to_http_methd(const char *v) {
#define XX(num, name, string)                      \
  if (strncmp(#string, v, strlen(#string)) == 0) { \
    return http_method::name;                      \
  }
  HTTP_METHOD_MAP(XX)
#undef XX
  return http_method::INVALID_METHOD;
}

static const char *s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};
const char *http_method_to_chars(const http_method &v) {
  uint32_t index = (uint32_t)v;
  if (index >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
    return "<unknow>";
  }
  return s_method_string[index];
}
const char *http_status_to_chars(const http_status &v) {
  switch (v) {
#define XX(code, name, msg) \
  case http_status::name:   \
    return #msg;
    HTTP_STATUS_MAP(XX);
#undef XX
  default:
    return "<unknow>";
  }
}

bool CaseInsensitiveLess::operator()(const std::string &lhs,
                                     const std::string &rhs) const {
  return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool close)
    : m_method(http_method::GET), m_version(version), m_close(close),
      m_path("/") {}

std::string HttpRequest::getHeader(const std::string &key,
                                   const std::string &def) const {
  auto it = m_headers.find(key);
  return it == m_headers.end() ? def : it->second;
}
std::string HttpRequest::getParam(const std::string &key,
                                  const std::string &def) const {
  auto it = m_params.find(key);
  return it == m_params.end() ? def : it->second;
}
std::string HttpRequest::getCookie(const std::string &key,
                                   const std::string &def) const {
  auto it = m_cookies.find(key);
  return it == m_cookies.end() ? def : it->second;
}

void HttpRequest::setHeader(const std::string &key, const std::string &val) {
  m_headers[key] = val;
}
void HttpRequest::setParam(const std::string &key, const std::string &val) {
  m_params[key] = val;
}
void HttpRequest::setCookie(const std::string &key, const std::string &val) {
  m_cookies[key] = val;
}

void HttpRequest::delHeader(const std::string &key) { m_headers.erase(key); }
void HttpRequest::delParam(const std::string &key) { m_params.erase(key); }
void HttpRequest::delCookie(const std::string &key) { m_params.erase(key); }

bool HttpRequest::hasHeader(const std::string &key, std::string *val) {
  auto it = m_headers.find(key);
  if (it == m_headers.end()) {
    // val = nullptr;
    return false;
  } else {
    *val = it->second;
    return true;
  }
}
bool HttpRequest::hasParam(const std::string &key, std::string *val) {
  auto it = m_params.find(key);
  if (it == m_params.end()) {
    return false;
  } else {
    *val = it->second;
    return true;
  }
}
bool HttpRequest::hasCookie(const std::string &key, std::string *val) {
  auto it = m_cookies.find(key);
  if (it == m_cookies.end()) {
    return false;
  } else {
    *val = it->second;
    return true;
  }
}

std::string HttpRequest::to_string() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

std::ostream &HttpRequest::dump(std::ostream &os) const {
  // GET /uri HTTP/1.1
  // Host: www.baidu.com
  os << http_method_to_chars(m_method) << " " << m_path
     << (m_query.empty() ? "" : "?") << m_query
     << (m_fragment.empty() ? "" : "#") << m_fragment << " HTTP/"
     << ((uint32_t)(m_version >> 4)) << "." << ((uint32_t)(m_version & 0x0F))
     << "\r\n";
  os << "Connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
  for (auto &i : m_headers) {
    if (strcasecmp(i.first.c_str(), "Connection") == 0) {
      continue;
    }
    os << i.first << ":" << i.second << "\r\n";
  }
  if (!m_body.empty()) {
    os << "Content-Length: " << m_body.size() << "\r\n\r\n" << m_body;
  } else {
    os << "\r\n";
  }
  return os;
}

HttpResponse::HttpResponse(uint8_t version, bool close)
    : m_status(http_status::OK), m_version(version), m_close(close) {}

std::string HttpResponse::getHeader(const std::string &key,
                                    const std::string &def) const {
  auto it = m_headers.find(key);
  return it == m_headers.end() ? def : it->second;
}

void HttpResponse::setHeader(const std::string &key, const std::string &val) {
  m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string &key) { m_headers.erase(key); }

std::ostream &HttpResponse::dump(std::ostream &os) const {
  os << "HTTP/" << ((uint32_t)(m_version >> 4)) << "."
     << ((uint32_t)(m_version & 0x0F)) << " " << (uint32_t)m_status << " "
     << (m_reason.empty() ? http_status_to_chars(m_status) : m_reason)
     << "\r\n";
  for (auto &i : m_headers) {
    if (strcasecmp(i.first.c_str(), "Connection") == 0) {
      continue;
    }
    os << i.first << ": " << i.second << "\r\n";
  }
  os << "Connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
  if (!m_body.empty()) {
    os << "Content-Length: " << m_body.size() << "\r\n\r\n" << m_body;
  } else {
    os << "\r\n";
  }
  return os;
}

std::string HttpResponse::to_string() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
  return req.dump(os);
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) {
  return rsp.dump(os);
}

} // namespace http
} // namespace cool
