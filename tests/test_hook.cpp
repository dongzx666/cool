#include "src/fiber.h"
#include "src/hook.h"
#include "src/iomanager.h"
#include "src/log.h"
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

cool::Logger::ptr g_logger = LOG_ROOT();

void test_sleep();
void test_sock();
int main(int argc, char *argv[]) {
  // test_sleep();
  // test_sock();
  cool::IOManager iom;
  iom.schedule(test_sock);
  return 0;
}

void test_sock() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, "110.242.68.4", &addr.sin_addr.s_addr);

  // LOG_DEBUG(g_logger) << "connect start";
  int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
  LOG_DEBUG(g_logger) << "connect rt=" << rt << " errno=" << strerror(errno);
  if (rt) {
    return;
  }

  const char data[] = "GET / HTTP/1.0\r\n\r\n";
  rt = send(sock, data, sizeof(data), 0);
  LOG_DEBUG(g_logger) << "send rt=" << rt << " errno=" << strerror(errno);

  if (rt <= 0) {
    return;
  }

  std::string buff;
  buff.resize(4096);
  rt = recv(sock, &buff[0], buff.size(), 0);
  LOG_DEBUG(g_logger) << "recv rt=" << rt << " errno=" << strerror(errno);

  if (rt <= 0) {
    return;
  }
  buff.resize(rt);
  LOG_DEBUG(g_logger) << buff;

}

void test_sleep() {
  cool::IOManager iom(1);
  iom.schedule([]() {
    sleep(2);
    LOG_DEBUG(g_logger) << "sleep 2";
  });
  iom.schedule([]() {
    sleep(3);
    LOG_DEBUG(g_logger) << "sleep 3";
  });
  LOG_DEBUG(g_logger) << "test_sleep over";
}
