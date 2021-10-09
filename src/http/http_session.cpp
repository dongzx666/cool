#include "http_parser.h"
#include "http_session.h"
#include "src/log.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

namespace cool {
LOGGER_DEF(g_logger, "system");
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {}

HttpRequest::ptr HttpSession::recvRequest() {
  HttpRequestParser::ptr parser(new HttpRequestParser);
  uint64_t buf_size = HttpRequestParser::GetHttpRequestBufferSize();
  // uint64_t buf_size = 100;

  std::shared_ptr<char> buffer(new char[buf_size],
                               [](char *ptr) { delete[] ptr; });
  char *data = buffer.get();
  int offset = 0;
  do {
    int len = read(data + offset, buf_size - offset);
    if (len <= 0) {
      close();
      LOG_DEBUG(g_logger) << "read len <= 0";
      return nullptr;
    }
    len += offset;
    size_t nparse = parser->execute(data, len);
    if (parser->hasError()) {
      close();
      LOG_DEBUG(g_logger) << "parser execute haserror";
      return nullptr;
    }
    offset = len - nparse;
    if (offset == (int)buf_size) {
      close();
      LOG_DEBUG(g_logger) << "offset == (int)buf_size";
      return nullptr;
    }
    if (parser->isFinished()) {
      break;
    }
  } while (true);
  int64_t length = parser->content_length();
  if (length > 0) {
    std::string body;
    body.resize(length);
    if (length >= offset) {
      body.append(data, offset);
    } else {
      body.append(data, length);
    }
    length -= offset;

    if (length > 0) {
      if (readFixSize(&body[body.size()], length) <= 0) {
        close();
        LOG_DEBUG(g_logger)
            << "readFixSize(&body[body.size()], length) <= 0";
        return nullptr;
      }
    }
    parser->m_data->body(body);
  }
  return parser->m_data;
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize(data.c_str(), data.size());
}

} /* namespace http */
} /* namespace cool */
