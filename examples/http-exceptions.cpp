// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/DirectExecutor.h>
#include <xzero/executor/NativeScheduler.h>
#include <xzero/net/Server.h>
#include <xzero/net/InetConnector.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/v1/Http1ConnectionFactory.h>
#include <xzero/logging/LogAggregator.h>
#include <xzero/logging/LogTarget.h>
#include <xzero/RuntimeError.h>
#include <xzero/WallClock.h>
#include <xzero/RuntimeError.h>

int main() {
  auto errorHandler = [](const std::exception& e) {
    xzero::consoleLogger(e);
  };

  xzero::WallClock* clock = xzero::WallClock::monotonic();
  xzero::NativeScheduler scheduler(errorHandler, clock);

  scheduler.setExceptionHandler(errorHandler);

  xzero::Server server;
  auto inet = server.addConnector<xzero::InetConnector>(
      "http", &scheduler, &scheduler, clock,
      xzero::TimeSpan::fromSeconds(30),
      &xzero::consoleLogger,
      xzero::IPAddress("0.0.0.0"), 3000, 128, true, false);
  auto http = inet->addConnectionFactory<xzero::http1::Http1ConnectionFactory>(
      clock, 100, 512, 5, xzero::TimeSpan::fromMinutes(3));

  http->setHandler([](xzero::HttpRequest* request,
                      xzero::HttpResponse* response) {
    if (request->path() == "/raise")
      throw RUNTIME_ERROR("maybe raise");

    response->setStatus(xzero::HttpStatus::Ok);
    response->output()->write("Call me maybe /raise ;-)\n",
        std::bind(&xzero::HttpResponse::completed, response));
  });

  try {
    server.start();
    scheduler.runLoop();
    server.stop();
  } catch (...) {
  }
  return 0;
}
