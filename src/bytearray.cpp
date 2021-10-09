#include "bytearray.h"
#include "endian.h"
#include "log.h"
#include "socket.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace cool {

static cool::Logger::ptr g_logger = LOG_NAME("system");

ByteArray::Node::Node(size_t s) : ptr(new char[s]), next(nullptr), size(s) {}

ByteArray::Node::Node() : ptr(nullptr), next(nullptr), size(0) {}

ByteArray::Node::~Node() {
  if (ptr) {
    delete[] ptr;
  }
}

ByteArray::ByteArray(size_t base_size)
    : m_base_size(base_size), m_pos(0), m_capacity(base_size), m_size(0),
      m_endian(COOL_BIG_ENDIAN), m_root(new Node(base_size)), m_cur(m_root) {}

ByteArray::~ByteArray() {
  Node *temp = m_root;
  while (temp) {
    m_cur = temp;
    temp = temp->next;
    delete m_cur;
  }
}

bool ByteArray::isLittleEndian() const {
  return m_endian == COOL_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val) {
  if (val) {
    m_endian = COOL_LITTLE_ENDIAN;
  } else {
    m_endian = COOL_BIG_ENDIAN;
  }
}

void ByteArray::write_fint8(int8_t val) { write(&val, sizeof(val)); }

void ByteArray::write_fuint8(uint8_t val) { write(&val, sizeof(val)); }

