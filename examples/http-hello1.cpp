// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/DirectExecutor.h>
#include <xzero/net/Server.h>
#include <xzero/net/InetConnector.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/v1/Http1ConnectionFactory.h>
#include <xzero/support/libev/LibevScheduler.h>
#include <xzero/support/libev/LibevSelector.h>
#include <xzero/support/libev/LibevClock.h>
#include <xzero/logging/LogAggregator.h>
#include <xzero/logging/LogTarget.h>
#include <ev++.h>

int main() {
  xzero::LogAggregator::get().setLogLevel(xzero::LogLevel::Trace);
  xzero::LogAggregator::get().setLogTarget(xzero::LogTarget::console());

  ev::loop_ref loop = ev::default_loop(0);
  xzero::support::LibevScheduler scheduler(loop);
  xzero::support::LibevSelector selector(loop);
  xzero::support::LibevClock clock(loop);

  xzero::Server server;

  auto inet = server.addConnector<xzero::InetConnector>(
      "http", &scheduler, &scheduler, &selector, &clock,
      xzero::TimeSpan::fromSeconds(60),
      xzero::IPAddress("0.0.0.0"), 3000, 128, true, false);
  inet->setBlocking(false);

  auto http = inet->addConnectionFactory<xzero::http1::Http1ConnectionFactory>(
      &clock, 100, 512, 5, xzero::TimeSpan::fromMinutes(3));

  http->setHandler([](xzero::HttpRequest* request,
                      xzero::HttpResponse* response) {
    response->setStatus(xzero::HttpStatus::Ok);
    response->setReason("because");

    xzero::Buffer body;
    body << "Hello " << request->path() << "\n";
    response->setContentLength(body.size());
    response->output()->write(std::move(body));
    response->completed();
  });

  server.start();
  selector.select();
  server.stop();
  return 0;
}
