// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/mock/MockTransport.h>
#include <cortex-http/mock/MockInput.h>
#include <cortex-http/HttpHandler.h>
#include <cortex-http/HttpResponseInfo.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpInput.h>
#include <cortex-http/HttpChannel.h>
#include <cortex-http/BadMessage.h>
#include <cortex-base/executor/Executor.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/io/FileRef.h>
#include <stdexcept>
#include <system_error>

namespace cortex {

MockTransport::MockTransport(Executor* executor, const HttpHandler& handler)
    : MockTransport(executor, handler, 32, 64, nullptr) {
}

MockTransport::MockTransport(Executor* executor,
                             const HttpHandler& handler,
                             size_t maxRequestUriLength,
                             size_t maxRequestBodyLength,
                             HttpOutputCompressor* outputCompressor)
    : HttpTransport(nullptr /*endpoint*/, executor),
      handler_(handler),
      maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      outputCompressor_(outputCompressor),
      isAborted_(false),
      isCompleted_(false),
      channel_(),
      responseInfo_(),
      responseBody_() {
}

MockTransport::~MockTransport() {
}

void MockTransport::run(HttpVersion version, const std::string& method,
                        const std::string& entity,
                        const HeaderFieldList& headers,
                        const std::string& body) {
  isCompleted_ = false;
  isAborted_ = false;

  std::unique_ptr<MockInput> input(new MockInput());
  channel_.reset(new HttpChannel(this, handler_, std::move(input),
                                 maxRequestUriLength_, maxRequestBodyLength_,
                                 outputCompressor_));


  try {
    if (!channel_->onMessageBegin(BufferRef(method), BufferRef(entity), version))
      return;

    for (const HeaderField& header: headers) {
      if (!channel_->onMessageHeader(BufferRef(header.name()),
                                    BufferRef(header.value()))) {
        return;
      }
    }

    if (!channel_->onMessageHeaderEnd())
      return;

    if (!channel_->onMessageContent(BufferRef(body.data(), body.size())))
      return;

    channel_->onMessageEnd();
  } catch (const BadMessage& e) {
    channel_->response()->sendError(e.httpCode(), e.what());
  }
}

void MockTransport::abort() {
  isAborted_ = true;
}

void MockTransport::completed() {
  isCompleted_ = true;

  responseInfo_.setTrailers(channel_->response()->trailers());
}

void MockTransport::send(HttpResponseInfo&& responseInfo,
                         const BufferRef& chunk,
                         CompletionHandler onComplete) {
  responseInfo_ = std::move(responseInfo);
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::send(HttpResponseInfo&& responseInfo,
                         Buffer&& chunk,
                         CompletionHandler onComplete) {
  responseInfo_ = std::move(responseInfo);
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::send(HttpResponseInfo&& responseInfo,
                         FileRef&& chunk,
                         CompletionHandler onComplete) {
  responseInfo_ = std::move(responseInfo);

  chunk.fill(&responseBody_);

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::send(const BufferRef& chunk, CompletionHandler onComplete) {
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::send(Buffer&& chunk, CompletionHandler onComplete) {
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::send(FileRef&& chunk, CompletionHandler onComplete) {
  chunk.fill(&responseBody_);

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::onOpen() {
  HttpTransport::onOpen();
}

void MockTransport::onClose() {
  HttpTransport::onClose();
}

void MockTransport::onFillable() {
}

void MockTransport::onFlushable() {
}

bool MockTransport::onReadTimeout() {
  return HttpTransport::onReadTimeout();
}

} // namespace mock
