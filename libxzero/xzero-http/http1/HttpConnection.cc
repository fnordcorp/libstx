// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero-http/http1/HttpConnection.h>
#include <xzero-http/http1/Http1Channel.h>
#include <xzero-http/HttpBufferedInput.h>
#include <xzero-http/HttpDateGenerator.h>
#include <xzero-http/HttpResponseInfo.h>
#include <xzero-http/HttpResponse.h>
#include <xzero-http/HttpRequest.h>
#include <xzero-http/BadMessage.h>
#include <xzero-base/net/Connection.h>
#include <xzero-base/net/EndPoint.h>
#include <xzero-base/net/EndPointWriter.h>
#include <xzero-base/executor/Executor.h>
#include <xzero-base/logging.h>
#include <xzero-base/RuntimeError.h>
#include <xzero-base/WallClock.h>
#include <xzero-base/sysconfig.h>
#include <cassert>
#include <cstdlib>

namespace xzero {
namespace http1 {

#define ERROR(msg...) logError("http1.HttpConnection", msg)

#ifndef NDEBUG
#define TRACE(msg...) logTrace("http1.HttpConnection", msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

HttpConnection::HttpConnection(EndPoint* endpoint,
                               Executor* executor,
                               const HttpHandler& handler,
                               HttpDateGenerator* dateGenerator,
                               HttpOutputCompressor* outputCompressor,
                               size_t maxRequestUriLength,
                               size_t maxRequestBodyLength,
                               size_t maxRequestCount,
                               TimeSpan maxKeepAlive)
    : HttpTransport(endpoint, executor),
      parser_(HttpParser::REQUEST),
      inputBuffer_(),
      inputOffset_(0),
      writer_(),
      onComplete_(),
      generator_(dateGenerator, &writer_),
      channel_(new Http1Channel(
          this, handler, std::unique_ptr<HttpInput>(new HttpBufferedInput()),
          maxRequestUriLength, maxRequestBodyLength, outputCompressor)),
      maxKeepAlive_(maxKeepAlive),
      requestCount_(0),
      requestMax_(maxRequestCount) {

  parser_.setListener(channel_.get());
  TRACE("%p ctor", this);
}

HttpConnection::~HttpConnection() {
  TRACE("%p dtor", this);
}

void HttpConnection::onOpen() {
  TRACE("%p onOpen", this);
  HttpTransport::onOpen();

  // TODO support TCP_DEFER_ACCEPT here
#if 0
  if (connector()->deferAccept())
    onFillable();
  else
    wantFill();
#else
  wantFill();
#endif
}

void HttpConnection::onClose() {
  TRACE("%p onClose", this);
  HttpTransport::onClose();
}

void HttpConnection::abort() {
  TRACE("%p abort", this);
  endpoint()->close();
}

void HttpConnection::completed() {
  TRACE("%p completed", this);

  if (onComplete_)
    RAISE(IllegalStateError, "there is still another completion hook.");

  if (!generator_.isChunked() && generator_.pendingContentLength() > 0)
    RAISE(IllegalStateError, "Invalid State. Response not fully written but completed() invoked.");

  onComplete_ = std::bind(&HttpConnection::onResponseComplete, this,
                          std::placeholders::_1);

  generator_.generateTrailer(channel_->response()->trailers());
  wantFlush();
}

void HttpConnection::onResponseComplete(bool succeed) {
  if (!succeed) {
    // writing trailer failed. do not attempt to do anything on the wire.
    return;
  }

  if (channel_->isPersistent()) {
    TRACE("%p completed.onComplete", this);
    // re-use on keep-alive
    channel_->reset();

    if (endpoint()->isCorking()) {
      endpoint()->setCorking(false);
    }

    if (inputOffset_ < inputBuffer_.size()) {
      // have some request pipelined
      TRACE("%p completed.onComplete: pipelined read", this);
      executor()->execute(std::bind(&HttpConnection::parseFragment, this));
    } else {
      // wait for next request
      TRACE("%p completed.onComplete: keep-alive read", this);
      wantFill();
    }
  } else {
    endpoint()->close();
  }
}

void HttpConnection::send(HttpResponseInfo&& responseInfo,
                          const BufferRef& chunk,
                          CompletionHandler onComplete) {
  if (onComplete && onComplete_)
    // "there is still another completion hook."
    RAISE(IllegalStateError);

  TRACE("%p send(BufferRef, status=%d, persistent=%s, chunkSize=%zu)",
        this, responseInfo.status(), channel_->isPersistent() ? "yes" : "no",
        chunk.size());

  patchResponseInfo(responseInfo);

  const bool corking_ = true;  // TODO(TCP_CORK): part of HttpResponseInfo?
  if (corking_)
    endpoint()->setCorking(true);

  generator_.generateResponse(responseInfo, chunk);
  onComplete_ = std::move(onComplete);
  wantFlush();
}

void HttpConnection::send(HttpResponseInfo&& responseInfo,
                          Buffer&& chunk,
                          CompletionHandler onComplete) {
  if (onComplete && onComplete_)
    // "there is still another completion hook."
    RAISE(IllegalStateError);

  TRACE("%p send(Buffer, status=%d, persistent=%s, chunkSize=%zu)",
        this, responseInfo.status(), channel_->isPersistent() ? "yes" : "no",
        chunk.size());

  patchResponseInfo(responseInfo);

  const bool corking_ = true;  // TODO(TCP_CORK): part of HttpResponseInfo?
  if (corking_)
    endpoint()->setCorking(true);

  generator_.generateResponse(responseInfo, std::move(chunk));
  onComplete_ = std::move(onComplete);
  wantFlush();
}

void HttpConnection::send(HttpResponseInfo&& responseInfo,
                          FileRef&& chunk,
                          CompletionHandler onComplete) {
  if (onComplete && onComplete_)
    // "there is still another completion hook."
    RAISE(IllegalStateError);

  TRACE("%p send(FileRef, status=%d, persistent=%s, fileRef.fd=%d, chunkSize=%zu)",
        this, responseInfo.status(), channel_->isPersistent() ? "yes" : "no",
        chunk.handle(), chunk.size());

  patchResponseInfo(responseInfo);

  const bool corking_ = true;  // TODO(TCP_CORK): part of HttpResponseInfo?
  if (corking_)
    endpoint()->setCorking(true);

  generator_.generateResponse(responseInfo, std::move(chunk));
  onComplete_ = std::move(onComplete);
  wantFlush();
}

void HttpConnection::patchResponseInfo(HttpResponseInfo& responseInfo) {
  if (static_cast<int>(responseInfo.status()) >= 200) {
    // patch in HTTP transport-layer headers
    if (channel_->isPersistent() && requestCount_ < requestMax_) {
      ++requestCount_;

      char keepAlive[64];
      snprintf(keepAlive, sizeof(keepAlive), "timeout=%lu, max=%zu",
               maxKeepAlive_.totalSeconds(), requestMax_ - requestCount_);

      responseInfo.headers().push_back("Connection", "Keep-Alive");
      responseInfo.headers().push_back("Keep-Alive", keepAlive);
    } else {
      channel_->setPersistent(false);
      responseInfo.headers().push_back("Connection", "closed");
    }
  }
}

void HttpConnection::send(Buffer&& chunk, CompletionHandler onComplete) {
  if (onComplete && onComplete_)
    // "there is still another completion hook."
    RAISE(IllegalStateError);

  TRACE("%p send(Buffer, chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(std::move(chunk));
  onComplete_ = std::move(onComplete);

  wantFlush();
}

void HttpConnection::send(const BufferRef& chunk,
                          CompletionHandler onComplete) {
  if (onComplete && onComplete_)
    // "there is still another completion hook."
    RAISE(IllegalStateError);

  TRACE("%p send(BufferRef, chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(chunk);
  onComplete_ = std::move(onComplete);

  wantFlush();
}

void HttpConnection::send(FileRef&& chunk, CompletionHandler onComplete) {
  if (onComplete && onComplete_)
    // "there is still another completion hook."
    RAISE(IllegalStateError);

  TRACE("%p send(FileRef, chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(std::move(chunk));
  onComplete_ = std::move(onComplete);
  wantFlush();
}

void HttpConnection::setInputBufferSize(size_t size) {
  TRACE("%p setInputBufferSize(%zu)", this, size);
  inputBuffer_.reserve(size);
}

void HttpConnection::onFillable() {
  TRACE("%p onFillable", this);

  TRACE("%p onFillable: calling fill()", this);
  if (endpoint()->fill(&inputBuffer_) == 0) {
    TRACE("%p onFillable: fill() returned 0", this);
    // RAISE("client EOF");
    abort();
    return;
  }

  parseFragment();
}

void HttpConnection::parseFragment() {
  try {
    TRACE("parseFragment: calling parseFragment (%zu into %zu)",
          inputOffset_, inputBuffer_.size());
    size_t n = parser_.parseFragment(inputBuffer_.ref(inputOffset_));
    TRACE("parseFragment: called (%zu into %zu) => %zu",
          inputOffset_, inputBuffer_.size(), n);
    inputOffset_ += n;
  } catch (const BadMessage& e) {
    TRACE("%p parseFragment: BadMessage caught. %s", this, e.what());
    channel_->response()->sendError(e.httpCode(), e.what());
  }
}

void HttpConnection::onFlushable() {
  TRACE("%p onFlushable", this);

  if (channel_->state() != HttpChannelState::SENDING)
    channel_->setState(HttpChannelState::SENDING);

  const bool complete = writer_.flush(endpoint());

  if (complete) {
    TRACE("%p onFlushable: completed. (%s)", this, (onComplete_ ? "onComplete cb set" : "onComplete cb not set"));
    channel_->setState(HttpChannelState::HANDLING);

    if (onComplete_) {
      TRACE("%p onFlushable: invoking completion callback", this);
      auto callback = std::move(onComplete_);
      onComplete_ = nullptr;
      callback(true);
    }
  } else {
    // continue flushing as we still have data pending
    wantFlush();
  }
}

void HttpConnection::onInterestFailure(const std::exception& error) {
  TRACE("%p onInterestFailure(%s): %s", this, typeid(error).name(), error.what());

  // TODO: improve logging here, as this eats our exception here.
  // e.g. via (factory or connector)->error(error);
  logError("HttpConnection", error);

  auto callback = std::move(onComplete_);
  onComplete_ = nullptr;

  // notify the callback that we failed doing something wrt. I/O.
  if (callback) {
    TRACE("%p onInterestFailure: invoking onComplete(false)", this);
    callback(false);
  }

  abort();
}

}  // namespace http1
}  // namespace xzero
