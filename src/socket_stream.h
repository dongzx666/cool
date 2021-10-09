#ifndef __COOL_SOCKET_STREAM_H
#define __COOL_SOCKET_STREAM_H

#include "stream.h"
#include "socket.h"
#include <memory>

namespace cool {

class SocketStream : public Stream {
public:
  using ptr = std::shared_ptr<SocketStream>;
  SocketStream (Socket::ptr sock, bool owner = true);
  ~SocketStream();
  virtual int read(void *buffer, size_t length) override;
  virtual int read(ByteArray::ptr ba, size_t length) override;
  virtual int write(const void *buffer, size_t length) override;
  virtual int write(ByteArray::ptr ba, size_t length) override;
  virtual void close() override;

  Socket::ptr socket() const {return m_socket;}
  bool isConnected() const;
protected:
  Socket::ptr m_socket;
  bool m_owner;

private:
};

} /* namespace cool */ 

#endif /* ifndef __COOL_SOCKET_STREAM_H */
