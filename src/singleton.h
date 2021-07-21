#ifndef __COOL_SINGLETON
#define __COOL_SINGLETON

#include <memory>
namespace cool {

  template<class T, class X = void, int N = 0>
  class Singleton {
    public:
      static T* instance() {
        static T v;
        return &v;
      }
  };

  template <class T, class X = void, int N = 0>
  class SingletonPtr {
    public:
      static std::shared_ptr<T> instance() {
        static std::shared_ptr<T> v(new T);
        return v;
      }
  };
}

#endif /* ifndef __COOL_SINGLETON */
