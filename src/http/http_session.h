#ifndef __COOL_HTTP_SESSION_H
#define __COOL_HTTP_SESSION_H

#include "http.h"
#include "src/socket_stream.h"
#include <memory>

namespace cool {
namespace http {

class HttpSession : public SocketStream {
public:
  using ptr = std::shared_ptr<HttpSession>;
  HttpSession(Socket::ptr sock, bool owner = true);
  HttpRequest::ptr recvRequest();
  int sendResponse(HttpResponse::ptr rsp);

private:
};

} /* namespace http */

} /* namespace cool */

#endif /* ifndef __COOL_HTTP_SESSION_H */
