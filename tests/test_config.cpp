#include "src/config.h"
#include "src/log.h"
#include <vector>
#include <yaml-cpp/node/parse.h>
#include <yaml.h>
#include <string>

cool::ConfigVar<int>::ptr g_int_value_config =
cool::Config::Lookup("system.port", static_cast<int>(8080), "system port");

cool::ConfigVar<double>::ptr g_double_value_config =
  cool::Config::Lookup("system.value", static_cast<double>(10.2), "system value");

cool::ConfigVar<std::vector<int>>::ptr g_vector_value_config =
  cool::Config::Lookup("system.int_vec", std::vector<int>{1,2,3}, "system int vec");

void print_yaml (const YAML::Node& node, int level) {
  if (node.IsScalar()) {
    COOL_LOG_INFO(COOL_LOG_ROOT()) << std::string(level*4, ' ') << node.Scalar() << " - " << node.Type();
  } else if (node.IsNull()){
    COOL_LOG_INFO(COOL_LOG_ROOT()) << std::string(level*4, ' ') << "NULL - " << node.Type() << " - " << level;
  } else if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      COOL_LOG_INFO(COOL_LOG_ROOT()) << std::string(level*4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
      print_yaml(it->second, level+1);
    }
  } else if (node.IsSequence()) {
    for (size_t i = 0; i < node.size(); ++i) {
      COOL_LOG_INFO(COOL_LOG_ROOT()) << std::string(level*4, ' ') << i << " - " << node[i].Type() << " - " << level;
      print_yaml(node[i], level+1);
    }
  }
}
void test_yaml() {
  YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
  print_yaml(root, 0);
  // COOL_LOG_INFO(COOL_LOG_ROOT()) << config;
}

void test_config () {
  COOL_LOG_INFO(COOL_LOG_ROOT()) << "before " << g_int_value_config->value();
  COOL_LOG_INFO(COOL_LOG_ROOT()) << "before " << g_double_value_config->to_string();
  auto v = g_vector_value_config->value();
  for (auto& i : v) {
    COOL_LOG_INFO(COOL_LOG_ROOT()) << "befor int_vec:" << i;
  }

  YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
  cool::Config::loadFromYaml(root);

  COOL_LOG_INFO(COOL_LOG_ROOT()) << "after " << g_int_value_config->value();
  COOL_LOG_INFO(COOL_LOG_ROOT()) << "after " << g_double_value_config->to_string();
  v = g_vector_value_config->value();
  for (auto& i : v) {
    COOL_LOG_INFO(COOL_LOG_ROOT()) << "after int_vec:" << i;
  }
}

int main() {
  // COOL_LOG_INFO(COOL_LOG_ROOT()) << g_double_value_config->val();
  // COOL_LOG_INFO(COOL_LOG_ROOT()) << g_double_value_config->to_string();

  //test_yaml();

  test_config();
  return 0;
}
