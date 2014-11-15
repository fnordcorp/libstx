// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/ThreadPool.h>
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
#include <xzero/WallClock.h>
#include <unistd.h>
#include <ev++.h>

void runJob(xzero::HttpRequest* request, xzero::HttpResponse* response, xzero::Executor* context) {
  printf("request. job\n");
  // run the complex stuff here
  xzero::BufferRef body = "Hello, World\n";

  // now respond to the client
  context->execute([=]() {
  printf("request. response\n");
    response->setStatus(xzero::HttpStatus::Ok);
    response->setContentLength(body.size());

    response->output()->write(body, std::bind(&xzero::HttpResponse::completed,
                                              response));
  });
}

int main() {
  // xzero::LogAggregator::get().setLogLevel(xzero::LogLevel::Trace);
  // xzero::LogAggregator::get().setLogTarget(xzero::LogTarget::console());

  ev::loop_ref loop = ev::default_loop(0);
  xzero::support::LibevScheduler scheduler(loop);
  xzero::support::LibevSelector selector(loop);
  xzero::support::LibevClock clock(loop);

  xzero::ThreadPool threaded(16);
  xzero::Server server;
  bool shutdown = false;

  auto inet = server.addConnector<xzero::InetConnector>(
      "http", &scheduler, &scheduler, &selector, &clock,
      xzero::TimeSpan::fromSeconds(30),
      xzero::IPAddress("0.0.0.0"), 3000, 128, true, false);

  auto http = inet->addConnectionFactory<xzero::http1::Http1ConnectionFactory>(
      &clock, 100, 512, 5, xzero::TimeSpan::fromMinutes(3));

  http->setHandler([&](xzero::HttpRequest* request, xzero::HttpResponse* response) {
    printf("request\n");
    threaded.execute(std::bind(&runJob, request, response, &scheduler));
    return true;
  });

  server.start();

  while (true)
    selector.select();

  server.stop();
  return 0;
}
