#include "http_parser.h"
#include "src/config.h"
#include "src/http/http.h"
#include "src/http/http11_parser.h"
#include "src/http/httpclient_parser.h"
#include "src/log.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace cool {
namespace http {

static cool::Logger::ptr g_logger = LOG_NAME("system");
static cool::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    cool::Config::lookup("http.request.buffer_size", 4 * 1024ul,
                         "http request buffer size");
static cool::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    cool::Config::lookup("http.request.max_body_size", 64 * 1024 * 1024ul,
                         "http request max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
  return s_http_request_buffer_size;
}
uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
  return s_http_request_max_body_size;
}

namespace {
struct _RequestSizeIniter {
  _RequestSizeIniter() {
    s_http_request_buffer_size = g_http_request_buffer_size->get_value();
    g_http_request_buffer_size->add_listener(
        [](const uint64_t &ov, const uint64_t &nv) {
          s_http_request_buffer_size = nv;
        });
    s_http_request_max_body_size = g_http_request_max_body_size->get_value();
    g_http_request_max_body_size->add_listener(
        [](const uint64_t &ov, const uint64_t &nv) {
          s_http_request_max_body_size = nv;
        });
  }
};

static _RequestSizeIniter _init;
}

void on_request_method(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpRequestParser *>(data);
  http_method m = chars_to_http_methd(at);

  if (m == http_method::INVALID_METHOD) {
    LOG_WARN(g_logger) << "http_method::INVALID_METHOD, "
                       << std::string(at, length);
    parser->m_error = 1000;
    return;
  }
  parser->m_data->method(m);
}
void on_request_uri(void *data, const char *at, size_t length) {
  // auto parser = static_cast<HttpRequestParser *>(data);
}
void on_request_fragment(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpRequestParser *>(data);
  parser->m_data->fragment(std::string(at, length));
}
void on_request_path(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpRequestParser *>(data);
  parser->m_data->path(std::string(at, length));
}
void on_request_query(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpRequestParser *>(data);
  parser->m_data->query(std::string(at, length));
}
void on_request_version(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpRequestParser *>(data);
  uint8_t v = 0;
  if (strncmp(at, "HTTP/1.1", length) == 0) {
    v = 0x11;
  } else if (strncmp(at, "HTTP/1.0", length) == 0) {
    v = 0x10;
  } else {
    LOG_WARN(g_logger) << "invalid http request version: "
                       << std::string(at, length);
    parser->m_error = 1001;
    return;
  }
  parser->m_data->version(v);
}

void on_request_header_done(void *data, const char *at, size_t length) {}

void on_request_http_field(void *data, const char *field, size_t flen,
                           const char *value, size_t vlen) {
  auto parser = static_cast<HttpRequestParser *>(data);
  if (flen == 0) {
    LOG_WARN(g_logger) << "invalid http request field length = 0";
    // parser->m_error = 1002;
    return;
  }
  parser->m_data->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser() : m_error(0) {
  m_data.reset(new cool::http::HttpRequest);
  http_parser_init(&m_parser);
  m_parser.request_method = on_request_method;
  m_parser.request_uri = on_request_uri;
  m_parser.fragment = on_request_fragment;
  m_parser.request_path = on_request_path;
  m_parser.query_string = on_request_query;
  m_parser.http_version = on_request_version;
  m_parser.header_done = on_request_header_done;
  m_parser.http_field = on_request_http_field;
  m_parser.data = this;
}

uint64_t HttpRequestParser::content_length() {
  return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

size_t HttpRequestParser::execute(char *data, size_t len) {
  size_t offset = http_parser_execute(&m_parser, data, len, 0);
  memmove(data, data + offset, (len - offset));
  return offset;
}

int HttpRequestParser::isFinished() { return http_parser_finish(&m_parser); }

int HttpRequestParser::hasError() {
  return m_error || http_parser_has_error(&m_parser);
}

void on_response_reason(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpResponseParser *>(data);
  parser->m_data->reason(std::string(at, length));
}

void on_response_status(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpResponseParser *>(data);
  parser->m_data->status((http_status)atoi(at));
}

void on_response_chunk(void *data, const char *at, size_t length) {
  // auto parser = static_cast<HttpResponseParser*>(data);
}

void on_response_version(void *data, const char *at, size_t length) {
  auto parser = static_cast<HttpResponseParser *>(data);
  uint8_t v = 0;
  if (strncmp(at, "HTTP/1.1", length) == 0) {
    v = 0x11;
  } else if (strncmp(at, "HTTP/1.0", length) == 0) {
    v = 0x10;
  } else {
    LOG_WARN(g_logger) << "invalid http response version: "
                       << std::string(at, length);
    parser->m_error = 1001;
    return;
  }
  parser->m_data->version(v);
}

void on_response_header_done(void *data, const char *at, size_t length) {}

void on_response_last_chunk(void *data, const char *at, size_t length) {}

void on_response_http_field(void *data, const char *field, size_t flen,
                            const char *value, size_t vlen) {
  auto parser = static_cast<HttpResponseParser *>(data);
  if (flen == 0) {
    LOG_WARN(g_logger) << "invalid http response field length = 0";
    parser->m_error = 1002;
    return;
  }
  parser->m_data->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser() : m_error(0) {
  m_data.reset(new cool::http::HttpResponse);
  httpclient_parser_init(&m_parser);
  m_parser.reason_phrase = on_response_reason;
  m_parser.status_code = on_response_status;
  m_parser.chunk_size = on_response_chunk;
  m_parser.http_version = on_response_version;
  m_parser.header_done = on_response_header_done;
  m_parser.last_chunk = on_response_last_chunk;
  m_parser.http_field = on_response_http_field;
  m_parser.data = this;
}

uint64_t HttpResponseParser::content_length() {
  return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

size_t HttpResponseParser::execute(char *data, size_t len) {
  size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
  memmove(data, data + offset, (len - offset));
  return offset;
}

int HttpResponseParser::isFinished() {
  return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::hasError() {
  return m_error || httpclient_parser_has_error(&m_parser);
}

} /* namespace http */
} /* namespace cool */
