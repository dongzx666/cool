#include "config.h"
#include "src/log.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace cool {
Config::configVarMap Config::s_datas;
ConfigVarBase::ptr Config::lookupBase(const std::string &name) {
  auto it = s_datas.find(name);
  return it == s_datas.end() ? nullptr : it->second;
}

static void listAllMember(const std::string& prefix,
    const YAML::Node& node,
    std::list<std::pair<std::string, const YAML::Node>>& output) {
  if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
    COOL_LOG_ERROR(COOL_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
    return;
  }
  output.push_back(std::make_pair(prefix, node));
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      listAllMember(prefix.empty() ? it->first.Scalar()
          : (prefix + "." + it->first.Scalar()), it->second, output);
    }
  }
}
void Config::loadFromYaml(const YAML::Node& root){
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  listAllMember("", root, all_nodes);

  for (auto& i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) {
      continue;
    }
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = Config::lookupBase(key);

    if (var) {
      if (i.second.IsScalar()) {
        var->from_string(i.second.Scalar());
      } else {
        std::stringstream ss;
        ss << i.second;
        var->from_string(ss.str());
      }
    }
  }
}
}