void ByteArray::write_fint16(int16_t val) {
  if (m_endian != COOL_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::write_fuint16(uint16_t val) {
  if (m_endian != COOL_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}
void ByteArray::write_fint32(int32_t val) {
  if (m_endian != COOL_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}
void ByteArray::write_fuint32(uint32_t val) {
  if (m_endian != COOL_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}
void ByteArray::write_fint64(int64_t val) {
  if (m_endian != COOL_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}
void ByteArray::write_fuint64(uint64_t val) {
  if (m_endian != COOL_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

static uint32_t EncodeZigzap32(const int32_t &val) {
  if (val < 0) {
    return ((uint32_t)(-val)) * 2 - 1;
  } else {
    return val * 2;
  }
}

static int32_t DecodeZigzap32(const uint32_t &val) {
  return (val >> 1) ^ -(val & 1);
}

static uint64_t EncodeZigzap64(const int64_t &val) {
  if (val < 0) {
    return ((uint64_t)(-val)) * 2 - 1;
  } else {
    return val * 2;
  }
}

static int64_t DecodeZigzap64(const uint64_t &val) {
  return (val >> 1) ^ -(val & 1);
}

void ByteArray::write_int32(int32_t val) { write_uint32(EncodeZigzap32(val)); }

void ByteArray::write_uint32(uint32_t val) {
  uint8_t temp[5];
  uint8_t i = 0;
  while (val >= 0x80) {
    temp[i++] = (val & 0x7f) | 0x80;
    val >>= 7;
  }
  temp[i++] = val;
  write(temp, i);
}

void ByteArray::write_int64(int64_t val) { write_uint64(EncodeZigzap64(val)); }

void ByteArray::write_uint64(uint64_t val) {
  uint8_t temp[10];
  uint8_t i = 0;
  while (val >= 0x80) {
    temp[i++] = (val & 0x7f) | 0x80;
    val >>= 7;
  }
  temp[i++] = val;
  write(temp, i);
}

void ByteArray::write_float(float val) {
  uint32_t temp;
  memcpy(&temp, &val, sizeof(val));
  write_fuint32(temp);
}

void ByteArray::write_double(double val) {
  uint64_t temp;
  memcpy(&temp, &val, sizeof(val));
  write_fuint64(temp);
}

// length: int16, data
void ByteArray::write_string_f16(const std::string &val) {
  write_fint16(val.size());
  write(val.c_str(), val.size());
}
// length: int32, data
void ByteArray::write_string_f32(const std::string &val) {
  write_fint32(val.size());
  write(val.c_str(), val.size());
}
// length: int64, data
void ByteArray::write_string_f64(const std::string &val) {
  write_fint64(val.size());
  write(val.c_str(), val.size());
}
// length: varint, data
void ByteArray::write_string_vint(const std::string &val) {
  write_uint64(val.size());
  write(val.c_str(), val.size());
}
// data
void ByteArray::write_string_withoutlen(const std::string &val) {
  write(val.c_str(), val.size());
}

// read
int8_t ByteArray::read_fint8() {
  int8_t val;
  read(&val, sizeof(val));
  return val;
}
uint8_t ByteArray::read_fuint8() {
  uint8_t val;
  read(&val, sizeof(val));
  return val;
}

#define XX(type)                     \
  type val;                          \
  read(&val, sizeof(val));           \
  if (m_endian == COOL_BYTE_ORDER) { \
    return val;                      \
  } else {                           \
    return byteswap(val);            \
  }

int16_t ByteArray::read_fint16() { XX(int16_t); }
uint16_t ByteArray::read_fuint16() { XX(uint16_t); }
int32_t ByteArray::read_fint32() { XX(int32_t); }
uint32_t ByteArray::read_fuint32() { XX(uint32_t); }
int64_t ByteArray::read_fint64() { XX(int64_t); }
uint64_t ByteArray::read_fuint64() { XX(uint64_t); }
#undef XX

int32_t ByteArray::read_int32() { return DecodeZigzap32(read_uint32()); }
uint32_t ByteArray::read_uint32() {
  uint32_t res = 0;
  for (int i = 0; i < 32; i += 7) {
    uint8_t b = read_fuint8();
    if (b < 0x80) {
      res |= ((uint32_t)b) << i;
      break;
    } else {
      res |= (((uint32_t)b & 0x7f) << i);
    }
  }
  return res;
}
int64_t ByteArray::read_int64() { return DecodeZigzap64(read_uint64()); }
uint64_t ByteArray::read_uint64() {
  uint64_t res = 0;
  for (int i = 0; i < 64; i += 7) {
    uint8_t b = read_fuint8();
    if (b < 0x80) {
      res |= ((uint64_t)b) << i;
      break;
    } else {
      res |= (((uint64_t)b & 0x7f) << i);
    }
  }
  return res;
}

float ByteArray::read_float() {
  uint32_t temp = read_fuint32();
  float res;
  memcpy(&res, &temp, sizeof(temp));
  return res;
}
double ByteArray::read_double() {
  uint64_t temp = read_fuint64();
  double res;
  memcpy(&res, &temp, sizeof(temp));
  return res;
}

// length: int16, data
std::string ByteArray::read_string_f16() {
  uint16_t len = read_fuint16();
  std::string buf;
  buf.resize(len);
  read(&buf[0], len);
  return buf;
}
// length: int32, data
std::string ByteArray::read_string_f32() {
  uint32_t len = read_fuint32();
  std::string buf;
  buf.resize(len);
  read(&buf[0], len);
  return buf;
}
// length: int64, data
std::string ByteArray::read_string_f64() {
  uint64_t len = read_fuint64();
  std::string buf;
  buf.resize(len);
  read(&buf[0], len);
  return buf;
}
// data
std::string ByteArray::read_string_vint() {
  uint64_t len = read_fuint64();
  std::string buf;
  buf.resize(len);
  read(&buf[0], len);
  return buf;
}

// inner func
void ByteArray::clear() {
  m_pos = m_size = 0;
  m_capacity = m_base_size;
  Node *temp = m_root->next;
  while (temp) {
    m_cur = temp;
    temp = temp->next;
    delete m_cur;
  }
  m_cur = m_root;
  m_root->next = nullptr;
}

void ByteArray::write(const void *buf, size_t size) {
  if (size == 0) {
    return;
  }
  addCapacity(size);

  size_t npos = m_pos % m_base_size;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;

  while (size > 0) {
    if (ncap >= size) {
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size);
      if (m_cur->size == (npos + size)) {
        m_cur = m_cur->next;
      }
      m_pos += size;
      bpos += size;
      size = 0;
    } else {
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);
      m_pos += ncap;
      bpos += ncap;
      size -= ncap;
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }

  if (m_pos > m_size) {
    m_size = m_pos;
  }
}

void ByteArray::read(void *buf, size_t size) {
  if (size > getReadSize()) {
    throw std::out_of_range("not enough len");
  }
  size_t npos = m_pos % m_base_size;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;

  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, size);
      if (m_cur->size == npos + size) {
        m_cur = m_cur->next;
      }
      m_pos += size;
      bpos += size;
      size = 0;
    } else {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, ncap);
      m_pos += ncap;
      bpos += ncap;
      size -= ncap;
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }
}

void ByteArray::read(void *buf, size_t size, size_t pos) const {
  if (size > getReadSize()) {
    throw std::out_of_range("not enough len");
  }
  size_t npos = pos % m_base_size;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  Node *cur = m_cur;

  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, cur->ptr + npos, size);
      if (cur->size == npos + size) {
        cur = cur->next;
      }
      pos += size;
      bpos += size;
      size = 0;
    } else {
      memcpy((char *)buf + bpos, cur->ptr + npos, ncap);
      pos += ncap;
      bpos += ncap;
      size -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
  }
}

void ByteArray::position(size_t val) {
  if (val > m_capacity) {
    throw std::out_of_range("set pos out of range");
  }
  m_pos = val;
  if (m_pos > m_size) {
    m_size = m_pos;
  }
  m_cur = m_root;
  while (val > m_cur->size) {
    val -= m_cur->size;
    m_cur = m_cur->next;
  }
  if (val == m_cur->size) {
    m_cur = m_cur->next;
  }
}

bool ByteArray::writeToFile(const std::string &name) const {
  std::ofstream ofs;
  ofs.open(name, std::ios::trunc | std::ios::binary);
  if (!ofs) {
    LOG_ERROR(g_logger) << "write to file, name is " << name << " error";
    return false;
  }
  int64_t read_size = getReadSize();
  int64_t pos = m_pos;
  Node *cur = m_cur;

  while (read_size > 0) {
    int diff = pos % m_base_size;
    int64_t len =
        (read_size > (int64_t)m_base_size ? m_base_size : read_size) - diff;

    ofs.write(cur->ptr + diff, len);
    cur = cur->next;
    pos += len;
    read_size -= len;
  }

  return true;
}
bool ByteArray::readFromFile(const std::string &name) {
  std::ifstream ifs;
  ifs.open(name, std::ios::binary);

  if (!ifs) {
    LOG_ERROR(g_logger) << "read from file, name is " << name << " error";
    return false;
  }

  std::shared_ptr<char> buf(new char[m_base_size],
                            [](char *ptr) { delete[] ptr; });
  while (!ifs.eof()) {
    ifs.read(buf.get(), m_base_size);
    write(buf.get(), ifs.gcount());
  }
  return true;
}

void ByteArray::addCapacity(size_t size) {
  if (size == 0) {
    return;
  }
  size_t old_cap = getCapacity();
  if (old_cap >= size) {
    LOG_FMT_DEBUG(g_logger, "old_cap(%u) >= size(%u)", old_cap, size);
    return;
  }

  size = size - old_cap;
  size_t count =
      (size / m_base_size) + ((size % m_base_size > old_cap) ? 1 : 0);
  // size_t count = ceil(1.0 * size / m_base_size);
  Node *temp = m_root;

  while (temp->next) {
    temp = temp->next;
  }

  Node *first = nullptr;
  for (size_t i = 0; i < count; ++i) {
    temp->next = new Node(m_base_size);
    if (first == nullptr) {
      first = temp->next;
    }
    temp = temp->next;
    m_capacity += m_base_size;
  }

  if (old_cap == 0) {
    m_cur = first;
  }
}

std::string ByteArray::to_string() const {
  std::string str;
  str.resize(getReadSize());
  if (str.empty()) {
    return str;
  }
  read(&str[0], str.size(), m_pos);
  return str;
}

std::string ByteArray::to_hex_string() const {
  std::string str = to_string();
  std::stringstream ss;

  for (size_t i = 0; i < str.size(); ++i) {
    if (i > 0 && i % 32 == 0) {
      ss << std::endl;
    }
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i]
       << " ";
  }

  return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,
                                   uint64_t len) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }
  uint64_t size = len;
  size_t npos = m_pos % m_base_size;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node *cur = m_cur;

  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                                   uint64_t pos) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }
  uint64_t size = len;
  size_t npos = pos % m_base_size;
  size_t count = pos / m_base_size;
  Node *cur = m_root;
  while (count > 0) {
    cur = cur->next;
    --count;
  }
  size_t ncap = cur->size - npos;
  struct iovec iov;

  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
  if (len == 0) {
    return 0;
  }
  addCapacity(len);
  uint64_t size = len;

  size_t npos = m_pos % m_base_size;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node *cur = m_cur;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

} // namespace cool
