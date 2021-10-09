#ifndef __COOL_ENDIAN_H
#define __COOL_ENDIAN_H

#include <type_traits>
#define COOL_LITTLE_ENDIAN 1
#define COOL_BIG_ENDIAN 2

#include <byteswap.h>
#include <cstdint>

namespace cool {

// c++17可以用constexpr if替换模板写法
// SFINEA特性
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
  return (T)bswap_64((uint64_t)value);
}
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
  return (T)bswap_32((uint32_t)value);
}
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
  return (T)bswap_16((uint16_t)value);
}

#if COOL_ORDER == BIG_ENDIAN
#define COOL_BYTE_ORDER COOL_BIG_ENDIAN
#else
#define COOL_BYTE_ORDER COOL_LITTLE_ENDIAN
#endif

#if COOL_BYTE_ORDER == COOL_BIG_ENDIAN
template<class T>
T byteswapOnLittleEndian(T t) {
  return t;
}
template<class T>
T byteswapOnBigEndian(T t) {
  return byteswap(t);
}
#else
template<class T>
T byteswapOnLittleEndian(T t) {
  return byteswap(t);
}
template<class T>
T byteswapOnBigEndian(T t) {
  return t;
}
#endif

} // namespace cool

#endif /* ifndef __COOL_ENDIAN_H */
