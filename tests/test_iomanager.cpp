#include "src/cool.h"
#include "src/iomanager.h"
#include "src/log.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

cool::Logger::ptr g_logger = LOG_ROOT();

void test_timer();
void test_fiber();
void test1();
int main(int argc, char *argv[]) {
  // test1();
  test_timer();
  return 0;
}

cool::Timer::ptr timer;
void test_timer() {
  cool::IOManager iom(2);
  timer = iom.addTimer(
      1000,
      []() {
        static int i = 0;
        LOG_DEBUG(g_logger) << "hello timer i = " << i;
        if (++i == 3) {
          timer->reset(2000, true);
          // timer->cancel();
        }
      },
      true);
}

void test_fiber() {
  LOG_DEBUG(g_logger) << "test fiber";
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, "39.156.66.18", &addr.sin_addr.s_addr);

  int rt = connect(sock, (const sockaddr *)&addr, sizeof(addr));
  if (rt < 0 && errno == EINPROGRESS) {
    // cool::IOManager::GetThis()->addEvent(sock, cool::IOManager::READ,
    //              []() { LOG_DEBUG(g_logger) << "read callback"; });
    cool::IOManager::GetThis()->addEvent(
        sock, cool::IOManager::WRITE, [sock]() {
          LOG_DEBUG(g_logger) << "write callback";
          // cool::IOManager::GetThis()->cancelEvent(sock,
          // cool::IOManager::READ);
          close(sock);
        });
  } else if (rt < 0) {
    perror("connect");
    // exit(1);
  }
}
void test1() {
  // cool::IOManager iom;
  cool::IOManager iom{2, false};
  iom.schedule(&test_fiber);
}
