#ifndef __COOL_NONCOPYABLE_H
#define __COOL_NONCOPYABLE_H

namespace cool {
class Noncopyable {
  public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};
}

#endif /* ifndef __COOL_NONCOPYABLE_H */
