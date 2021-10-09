#include "src/address.h"
#include "src/tcp_server.h"
#include "src/log.h"
#include "src/iomanager.h"
#include <unistd.h>
#include <vector>

LOGGER_DEF(g_logger, "root");

void run () {
  auto addr = cool::Address::LookupAny("0.0.0.0:80");
  // LOG_DEBUG(g_logger) << *addr;
  std::vector<cool::Address::ptr> addrs;
  addrs.push_back(addr);

  cool::TcpServer::ptr tcp_server(new cool::TcpServer());
  std::vector<cool::Address::ptr> fails;
  while (!tcp_server->bind(addrs, fails)) {
    sleep(2);
  }
  tcp_server->start();
}

// void http_session() {

// }

int main(int argc, char* argv[]) {
  cool::IOManager iom(2);
  iom.schedule(run);
  return 0;
}
