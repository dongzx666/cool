#include "socket.h"
#include "fd_manager.h"
#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include "macro.h"
#include <cerrno>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <time.h>

namespace cool {

static cool::Logger::ptr g_logger = LOG_NAME("system");

Socket::ptr Socket::CreateTCP(cool::Address::ptr address) {
  Socket::ptr sock(new Socket(address->family(), TYPE::TCP, 0));
  return sock;
}
Socket::ptr Socket::CreateUDP(cool::Address::ptr address) {
  Socket::ptr sock(new Socket(address->family(), TYPE::UDP, 0));
  return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
  Socket::ptr sock(new Socket(FAMILY::IPv4, TYPE::TCP, 0));
  return sock;
}
Socket::ptr Socket::CreateUDPSocket() {
  Socket::ptr sock(new Socket(FAMILY::IPv4, TYPE::UDP, 0));
  return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
  Socket::ptr sock(new Socket(FAMILY::IPv6, TYPE::TCP, 0));
  return sock;
}
Socket::ptr Socket::CreateUDPSocket6() {
  Socket::ptr sock(new Socket(FAMILY::IPv6, TYPE::UDP, 0));
  return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket() {
  Socket::ptr sock(new Socket(FAMILY::Unix, TYPE::TCP, 0));
  return sock;
}
Socket::ptr Socket::CreateUnixUDPSocket() {
  Socket::ptr sock(new Socket(FAMILY::Unix, TYPE::UDP, 0));
  return sock;
}

Socket::Socket(int family, int type, int protocol)
    :m_sock(-1), m_family(family), m_type(type), m_protocol(protocol),
      m_isConnected(false) {}
Socket::~Socket() { close(); }

int64_t Socket::sendTimeout() {
  FdCtx::ptr ctx = FdMgr::instance()->get(m_sock);
  if (ctx) {
    return ctx->getTimeout(SO_SNDTIMEO);
  }
  return -1;
}

void Socket::sendTimeout(int64_t v) {
  struct timeval tv {
    (int)(v / 1000), (int)(v % 1000 * 1000)
  };
  setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::recvTimeout() {
  FdCtx::ptr ctx = FdMgr::instance()->get(m_sock);
  if (ctx) {
    return ctx->getTimeout(SO_RCVTIMEO);
  }
  return -1;
}
void Socket::recvTimeout(int64_t v) {
  struct timeval tv {
    (int)(v / 1000), (int)(v % 1000 * 1000)
  };
  setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void *res, socklen_t *len) {
  int rt = getsockopt(m_sock, level, option, res, (socklen_t *)len);
  if (rt) {
    LOG_DEBUG(g_logger) << "getoption";
    return false;
  }
  return true;
}

bool Socket::setOption(int level, int option, const void *val, socklen_t len) {
  if (setsockopt(m_sock, level, option, val, (socklen_t)len)) {
    LOG_DEBUG(g_logger) << "setoption";
    return false;
  }
  return true;
}

Socket::ptr Socket::accept() {
  Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
  int newsock = ::accept(m_sock, nullptr, nullptr);
  if (newsock == -1) {
    LOG_ERROR(g_logger) << "accept, sockfd: " << m_sock
                        << ", errno: " << strerror(errno);
    return nullptr;
  }
  if (sock->init(newsock)) {
    return sock;
  }
  return nullptr;
}

bool Socket::init(int sock) {
  FdCtx::ptr ctx = FdMgr::instance()->get(sock);
  if (ctx && ctx->isSocket() && !ctx->isClose()) {
    m_sock = sock;
    m_isConnected = true;
    initSock();
    localAddress();
    remoteAddress();
    return true;
  }
  return false;
}

bool Socket::bind(const Address::ptr addr) {
  if (!isValid()) {
    newSock();
    if (UNLIKEY(!isValid())) {
      return false;
    }
  }
  if (UNLIKEY(addr->family() != m_family)) {
    LOG_ERROR(g_logger) << "bind sock family" << addr->family()
                        << "not equl to " << m_family;
    return false;
  }
  if (::bind(m_sock, addr->addr(), addr->addrlen())) {
    LOG_ERROR(g_logger) << "bind error, errno is" << strerror(errno);
    return false;
  }
  localAddress();
  return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
  // LOG_DEBUG(g_logger) << 1;
  if (!isValid()) {
    newSock();
    if (UNLIKEY(!isValid())) {
      return false;
    }
  }
  // LOG_DEBUG(g_logger) << 2;
  if (UNLIKEY(addr->family() != m_family)) {
    LOG_ERROR(g_logger) << "connect sock family" << addr->family()
                        << "not equl to " << m_family;
    return false;
  }
  // LOG_DEBUG(g_logger) << 3;
  if (timeout_ms == (uint64_t)-1) {
    if (::connect(m_sock, addr->addr(), addr->addrlen())) {
      LOG_ERROR(g_logger) << "sock " << m_sock << " connect "
                          << addr->to_string() << ", errstr "
                          << strerror(errno);
      close();
      return false;
    }
  } else {
    if (::connect_with_timeout(m_sock, addr->addr(), addr->addrlen(),
                               timeout_ms)) {
      LOG_ERROR(g_logger) << "sock " << m_sock << " connect "
                          << addr->to_string() << ", errstr "
                          << strerror(errno);
      close();
      return false;
    }
  }
  // LOG_DEBUG(g_logger) << 4;
  m_isConnected = true;
  remoteAddress();
  localAddress();
  return true;
}

bool Socket::listen(int backlog) {
  if (!isValid()) {
    LOG_ERROR(g_logger) << "listen errr sock=-1";
  }
  if (::listen(m_sock, backlog)) {
    LOG_ERROR(g_logger) << "listren error, errstr is" << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::close() {
  if (!m_isConnected && m_sock == -1) {
    return true;
  }
  m_isConnected = true;
  if (m_sock != -1) {
    ::close(m_sock);
    m_sock = -1;
  }
  return false;
}

int Socket::send(const void *buf, size_t len, int flags) {
  if (isConnected()) {
    return ::send(m_sock, buf, len, flags);
  }
  return -1;
}
int Socket::send(const iovec *buf, size_t len, int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buf;
    msg.msg_iovlen = len;
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}
// TODO(fengyu): you danmu shuo chang yong yu UDP [09-09-21] //
int Socket::sendTo(const void *buf, size_t len, const Address::ptr to,
                   int flags) {
  if (isConnected()) {
    return ::sendto(m_sock, buf, len, flags, to->addr(), to->addrlen());
  }
  return -1;
}
int Socket::sendTo(const iovec *buf, size_t len, const Address::ptr to,
                   int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buf;
    msg.msg_iovlen = len;
    msg.msg_name = to->addr();
    msg.msg_namelen = to->addrlen();
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::recv(void *buf, size_t len, int flags) {
  if (isConnected()) {
    return ::recv(m_sock, buf, len, flags);
  }
  return -1;
}
int Socket::recv(iovec *buf, size_t len, int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buf;
    msg.msg_iovlen = len;
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}
int Socket::recvFrom(void *buf, size_t len, Address::ptr from, int flags) {
  if (isConnected()) {
    socklen_t len = from->addrlen();
    return ::recvfrom(m_sock, buf, len, flags, from->addr(), &len);
  }
  return -1;
}
int Socket::recvFrom(iovec *buf, size_t len, Address::ptr from, int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buf;
    msg.msg_iovlen = len;
    msg.msg_name = from->addr();
    msg.msg_namelen = from->addrlen();
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}

Address::ptr Socket::remoteAddress() {
  if (m_remoteAddress) {
    return m_remoteAddress;
  }
  Address::ptr result;
  switch (m_family) {
  case AF_INET:
    result.reset(new IPv4Address());
    break;
  case AF_INET6:
    result.reset(new IPv6Address());
    break;
  case AF_UNIX:
    result.reset(new UnixAddress());
    break;
  default:
    result.reset(new UnknowAddress(m_family));
    break;
  }
  socklen_t addrlen = result->addrlen();
  if (getpeername(m_sock, result->addr(), &addrlen)) {
    LOG_ERROR(g_logger) << "getpeername error, errstr is" << strerror(errno);
    return Address::ptr(new UnknowAddress(m_family));
  }
  if (m_family == AF_UNIX) {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->addrlen(addrlen);
  }
  m_remoteAddress = result;
  return m_remoteAddress;
}

Address::ptr Socket::localAddress() {
  if (m_localAddress) {
    return m_localAddress;
  }
  Address::ptr result;
  switch (m_family) {
  case AF_INET:
    result.reset(new IPv4Address());
    break;
  case AF_INET6:
    result.reset(new IPv6Address());
    break;
  case AF_UNIX:
    result.reset(new UnixAddress());
    break;
  default:
    result.reset(new UnknowAddress(m_family));
    break;
  }
  socklen_t addrlen = result->addrlen();
  if (getsockname(m_sock, result->addr(), &addrlen)) {
    LOG_ERROR(g_logger) << "getsockname error, errstr is" << strerror(errno);
    return Address::ptr(new UnknowAddress(m_family));
  }
  if (m_family == AF_UNIX) {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->addrlen(addrlen);
  }
  m_localAddress = result;
  return m_localAddress;
}

bool Socket::isValid() const { return m_sock != -1; }

int Socket::error() {
  int error = 0;
  socklen_t len = sizeof(error);
  if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  return error;
}

std::ostream &Socket::dump(std::ostream &os) const {
  os << "[Socket sock: " << m_sock << ", isConnected: " << m_isConnected
     << ", family: " << m_family << ", type: " << m_type
     << ", protocol: " << m_protocol;
  if (m_localAddress) {
    os << ", localAddress: " << m_localAddress->to_string();
  }
  if (m_remoteAddress) {
    os << ", remoteAddress " << m_remoteAddress->to_string();
  }
  os << "]";
  return os;
}

bool Socket::cancelRead() {
  return IOManager::GetThis()->cancelEvent(m_sock, cool::IOManager::READ);
}
bool Socket::cancelWrite() {
  return IOManager::GetThis()->cancelEvent(m_sock, cool::IOManager::WRITE);
}
bool Socket::cancelAccept() {
  return IOManager::GetThis()->cancelEvent(m_sock, cool::IOManager::READ);
}
bool Socket::cancelAll() { return IOManager::GetThis()->cancekAll(m_sock); }

void Socket::initSock() {
  int val = 1;
  setOption(SOL_SOCKET, SO_REUSEADDR, val);
  if (m_type == SOCK_STREAM) {
    setOption(IPPROTO_TCP, TCP_NODELAY, val);
  }
}
void Socket::newSock() {
  m_sock = ::socket(m_family, m_type, m_protocol);
  if (LIKEY(m_sock != -1)) {
    initSock();
  } else {
    LOG_ERROR(g_logger) << "newsock error" << m_family << ", " << m_type << ", "
                        << m_protocol;
  }
}

std::ostream& operator<<(std::ostream& os, const Socket& sock) {
  return sock.dump(os);
}

} // namespace cool
