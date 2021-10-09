#include "src/socket.h"
#include "src/log.h"
#include "src/address.h"
#include "src/iomanager.h"
#include <string>

static cool::Logger::ptr g_logger = LOG_ROOT();

void test_socket() {
  cool::IPAddress::ptr addr = cool::Address::LookupAnyIPAddress("www.baidu.com");
  if (addr) {
    LOG_DEBUG(g_logger) << "get address: " << addr->to_string();
  } else {
    LOG_ERROR(g_logger) << "get address fail";
    return;
  }

  cool::Socket::ptr sock = cool::Socket::CreateTCP(addr);
  addr->port(80);
  if (!sock->connect(addr)) {
    LOG_ERROR(g_logger) << "connect " << addr->to_string() << " fail";
    return;
  } else {
    LOG_DEBUG(g_logger) << "connect " << addr->to_string() << " success";
  }

  const char buf[] = "GET / HTTP/1.0\r\n\r\n";
  int rt = sock->send(buf, sizeof(buf));
  if (rt <= 0) {
    LOG_ERROR(g_logger) << "send fail";
    return;
  }
  std::string res;
  res.resize(4096);
  rt = sock->recv(&res[0], res.size());

  if (rt <= 0) {
    LOG_ERROR(g_logger) << "recv fail";
    return;
  }

  res.resize(rt);
  LOG_DEBUG(g_logger) << std::endl  << res;
}

int main(int argc, char* argv[]) {
  cool::IOManager iom;
  iom.schedule(&test_socket);
  return 0;
}
