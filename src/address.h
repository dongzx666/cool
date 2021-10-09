#ifndef __COOL_ADDRESS_H
#define __COOL_ADDRESS_H

#include <cstdint>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <utility>
#include <vector>

namespace cool {
class IPAddress;
class Address {
public:
  using ptr = std::shared_ptr<Address>;
  virtual ~Address() {}

  static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

  static bool Lookup(std::vector<Address::ptr>& res, const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);
  static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);
  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

  static bool GetInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& res, int family = AF_UNSPEC);
  static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>>& res, const std::string& iface, int family = AF_UNSPEC);


  int family() const;
  virtual const sockaddr *addr() const = 0;
  virtual sockaddr *addr() = 0;
  virtual socklen_t addrlen() const = 0;
  virtual std::ostream &insert(std::ostream &os) const = 0;
  std::string to_string() const;

  bool operator<(const Address &rhs) const;
  bool operator==(const Address &rhs) const;
  bool operator!=(const Address &rhs) const;
};

class IPAddress : public Address {
public:
  using ptr = std::shared_ptr<IPAddress>;

  static IPAddress::ptr Create(const char* address, uint16_t port = 0);

  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

  virtual uint16_t port() const = 0;
  virtual void port(uint16_t v) = 0;
};

class IPv4Address : public IPAddress {
public:
  using ptr = std::shared_ptr<IPv4Address>;

  static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

  IPv4Address(const sockaddr_in& address);
  IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  const sockaddr *addr() const override;
  sockaddr *addr() override;
  socklen_t addrlen() const override;
  std::ostream &insert(std::ostream &os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  uint16_t port() const override;
  void port(uint16_t v) override;
private:
  sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
  using ptr = std::shared_ptr<IPv6Address>;
  static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

  IPv6Address();
  IPv6Address(const sockaddr_in6& address);
  IPv6Address(const uint8_t address[16], uint16_t port);

  const sockaddr *addr() const override;
  sockaddr *addr() override;
  socklen_t addrlen() const override;
  std::ostream &insert(std::ostream &os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  uint16_t port() const override;
  void port(uint16_t v) override;
private:
  sockaddr_in6 m_addr;
};

class UnixAddress : public Address {
public:
  using ptr = std::shared_ptr<UnixAddress>;
  UnixAddress();
  UnixAddress(const std::string& path);

  const sockaddr *addr() const override;
  sockaddr *addr() override;
  socklen_t addrlen() const override;
  void addrlen(uint32_t v);
  std::ostream &insert(std::ostream &os) const override;
private:
  struct sockaddr_un m_addr;
  socklen_t m_len;
};

class UnknowAddress : public Address {
public:
  using ptr = std::shared_ptr<UnknowAddress>;
  UnknowAddress(int family);
  UnknowAddress(const sockaddr& addr);
  const sockaddr *addr() const override;
  sockaddr *addr() override;
  socklen_t addrlen() const override;
  std::ostream &insert(std::ostream &os) const override;
private:
  sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

} // namespace cool

#endif /* ifndef __COOL_ADDRESS_H */
