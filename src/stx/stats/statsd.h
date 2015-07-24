/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/buffer.h>
#include <stx/net/udpserver.h>
#include <stx/thread/taskscheduler.h>

namespace fnord {
namespace statsd {

class StatsdServer {
public:

  StatsdServer(
      fnord::TaskScheduler* server_scheduler,
      fnord::TaskScheduler* work_scheduler);

  void listen(int port);

  void onSample(std::function<void (
      const std::string&,
      double,
      const std::vector<std::pair<std::string, std::string>>&)> callback);

  static char const* parseStatsdSample(
      char const* begin,
      char const* end,
      std::string* key,
      std::string* value,
      std::vector<std::pair<std::string, std::string>>* labels);

protected:

  void messageReceived(const fnord::Buffer& msg);

  fnord::net::UDPServer udp_server_;

  std::function<void (
      const std::string&,
      double,
      const std::vector<std::pair<std::string, std::string>>&)> callback_;
};


}
}
