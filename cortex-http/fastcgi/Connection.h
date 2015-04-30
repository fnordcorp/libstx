// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#pragma once

#include <cortex-http/Api.h>
#include <cortex-http/fastcgi/RequestParser.h>
#include <cortex-http/fastcgi/Generator.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-http/HttpTransport.h>
#include <cortex-http/HttpHandler.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/TimeSpan.h>
#include <memory>
#include <unordered_map>

namespace cortex {

class HttpDateGenerator;
class HttpOutputCompressor;
class HttpChannel;

namespace http {
namespace fastcgi {

class Stream;

/**
 * @brief Implements a HTTP/1.1 transport connection.
 */
class CORTEX_HTTP_API Connection : public HttpTransport {
 public:
  Connection(EndPoint* endpoint,
             Executor* executor,
             const HttpHandler& handler,
             HttpDateGenerator* dateGenerator,
             HttpOutputCompressor* outputCompressor,
             size_t maxRequestUriLength,
             size_t maxRequestBodyLength);
  ~Connection();

  void onOpen() override;
  void onClose() override;

  void abort() override;
  void completed() override;

  void send(HttpResponseInfo&& responseInfo, Buffer&& chunk,
            CompletionHandler onComplete) override;
  void send(HttpResponseInfo&& responseInfo, const BufferRef& chunk,
            CompletionHandler onComplete) override;
  void send(HttpResponseInfo&& responseInfo, FileRef&& chunk,
            CompletionHandler onComplete) override;

  void send(Buffer&& chunk, CompletionHandler onComplete) override;
  void send(const BufferRef& chunk, CompletionHandler onComplete) override;
  void send(FileRef&& chunk, CompletionHandler onComplete) override;

  void setInputBufferSize(size_t size) override;

  size_t bytesReceived() const noexcept;// { return parser_.bytesReceived(); }

 private:
  void patchResponseInfo(HttpResponseInfo& info);
  void onFillable() override;
  void parseFragment();
  void onFlushable() override;
  void onInterestFailure(const std::exception& error) override;
  void onResponseComplete(bool succeed);

 private:
  Buffer inputBuffer_;
  size_t inputOffset_;
  RequestParser parser;

  std::unordered_map<int, std::unique_ptr<Stream>> streams_;

  EndPointWriter writer_;
  CompletionHandler onComplete_;
};

} // namespace fastcgi
} // namespace http
} // namespace cortex
