#ifndef _COOL_SOCKET_H
#define _COOL_SOCKET_H

#include "address.h"
#include "noncopyable.h"
#include <cstdint>
#include <memory>
#include <ostream>
#include <sys/socket.h>

namespace cool {

class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
  using ptr = std::shared_ptr<Socket>;
  using weak_ptr = std::weak_ptr<Socket>;

  enum TYPE {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM
  };

  enum FAMILY {
    IPv4 = AF_INET,
    IPv6 = AF_INET6,
    Unix = AF_UNIX
  };

  static Socket::ptr CreateTCP(cool::Address::ptr address);
  static Socket::ptr CreateUDP(cool::Address::ptr address);

  static Socket::ptr CreateTCPSocket();
  static Socket::ptr CreateUDPSocket();

  static Socket::ptr CreateTCPSocket6();
  static Socket::ptr CreateUDPSocket6();

  static Socket::ptr CreateUnixTCPSocket();
  static Socket::ptr CreateUnixUDPSocket();

  Socket(int family, int type, int protocol = 0);
  ~Socket();

  int64_t sendTimeout();
  void sendTimeout(int64_t v);

  int64_t recvTimeout();
  void recvTimeout(int64_t v);

  bool getOption(int level, int option, void *res, socklen_t *len);
  template <class T> bool getOption(int level, int option, T &res) {
    size_t len = sizeof(T);
    return getOption(level, option, &res, &len);
  }

  bool setOption(int level, int option, const void *val, socklen_t len);
  template <class T> bool setOption(int level, int option, const T &val) {
    return setOption(level, option, &val, sizeof(T));
  }

  Socket::ptr accept();

  bool init(int sock);
  bool bind(const Address::ptr addr);
  bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
  bool listen(int backlog = SOMAXCONN);
  bool close();

  int send(const void *buf, size_t len, int flags = 0);
  int send(const iovec *buf, size_t len, int flags = 0);
  int sendTo(const void *buf, size_t len, const Address::ptr to, int flags = 0);
  int sendTo(const iovec *buf, size_t len, const Address::ptr to,
             int flags = 0);

  int recv(void *buf, size_t len, int flags = 0);
  int recv(iovec *buf, size_t len, int flags = 0);
  int recvFrom(void *buf, size_t len, Address::ptr from, int flags = 0);
  int recvFrom(iovec *buf, size_t len, Address::ptr from, int flags = 0);

  Address::ptr remoteAddress();
  Address::ptr localAddress();

  int family() const {return m_family;}
  int type() const {return m_type;}
  int protocol() const {return m_protocol;}

  bool isConnected() const {return m_isConnected;}
  bool isValid() const;
  int error();

  std::ostream &dump(std::ostream &os) const;
  int socket() const {return m_sock;}

  bool cancelRead();
  bool cancelWrite();
  bool cancelAccept();
  bool cancelAll();

private:
  void initSock();
  void newSock();

private:
  int m_sock;
  int m_family;
  int m_type;
  int m_protocol;
  bool m_isConnected;

  Address::ptr m_localAddress;
  Address::ptr m_remoteAddress;
};

std::ostream& operator<<(std::ostream& os, const Socket& sock);



} // namespace cool

#endif /* ifndef _COOL_SOCKET_H */
