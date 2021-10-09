#include "src/bytearray.h"
#include "src/log.h"
#include "src/macro.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

static cool::Logger::ptr g_logger = LOG_ROOT();

void test() {
#define XX(type, len, write_fun, read_fun, base_len)                       \
  {                                                                        \
    std::vector<type> vec;                                                 \
    for (int i = 0; i < len; ++i) {                                        \
      vec.push_back(rand());                                               \
    }                                                                      \
    cool::ByteArray::ptr ba(new cool::ByteArray(base_len));                \
    for (auto &i : vec) {                                                  \
      ba->write_fun(i);                                                    \
    }                                                                      \
    ba->position(0);                                                       \
    for (size_t i = 0; i < vec.size(); ++i) {                              \
      type v = ba->read_fun();                                             \
      ASSERT(v == vec[i]);                                                 \
    }                                                                      \
    ASSERT(ba->getReadSize() == 0);                                        \
    LOG_DEBUG(g_logger) << #write_fun "/" #read_fun "(" #type              \
                        << ") len: " << len << " size: " << ba->getSize(); \
  }

  XX(int8_t, 100, write_fint8, read_fint8, 1)
  XX(uint8_t, 100, write_fuint8, read_fuint8, 1)
  XX(int16_t, 100, write_fint16, read_fint16, 1)
  XX(uint16_t, 100, write_fuint16, read_fuint16, 1)
  XX(int32_t, 100, write_fint32, read_fint32, 1)
  XX(uint32_t, 100, write_fuint32, read_fuint32, 1)
  XX(int64_t, 100, write_fint64, read_fint64, 1)
  XX(uint64_t, 100, write_fuint64, read_fuint64, 1)

  XX(int32_t, 100, write_int32, read_int32, 1)
  XX(uint32_t, 100, write_uint32, read_uint32, 1)
  XX(int64_t, 100, write_int64, read_int64, 1)
  XX(uint64_t, 100, write_uint64, read_uint64, 1)
#undef XX

#define XX(type, len, write_fun, read_fun, base_len)                        \
  {                                                                         \
    std::vector<type> vec;                                                  \
    for (int i = 0; i < len; ++i) {                                         \
      vec.push_back(rand());                                                \
    }                                                                       \
    cool::ByteArray::ptr ba(new cool::ByteArray(base_len));                 \
    for (auto &i : vec) {                                                   \
      ba->write_fun(i);                                                     \
    }                                                                       \
    ba->position(0);                                                        \
    for (size_t i = 0; i < vec.size(); ++i) {                               \
      type v = ba->read_fun();                                              \
      ASSERT(v == vec[i]);                                                  \
    }                                                                       \
    ASSERT(ba->getReadSize() == 0);                                         \
    LOG_DEBUG(g_logger) << #write_fun "/" #read_fun "(" #type               \
                        << ") len: " << len << " size: " << ba->getSize();  \
    ba->position(0);                                                        \
    ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));   \
    cool::ByteArray::ptr ba2(new cool::ByteArray(base_len * 2));            \
    ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    ba2->position(0);                                                       \
    ASSERT(ba->to_string() == ba2->to_string());                            \
    ASSERT(ba->position() == 0);                                            \
    ASSERT(ba2->position() == 0);                                           \
  }

  XX(int8_t, 100, write_fint8, read_fint8, 1)
  XX(uint8_t, 100, write_fuint8, read_fuint8, 1)
  XX(int16_t, 100, write_fint16, read_fint16, 1)
  XX(uint16_t, 100, write_fuint16, read_fuint16, 1)
  XX(int32_t, 100, write_fint32, read_fint32, 1)
  XX(uint32_t, 100, write_fuint32, read_fuint32, 1)
  XX(int64_t, 100, write_fint64, read_fint64, 1)
  XX(uint64_t, 100, write_fuint64, read_fuint64, 1)

  XX(int32_t, 100, write_int32, read_int32, 1)
  XX(uint32_t, 100, write_uint32, read_uint32, 1)
  XX(int64_t, 100, write_int64, read_int64, 1)
  XX(uint64_t, 100, write_uint64, read_uint64, 1)
#undef XX
}

int main(int argc, char *argv[]) {
  test();
  return 0;
}
