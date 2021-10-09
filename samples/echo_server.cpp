#include "src/bytearray.h"
#include "src/iomanager.h"
#include "src/log.h"
#include "src/tcp_server.h"
#include <cstring>
#include <iostream>
#include <string>

LOGGER_DEF(g_logger, "root");

class EchoServer : public cool::TcpServer {
public:
  EchoServer(int type);
  void handle_client(cool::Socket::ptr client);

private:
  int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type) {}

void EchoServer::handle_client(cool::Socket::ptr client) {
  LOG_INFO(g_logger) << "handle client " << *client;
  cool::ByteArray::ptr ba(new cool::ByteArray);
  while (true) {
    ba->clear();
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, 1024);

    int rt = client->recv(&iovs[0], iovs.size());
    if (rt == 0) {
      LOG_INFO(g_logger) << "client close: " << *client;
      break;
    } else if (rt < 0) {
      LOG_INFO(g_logger) << "client errno is " << rt << " strerr is "
                         << strerror(rt);
      break;
    }
    // // TODO(fengyu): position out of range 会卡死 [27-09-21] //
    ba->position(ba->position() + rt);
    ba->position(0);
    LOG_INFO(g_logger) << "recv size is " << rt;
    // LOG_INFO(g_logger) << "recv rt is " << rt << " data is "
    //                    << std::string((char *)iovs[0].iov_base, rt);
    if (m_type == 1) { // text
      std::cout << ba->to_string() << std::endl;
    } else {
      std::cout << ba->to_hex_string() << std::endl;
    }
  }
}

int type = 1;

void run() {
  LOG_DEBUG(g_logger) << "type is " << type;
  EchoServer::ptr es(new EchoServer(type));
  auto addr = cool::Address::LookupAny("0.0.0.0:8020");
  while (!es->bind(addr)) {
    sleep(2);
  }
  es->start();
}

int main(int argc, char *argv[]) {
  // if (argc < 2) {
  //   LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0]
  //                      << " -b]";
  //   return 0;
  // }
  // if (!strcmp(argv[1], "-b")) {
  //   type = 2;
  // }
  cool::IOManager iom(2);
  iom.schedule(run);
  return 0;
}
