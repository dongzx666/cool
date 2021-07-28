#include "src/config.h"
#include "src/log.h"
#include <sstream>
#include <string>
#include <vector>
#include <yaml.h>

cool::ConfigVar<int>::ptr g_int_value_config =
    cool::Config::lookup("system.port", static_cast<int>(8080), "system port");

// cool::ConfigVar<double>::ptr g_int_valuex_config = cool::Config::lookup(
//     "system.port", static_cast<double>(8080), "system port");

cool::ConfigVar<double>::ptr g_double_value_config = cool::Config::lookup(
    "system.value", static_cast<double>(10.2), "system value");

cool::ConfigVar<std::vector<int>>::ptr g_int_vector_value_config =
    cool::Config::lookup("system.int_vec", std::vector<int>{1, 2, 3},
                         "system int vec");

cool::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    cool::Config::lookup("system.int_list", std::list<int>{1, 2, 3},
                         "system int list");

cool::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    cool::Config::lookup("system.int_set", std::set<int>{1, 2, 3},
                         "system int set");

cool::ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set_value_config =
    cool::Config::lookup("system.int_uset", std::unordered_set<int>{1, 2, 3},
                         "system int unordered set");

cool::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config =
    cool::Config::lookup("system.str_int_map",
                         std::map<std::string, int>{{"k", 2}},
                         "system str int map");

cool::ConfigVar<std::unordered_map<std::string, int>>::ptr
    g_str_int_umap_value_config =
        cool::Config::lookup("system.str_int_umap",
                             std::unordered_map<std::string, int>{{"k", 2}},
                             "system str int umap");

void print_yaml(const YAML::Node &node, int level) {
  if (node.IsScalar()) {
    LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar()
                         << " - " << node.Type();
  } else if (node.IsNull()) {
    LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - "
                         << node.Type() << " - " << level;
  } else if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - "
                           << it->second.Type() << " - " << level;
      print_yaml(it->second, level + 1);
    }
  } else if (node.IsSequence()) {
    for (size_t i = 0; i < node.size(); ++i) {
      LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << i << " - "
                           << node[i].Type() << " - " << level;
      print_yaml(node[i], level + 1);
    }
  }
}
void test_yaml() {
  YAML::Node root = YAML::LoadFile("../config/log.yml");
  print_yaml(root, 0);
  // COOL_LOG_INFO(COOL_LOG_ROOT()) << config;
}

void test_config() {
  LOG_INFO(LOG_ROOT()) << "before " << g_int_value_config->value();
  LOG_INFO(LOG_ROOT()) << "before " << g_double_value_config->to_string();
#define XX(g_var, name, prefix)                                                \
  {                                                                            \
    auto v = g_var->value();                                                   \
    for (auto &i : v) {                                                        \
      LOG_INFO(LOG_ROOT()) << #prefix " " #name ": " << i;                     \
    }                                                                          \
    LOG_INFO(LOG_ROOT()) << #prefix " " #name " yaml:\n"                       \
                         << g_var->to_string();                                \
  }
#define XX_M(g_var, name, prefix)                                              \
  {                                                                            \
    auto v = g_var->value();                                                   \
    for (auto &i : v) {                                                        \
      LOG_INFO(LOG_ROOT()) << #prefix " " #name ": {" << i.first << " : "      \
                           << i.second << "}";                                 \
    }                                                                          \
    LOG_INFO(LOG_ROOT()) << #prefix " " #name " yaml:\n"                       \
                         << g_var->to_string();                                \
  }
  XX(g_int_vector_value_config, int_vec, before);
  XX(g_int_list_value_config, int_list, before);
  XX(g_int_set_value_config, int_set, before);
  XX(g_int_unordered_set_value_config, int_uset, before);
  XX_M(g_str_int_map_value_config, str_int_map, before);
  XX_M(g_str_int_umap_value_config, str_int_umap, before);

  YAML::Node root = YAML::LoadFile("../config/log.yml");
  cool::Config::load_from_yaml(root);

  LOG_INFO(LOG_ROOT()) << "after " << g_int_value_config->value();
  LOG_INFO(LOG_ROOT()) << "after " << g_double_value_config->to_string();
  XX(g_int_vector_value_config, int_vec, after);
  XX(g_int_list_value_config, int_list, after);
  XX(g_int_set_value_config, int_set, after);
  XX(g_int_unordered_set_value_config, int_uset, after);
  XX_M(g_str_int_map_value_config, str_int_map, after);
  XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

class Person {
public:
  std::string m_name = "person";
  int m_age = 0;
  bool m_sex = 0;

  std::string to_string() const {
    std::stringstream ss;
    ss << "[Person name=" << m_name << ",age=" << m_age << ",sex=" << m_sex
       << "]";
    return ss.str();
  }

  bool operator==(const Person &oth) const {
    return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
  }
};
namespace cool {
template <> class LexicalCast<std::string, Person> {
public:
  Person operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    Person p;
    p.m_name = node["name"].as<std::string>();
    p.m_age = node["age"].as<int>();
    p.m_sex = node["sex"].as<int>();
    return p;
  }
};
template <> class LexicalCast<Person, std::string> {
public:
  std::string operator()(const Person &p) {
    YAML::Node node;
    node["name"] = p.m_name;
    node["age"] = p.m_age;
    node["sex"] = p.m_sex;
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
} // namespace cool
cool::ConfigVar<Person>::ptr g_person =
    cool::Config::lookup("class.person", Person(), "system person");

void test_class() {
  g_person->addListener([](const Person &old_value, const Person &new_value) {
    LOG_INFO(LOG_ROOT()) << "[addListener]old_value=" << old_value.to_string()
                         << "new_value=" << new_value.to_string();
  });
  LOG_INFO(LOG_ROOT()) << "before" << g_person->value().to_string() << std::endl
                       << g_person->to_string();

  YAML::Node root = YAML::LoadFile("../config/log.yml");
  cool::Config::load_from_yaml(root);

  LOG_INFO(LOG_ROOT()) << "after" << g_person->value().to_string() << std::endl
                       << g_person->to_string();
}

void test_log() {
  static cool::Logger::ptr sys_log = LOG_NAME("system");
  LOG_INFO(sys_log) << "hello system" << std::endl;
  std::cout << cool::LoggerMgr::instance()->to_yaml_string() << std::endl;
  YAML::Node root = YAML::LoadFile("../config/log.yml");
  // std::cout << root << std::endl;
  cool::Config::load_from_yaml(root);
  std::cout << "===========================" << std::endl;
  std::cout << cool::LoggerMgr::instance()->to_yaml_string() << std::endl;
  LOG_INFO(sys_log) << "hello system" << std::endl;
  sys_log->formatter("%d - %m%n");
  LOG_INFO(sys_log) << "hello system" << std::endl;
}
void test_visit() {
  cool::Config::Visit([](cool::ConfigVarBase::ptr var) {
    LOG_INFO(LOG_ROOT()) << "name=" << var->name() << ",des=" << var->des()
                       << ",typename=" << var->type()
                       << ",value=" << var->to_string();
  });
}
int main() {
  // LOG_INFO(LOG_ROOT()) << g_double_value_config->val();
  // LOG_INFO(LOG_ROOT()) << g_double_value_config->to_string();

  // test_yaml();

  // test_config();

  // test_class();

  // test_log();

  test_visit();
  return 0;
}
