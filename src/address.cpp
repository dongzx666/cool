#include "address.h"
#include "endian.h"
#include "src/log.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ifaddrs.h>
#include <map>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace cool {

static cool::Logger::ptr g_logger = LOG_NAME("system");

template <class T> static T CreateMask(uint32_t bits) {
  return (1 << (sizeof(T) * 8 - bits)) - 1;
}

template <class T> static uint32_t CountBytes(T value) {
  uint32_t result = 0;
  for (; value; ++result) {
    value &= value - 1;
  }
  return result;
}

Address::ptr Address::LookupAny(const std::string &host, int family, int type,
                                int protocol) {
  std::vector<Address::ptr> res;
  if (Lookup(res, host, family, type, protocol)) {
    return res[0];
  }
  return nullptr;
}
IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family,
                                           int type, int protocol) {
  std::vector<Address::ptr> res;
  // TODO(fengyu): family or AF_INET [09-09-21] //
  if (Lookup(res, host, family, type, protocol)) {
    for (auto &i : res) {
      IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
      if (v) {
        return v;
      }
    }
  }
  return nullptr;
}
bool Address::Lookup(std::vector<Address::ptr> &res, const std::string &host,
                     int family, int type, int protocol) {
  addrinfo hints, *results, *next;
  hints.ai_flags = 0;
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_protocol = protocol;
  hints.ai_addrlen = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  std::string node;
  const char *service = NULL;

  // check ipv6address service
  if (!host.empty() && host[0] == '[') {
    const char *endipv6 =
        (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);
    if (endipv6) {
      // TODO(fengyu): check out of range [08-09-21] //
      if (*(endipv6 + 1) == ':') {
        service = endipv6 + 2;
      }
      node = host.substr(1, endipv6 - host.c_str() - 1);
    }
  }
  // check node service
  if (node.empty()) {
    service = (const char *)memchr(host.c_str(), ':', host.size());
    if (service) {
      if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
        node = host.substr(0, service - host.c_str());
        ++service;
      }
    }
  }
  if (node.empty()) {
    node = host;
  }
  int error = getaddrinfo(node.c_str(), service, &hints, &results);
  if (error) {
    std::string strerr;
    if (error == EAI_SYSTEM) {
      strerr = strerror(errno);
    } else {
      strerr = gai_strerror(error);
    }
    LOG_ERROR(g_logger) << "address::lookup getaddrinfo host is " << host << " , family is "
                        << family << " , type is " << type
                        << " error_msg is " << strerr;
    return false;
  }
  next = results;
  while (next) {
    res.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
    next = next->ai_next;
  }
  freeaddrinfo(results);
  return true;
}
bool Address::GetInterfaceAddress(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &res,
    int family) {
  struct ifaddrs *next, *results;
  if (getifaddrs(&results) != 0) {
    LOG_ERROR(g_logger) << "address::getinterfaceaddress getifaddrs errno:"
                        << strerror(errno);
    return false;
  }
  try {
    for (next = results; next; next = next->ifa_next) {
      Address::ptr addr;
      uint32_t prefix_len = ~0u;
      if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
        continue;
      }
      if (next->ifa_addr == NULL) {
        // TODO(fengyu): ogstun virtual interface [08-09-21] //
        continue;
      }
      switch (next->ifa_addr->sa_family) {
      case AF_INET: {
        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
        uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
        prefix_len = CountBytes(netmask);
      } break;
      case AF_INET6: {
        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
        in6_addr &netmask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
        prefix_len = 0;
        for (int i = 0; i < 16; ++i) {
          prefix_len += CountBytes(netmask.s6_addr[i]);
        }
      } break;
      default:
        break;
      }
      if (addr) {
        res.insert(
            std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
      }
    }
  } catch (...) {
    LOG_ERROR(g_logger) << "address::getinterfaceaddress exception";
    freeifaddrs(results);
    return false;
  }
  freeifaddrs(results);
  return !res.empty();
}
bool Address::GetInterfaceAddress(
    std::vector<std::pair<Address::ptr, uint32_t>> &res,
    const std::string &iface, int family) {
  if (iface.empty() || iface == "*") {
    if (family == AF_INET || family == AF_UNSPEC) {
      res.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
    }
    if (family == AF_INET6 || family == AF_UNSPEC) {
      res.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
    }
    return true;
  }
  std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;
  if (!GetInterfaceAddress(results, family)) {
    return false;
  }
  auto its = results.equal_range(iface);
  for (; its.first != its.second; ++its.first) {
    res.push_back(its.first->second);
  }
  return true;
}

