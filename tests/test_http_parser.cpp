#include "src/http/http_parser.h"
#include "src/log.h"
#include <cstddef>
#include <cstdio>
#include <string>

static cool::Logger::ptr g_logger = LOG_ROOT();

const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                 "Host: www.baidu.com\r\n"
                                 "Content-Length: 10\r\n\r\n"
                                 "1234567890";

void test_request() {
  cool::http::HttpRequestParser parser;
  std::string temp = test_request_data;
  size_t s = parser.execute(&temp[0], temp.size());
  LOG_DEBUG(g_logger) << " execute rt = " << s
                      << " hasError = " << parser.hasError()
                      << " isFinished = " << parser.isFinished()
                      << " content-length = " << parser.content_length();
  temp.resize(temp.size() - s);
  LOG_DEBUG(g_logger) << parser.m_data->to_string();
  LOG_DEBUG(g_logger) << temp;
}

const char test_response_data[] =
    "HTTP/1.1 200 OK\r\n"
    "Accept-Ranges: bytes\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 81\r\n"
    "Content-Type: text/html\r\n"
    "Date: Fri, 24 Sep 2021 01:59:54 GMT\r\n\r\n"
    "<html>\r\n"
    "<meta http-equic=\"refresh\"> content=\"0;url=http://www.baidu.com/\">\r\n"
    "</html>\r\n";

void test_response() {
  cool::http::HttpResponseParser parser;
  std::string temp = test_response_data;
  size_t s = parser.execute(&temp[0], temp.size());
  LOG_ERROR(g_logger) << " execute rt = " << s
                      << " hasError = " << parser.hasError()
                      << " isFinished = " << parser.isFinished()
                      << " total = " << temp.size()
                      << " content_length = " << parser.content_length();
  temp.resize(temp.size() - s);

  LOG_DEBUG(g_logger) << parser.m_data->to_string();
  LOG_DEBUG(g_logger) << temp;
}

int main(int argc, char *argv[]) {
  test_request();
  printf("------------------------\n");
  test_response();
  return 0;
}
