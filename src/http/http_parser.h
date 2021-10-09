#ifndef __COOL_HTTP_PARSER_H
#define __COOL_HTTP_PARSER_H

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"
#include <cstdint>

namespace cool {
namespace http {

class HttpRequestParser {
public:
  using ptr = std::shared_ptr<HttpRequestParser>;
  HttpRequestParser();

  size_t execute(char *data, size_t len);
  int isFinished();
  int hasError();

  // HttpRequest::ptr data() const { return m_data; }

  uint64_t content_length();
public:
  static uint64_t GetHttpRequestBufferSize();
  static uint64_t GetHttpRequestMaxBodySize();

public:
  int m_error; // 0: valid, 1000: invalid method, 1001: invalid version, 1002 invalid field
  HttpRequest::ptr m_data;

private:
  http_parser m_parser;
  // HttpRequest::ptr m_data;
  // int m_error;
};

class HttpResponseParser {
public:
  using ptr = std::shared_ptr<HttpResponseParser>;
  HttpResponseParser();

  size_t execute(char *data, size_t len);
  int isFinished();
  int hasError();

  // HttpResponse::ptr data() const { return m_data; }
  uint64_t content_length();

public:
  HttpResponse::ptr m_data;
  int m_error; // 1001: invalid version, 1002: invalid field

private:
  httpclient_parser m_parser;
  // HttpResponse::ptr m_data;
  // int m_error;
};

} /* namespace http */
} /* namespace cool */

#endif /* ifndef __COOL_HTTP_PARSER_H */