int Address::family() const { return addr()->sa_family; }

std::string Address::to_string() const {
  std::stringstream ss;
  insert(ss);
  return ss.str();
}
Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
  if (addr == nullptr) {
    return nullptr;
  }
  Address::ptr res;
  switch (addr->sa_family) {
  case AF_INET:
    res.reset(new IPv4Address(*(const sockaddr_in *)addr));
    break;
  case AF_INET6:
    res.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
    break;
  default:
    res.reset(new UnknowAddress(*addr));
    break;
  }
  return res;
}

bool Address::operator<(const Address &rhs) const {
  socklen_t minlen = std::min(addrlen(), rhs.addrlen());
  int res = memcmp(addr(), rhs.addr(), minlen);
  if (res < 0) {
    return true;
  } else if (res > 0) {
    return false;
  } else if (addrlen() < rhs.addrlen()) {
    return true;
  }
  return false;
}
bool Address::operator==(const Address &rhs) const {
  return addrlen() == rhs.addrlen() &&
         memcmp(addr(), rhs.addr(), addrlen()) == 0;
}
bool Address::operator!=(const Address &rhs) const { return !(*this == rhs); }

IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
  addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;

  int error = getaddrinfo(address, NULL, &hints, &res);
  if (error) {
    LOG_ERROR(g_logger) << "ipaddress::create " << address << ":" << port
                        << ", errno: " << strerror(errno);
    return nullptr;
  }
  try {
    IPAddress::ptr iptr = std::dynamic_pointer_cast<IPAddress>(
        Address::Create(res->ai_addr, (socklen_t)res->ai_addrlen));
    if (iptr) {
      iptr->port(port);
    }
    freeaddrinfo(res);
    return iptr;
  } catch (...) {
    freeaddrinfo(res);
    return nullptr;
  }
}
IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
  IPv4Address::ptr rt(new IPv4Address);
  rt->m_addr.sin_port = byteswapOnLittleEndian(port);
  int res = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
  if (res <= 0) {
    LOG_ERROR(g_logger) << "ipv4address::create " << address << ":" << port
                        << ", errno: " << strerror(errno);
    return nullptr;
  }
  return rt;
}
IPv4Address::IPv4Address(const sockaddr_in &address) { m_addr = address; }
IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian(port);
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr *IPv4Address::addr() const { return (sockaddr *)&m_addr; }

sockaddr *IPv4Address::addr() { return (sockaddr *)&m_addr; }

socklen_t IPv4Address::addrlen() const { return sizeof(m_addr); }

std::ostream &IPv4Address::insert(std::ostream &os) const {
  uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
  os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
     << ((addr >> 8) & 0xff) << "." << (addr & 0xff);
  os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
  return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
  if (prefix_len > 32) {
    return nullptr;
  }
  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr |=
      byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));

  return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
  if (prefix_len > 32) {
    return nullptr;
  }
  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr &=
      byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));

  return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin_family = AF_INET;
  subnet.sin_addr.s_addr =
      ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));

  return IPv4Address::ptr(new IPv4Address(subnet));
}
uint16_t IPv4Address::port() const {
  return byteswapOnLittleEndian(m_addr.sin_port);
}
void IPv4Address::port(uint16_t v) {
  m_addr.sin_port = byteswapOnLittleEndian(v);
}

IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
  IPv6Address::ptr rt(new IPv6Address);
  rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
  int res = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
  if (res <= 0) {
    LOG_ERROR(g_logger) << "ipv6address::create " << address << ":" << port
                        << ", errno: " << strerror(errno);
    return nullptr;
  }
  return rt;
}

