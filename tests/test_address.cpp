#include "src/address.h"
#include "src/log.h"
#include <map>
#include <string>
#include <type_traits>

cool::Logger::ptr g_logger = LOG_ROOT();


void test_host2ip () {
  std::vector<cool::Address::ptr> addrs;
  bool v = cool::Address::Lookup(addrs, "www.baidu.com:http");
  // TODO(fengyu): serval same ip ? [08-09-21] //
  if (!v) {
    LOG_ERROR(g_logger) << "lookup fail";
    return;
  }
  for (size_t i = 0; i < addrs.size(); ++i) {
    LOG_DEBUG(g_logger) << i << " - " << addrs[i]->to_string();
  }
}

void test_iface () {
  std::multimap<std::string, std::pair<cool::Address::ptr, uint32_t>> results;
  bool v = cool::Address::GetInterfaceAddress(results);
  if (!v) {
    LOG_ERROR(g_logger) << "getinterfaceaddress fail";
    return ;
  }

  for (auto& i : results) {
    LOG_DEBUG(g_logger) << i.first << " - " << i.second.first->to_string() << " - " << i.second.second;
  }
}

void test_ipv4() {
  auto addr = cool::IPAddress::Create("127.0.0.8");
  // auto addr = cool::IPAddress::Create("www.baidu.com");
  if (addr) {
    LOG_DEBUG(g_logger) << addr->to_string();
  }
}

int main(int argc, char* argv[]) {
  // test_host2ip();
  // test_iface();
  test_ipv4();
  return 0;
}
