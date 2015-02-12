// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <xzero/net/EndPoint.h>
#include <xzero/IdleTimeout.h>
#include <openssl/ssl.h>

namespace xzero {

class SslConnector;
class Scheduler;

class SslEndPoint : public EndPoint {
 public:
  SslEndPoint(int socket, SslConnector* connector, Scheduler* scheduler);
  ~SslEndPoint();

  int handle() const noexcept { return handle_; }

  bool isOpen() const override;
  void close() override;

  /**
   * Reads from remote endpoint and fills given buffer with it.
   */
  size_t fill(Buffer* sink) override;

  /**
   * Appends given buffer into the pending buffer vector and attempts to flush.
   */
  size_t flush(const BufferRef& source) override;
  size_t flush(int fd, off_t offset, size_t size) override;

  /**
   * Ensures that the SSL socket is ready for receiving data.
   *
   * Any pending data in the fillBuffer will preceed.
   *
   * This might internally cause write <b>and</b> read operations
   * through the SSL layer.
   */
  void wantFill() override;

  /** Ensures that the SSL socket is ready for more flush operations.
   *
   * Any pending data in the flushBuffer will be sent before
   * @c connection()->onFlushable() will be invoked.
   *
   * This might internally cause write <b>and</b> read operations
   * through the SSL layer.
   */
  void wantFlush() override;

  TimeSpan idleTimeout() override;
  void setIdleTimeout(TimeSpan timeout) override;
  bool isBlocking() const override;
  void setBlocking(bool enable) override;
  bool isCorking() const override;
  void setCorking(bool enable) override;
  std::string toString() const override;

 private:
  void fillable();
  void flushable();

  void onHandshake();
  void onRead();
  void onWrite();
  void onShutdown();

 private:
  int handle_;
  SslConnector* connector_;
  Scheduler* scheduler_;
  IdleTimeout idleTimeout_;
  SSL* ssl_;
  Buffer readBuffer_;
  Buffer writeBuffer_;
};

} // namespace xzero