IPv6Address::IPv6Address(const sockaddr_in6 &address) { m_addr = address; }
IPv6Address::IPv6Address() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
}
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
  m_addr.sin6_port = byteswapOnLittleEndian(port);
  memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
  // m_addr.sin6_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr *IPv6Address::addr() const { return (sockaddr *)&m_addr; }

sockaddr *IPv6Address::addr() { return (sockaddr *)&m_addr; }

socklen_t IPv6Address::addrlen() const { return sizeof(m_addr); }

std::ostream &IPv6Address::insert(std::ostream &os) const {
  os << "[";
  uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
  bool used_zeros = false;
  for (size_t i = 0; i < 8; ++i) {
    if (addr[i] == 0 && !used_zeros) {
      continue;
    }
    if (i && addr[i - 1] == 0 && !used_zeros) {
      os << ":";
      used_zeros = true;
    }
    if (i) {
      os << ":";
    }
    os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
  }
  if (!used_zeros && addr[7] == 0) {
    os << "::";
  }
  os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
  return os;
}
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  baddr.sin6_addr.s6_addr[prefix_len / 8] |=
      CreateMask<uint8_t>(prefix_len % 8);
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    baddr.sin6_addr.s6_addr[i] = 0xff;
  }
  return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  baddr.sin6_addr.s6_addr[prefix_len / 8] &=
      CreateMask<uint8_t>(prefix_len % 8);
  // for (int i = prefix_len / 8 + 1; i < 16; ++i) {
  //   baddr.sin6_addr.s6_addr[i] = 0xff;
  // }
  return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in6 subnet(m_addr);
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin6_family = AF_INET6;
  subnet.sin6_addr.s6_addr[prefix_len / 8] =
      ~CreateMask<uint8_t>(prefix_len % 8);
  for (uint32_t i = 0; i < prefix_len / 8; ++i) {
    subnet.sin6_addr.s6_addr[i] = 0xFF;
  }
  return IPv6Address::ptr(new IPv6Address(subnet));
}
uint16_t IPv6Address::port() const {
  return byteswapOnLittleEndian(m_addr.sin6_port);
}
void IPv6Address::port(uint16_t v) {
  m_addr.sin6_port = byteswapOnLittleEndian(v);
}

static const size_t MAX_PATH_LEN = sizeof((sockaddr_un *)0)->sun_path - 1;
UnixAddress::UnixAddress() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_len = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}
UnixAddress::UnixAddress(const std::string &path) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_len = path.size() + 1;

  if (!path.empty() && path[0] == '\0') {
    --m_len;
  }
  if (m_len > sizeof(m_addr.sun_path)) {
    throw std::logic_error("path too long");
  }
  memcpy(m_addr.sun_path, path.c_str(), m_len);
  m_len += offsetof(sockaddr_un, sun_path);
}

const sockaddr *UnixAddress::addr() const { return (sockaddr *)&m_addr; }

sockaddr *UnixAddress::addr() { return (sockaddr *)&m_addr; }

socklen_t UnixAddress::addrlen() const { return m_len; }

void UnixAddress::addrlen(uint32_t v) { m_len = v; }

// void UnixAddress::addrlen(socklen_t v) {m_len = v;}
std::ostream &UnixAddress::insert(std::ostream &os) const {
  if (m_len > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
    return os << "\\0"
              << std::string(m_addr.sun_path + 1,
                             m_len - offsetof(sockaddr_un, sun_path) - 1);
  }
  return os << m_addr.sun_path;
}

UnknowAddress::UnknowAddress(int family) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sa_family = family;
}

UnknowAddress::UnknowAddress(const sockaddr &addr) { m_addr = addr; }

sockaddr *UnknowAddress::addr() { return &m_addr; }

const sockaddr *UnknowAddress::addr() const { return &m_addr; }

socklen_t UnknowAddress::addrlen() const { return sizeof(m_addr); }

std::ostream &UnknowAddress::insert(std::ostream &os) const {
  os << "[UnknowAddress family=" << m_addr.sa_family << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
  return addr.insert(os);
}

} // namespace cool
