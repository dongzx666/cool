#include "src/address.h"
#include "src/config.h"
#include "src/iomanager.h"
#include "src/log.h"
#include "src/socket.h"
#include "tcp_server.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>

namespace cool {
static cool::Logger::ptr g_logger = LOG_NAME("system");

cool_config(g_tcp_server_read_timeout, "tcp_server.read_timeout",
            (uint64_t)60 * 1000 * 2, "tcp server read timeout");

TcpServer::TcpServer(cool::IOManager *worker, cool::IOManager *accept_worker)
    : m_recv_timeout(g_tcp_server_read_timeout->value()), m_name("cool v1.0.0"), m_worker(worker),
      m_accept_worker(accept_worker), m_is_stop(false), m_is_start(false) {}

bool TcpServer::bind(cool::Address::ptr addr) {
  std::vector<Address::ptr> addrs;
  addrs.push_back(addr);
  std::vector<Address::ptr> fails;
  return bind(addrs, fails);
}

TcpServer::~TcpServer() {
  for (auto &i : m_socks) {
    i->close();
  }
  m_socks.clear();
}

bool TcpServer::bind(const std::vector<Address::ptr> &addrs,
                     std::vector<Address::ptr> &fails) {
  for (auto &addr : addrs) {
    Socket::ptr sock = Socket::CreateTCP(addr);
    if (!sock->bind(addr)) {
      LOG_ERROR(g_logger) << "bind fail, errstr = " << strerror(errno)
                          << " addr is " << addr->to_string();
      fails.push_back(addr);
      continue;
    }
    if (!sock->listen()) {
      LOG_ERROR(g_logger) << "listen fail strerr = " << strerror(errno)
                          << " addr is " << addr->to_string();
      fails.push_back(addr);
      continue;
    }
    m_socks.push_back(sock);
  }
  if (!fails.empty()) {
    m_socks.clear();
    return false;
  }
  for (auto &i : m_socks) {
    LOG_INFO(g_logger) << "server bind success: " << *i;
  }
  return true;
}

void TcpServer::start_accept(Socket::ptr sock) {
  while (!m_is_stop) {
    Socket::ptr client = sock->accept();
    if (client) {
      client->recvTimeout(m_recv_timeout);
      m_worker->schedule(
          std::bind(&TcpServer::handle_client, shared_from_this(), client));
    } else {
      LOG_ERROR(g_logger) << "accept error, strerr is " << strerror(errno);
    }
  }
}

bool TcpServer::start() {
  if (m_is_start) {
    return true;
  }
  m_is_start = true;
  // if (!m_is_stop) {
  //   return true;
  // }
  // m_is_stop = false;
  for (auto &sock : m_socks) {
    m_accept_worker->schedule(
        std::bind(&TcpServer::start_accept, shared_from_this(), sock));
  }
  return true;
}

void TcpServer::stop() {
  if (m_is_stop) {
    return ;
  }
  m_is_stop = true;
  auto self = shared_from_this();
  m_accept_worker->schedule([this, self]() {
    for (auto &sock : m_socks) {
      sock->cancelAll();
      sock->close();
    }
    m_socks.clear();
  });
}

void TcpServer::handle_client(Socket::ptr client) {
  LOG_INFO(g_logger) << "handle client: " << *client;
}

} /* namespace cool */
