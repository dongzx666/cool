#ifndef __COOL__BYTEARRY_H
#define __COOL__BYTEARRY_H

#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace cool {

class ByteArray {
public:
  using ptr = std::shared_ptr<ByteArray>;
  struct Node {
    Node(size_t size);
    Node();
    ~Node();

    char *ptr;
    Node *next;
    size_t size;
  };

  ByteArray(size_t base_size = 4096);
  ~ByteArray();

  // write
  void write_fint8(int8_t val);
  void write_fuint8(uint8_t val);
  void write_fint16(int16_t val);
  void write_fuint16(uint16_t val);
  void write_fint32(int32_t val);
  void write_fuint32(uint32_t val);
  void write_fint64(int64_t val);
  void write_fuint64(uint64_t val);

  void write_int32(int32_t val);
  void write_uint32(uint32_t val);
  void write_int64(int64_t val);
  void write_uint64(uint64_t val);

  void write_float(float val);
  void write_double(double val);

  // length: int16, data
  void write_string_f16(const std::string &val);
  // length: int32, data
  void write_string_f32(const std::string &val);
  // length: int64, data
  void write_string_f64(const std::string &val);
  // length: varint, data
  void write_string_vint(const std::string &val);
  // data
  void write_string_withoutlen(const std::string &val);

  // read
  int8_t read_fint8();
  uint8_t read_fuint8();
  int16_t read_fint16();
  uint16_t read_fuint16();
  int32_t read_fint32();
  uint32_t read_fuint32();
  int64_t read_fint64();
  uint64_t read_fuint64();

  int32_t read_int32();
  uint32_t read_uint32();
  int64_t read_int64();
  uint64_t read_uint64();

  float read_float();
  double read_double();

  // length: int16, data
  std::string read_string_f16();
  // length: int32, data
  std::string read_string_f32();
  // length: int64, data
  std::string read_string_f64();
  // data
  std::string read_string_vint();

  // inner func
  void clear();;

  void write(const void* buf, size_t size);
  void read(void* buf, size_t size);
  void read(void* buf, size_t size, size_t pos) const;

  size_t position() const {return m_pos;}
  void position(size_t val);

  bool writeToFile(const std::string& name) const;
  bool readFromFile(const std::string& name);

  size_t getBaseSize() const {return m_base_size;}
  size_t getReadSize() const {return m_size - m_pos;}

  bool isLittleEndian() const;
  void setIsLittleEndian(bool val);

  std::string to_string() const;
  std::string to_hex_string() const;

  uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0) const;
  uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t pos) const;
  uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

  size_t getSize() const {return m_size;}
private:
  void addCapacity(size_t size);
  size_t getCapacity() const {return m_capacity - m_pos;}
private:
  size_t m_base_size;
  size_t m_pos;
  size_t m_capacity;
  size_t m_size;
  int8_t m_endian;

  Node* m_root;
  Node* m_cur;
};

} // namespace cool

#endif /* ifndef __COOL__BYTEARRY_H */
