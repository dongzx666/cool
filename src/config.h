#ifndef __COOL_CONFIG_H
#define __COOL_CONFIG_H


#include <boost/lexical_cast.hpp>
#include "yaml.h"
#include <algorithm>
#include <exception>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include "log.h"
#include "util.h"

namespace cool{
class ConfigVarBase {
  public:
    using ptr = std::shared_ptr<ConfigVarBase>;
    ConfigVarBase(const std::string& name, const std::string& des = "")
      :m_name(name)
      ,m_des(des){
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
     }
    virtual ~ConfigVarBase() {}
    const std::string& name() const {return m_name;}
    const std::string& des() const {return m_des;}
    virtual std::string to_string() = 0;
    virtual bool from_string(const std::string& val) = 0;
  protected:
    std::string m_name;
    std::string m_des;
};

// F from_type, T to_type
template<class F, class T>
class LexicalCast {
public:
  T operator()(const F& v) {
    return boost::lexical_cast<T>(v);
  }
};

template<class T>
class LexicalCast<std::string, std::vector<T>> {
public:
  std::vector<T> operator()(const std::string& v) {
    YAML::Node node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};
template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
  std::string operator()(const std::vector<T>& v) {
    YAML::Node node;
    for (auto& i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// FromStr T operator(const std::string&)
// ToStr std::string operator() (const T&)
template<class T
  , class FromStr = LexicalCast<std::string, T>
  , class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
  public:
    using ptr = std::shared_ptr<ConfigVar>;
    ConfigVar(const std::string& name, const T& default_val, const std::string& des = "")
      :ConfigVarBase(name, des)
      ,m_val(default_val) {}
    std::string to_string() override {
      try {
        // return boost::lexical_cast<std::string>(m_val);
        return ToStr()(m_val);
      } catch (std::exception& e) {
        COOL_LOG_ERROR(COOL_LOG_ROOT()) << "ConfigVar::tostring exception" << e.what() << " convert: " << typeid(m_val).name() << "to string";
      }
      return "";
    }
    bool from_string(const std::string& val) override {
      try {
        // m_val = boost::lexical_cast<T>(val);
        value(FromStr()(val));
      }catch(std::exception& e) {
        COOL_LOG_ERROR(COOL_LOG_ROOT()) << "ConfigVar::from string exception" << e.what() << " convert: string to " << typeid(m_val).name();
      }
      return false;
    }
    const T value() const {return m_val;}
    void value(const T& v) {m_val = v;}
  private:
    T m_val;
};

class Config {
public:
  using configVarMap = std::map<std::string, ConfigVarBase::ptr>;

  template<class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_val,const std::string& des = "") {
    auto temp = Lookup<T>(name);
    if (temp) {
      COOL_LOG_INFO(COOL_LOG_ROOT()) << "Lookup name=" << name << " exist";
      return temp;
    }
    if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != std::string::npos) {
      COOL_LOG_ERROR(COOL_LOG_ROOT()) << "Lookup name invalid" << name;
      throw std::invalid_argument(name);
    }
    typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_val, des));
    s_datas[name] = v;
    return v;
  }

  template<class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
    auto it = s_datas.find(name);
    if (it == s_datas.end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
  }
  static void loadFromYaml(const YAML::Node& root);
  static ConfigVarBase::ptr lookupBase(const std::string& name);
private:
  static configVarMap s_datas;
};


}


#endif /* ifndef __COOL_CONFIG_H */
