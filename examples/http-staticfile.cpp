// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/DirectExecutor.h>
#include <xzero/executor/NativeScheduler.h>
#include <xzero/WallClock.h>
#include <xzero/net/Server.h>
#include <xzero/net/InetConnector.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/HttpOutputCompressor.h>
#include <xzero/http/HttpFileHandler.h>
#include <xzero/http/v1/Http1ConnectionFactory.h>
#include <xzero/logging/LogTarget.h>
#include <xzero/logging/LogAggregator.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, const char* argv[]) {
  xzero::LogAggregator::get().setLogTarget(xzero::LogTarget::console());
  xzero::LogAggregator::get().setLogLevel(xzero::LogLevel::Trace);

  xzero::NativeScheduler scheduler;
  xzero::WallClock* clock = xzero::WallClock::monotonic();

  std::string docroot = argc == 2 ? argv[1] : ".";
  xzero::Server server;
  auto inet = server.addConnector<xzero::InetConnector>(
      "http", &scheduler, &scheduler, clock,
      xzero::TimeSpan::fromSeconds(30),
      xzero::IPAddress("0.0.0.0"), 3000, 128, true, false);
  auto http = inet->addConnectionFactory<xzero::http1::Http1ConnectionFactory>(
      clock, 100, 512, 5, xzero::TimeSpan::fromMinutes(3));

  xzero::HttpOutputCompressor* compressor = http->outputCompressor();
  compressor->setMinSize(5);

  xzero::HttpFileHandler fileHandler(true, true, true, "/etc/mime.types",
                                     "application/octet-stream");

  http->setHandler([&](xzero::HttpRequest* request, xzero::HttpResponse* response) {
    if (!fileHandler.handle(request, response, docroot)) {
      response->setStatus(xzero::HttpStatus::NotFound);
      response->completed();
    }
  });

  server.start();
  scheduler.runLoop();
  server.stop();
  return 0;
}
