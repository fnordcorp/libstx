// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero-base/logging/LogTarget.h>
#include <unordered_map>
#include <mutex>
#include <stdio.h>
#include <stdarg.h>

namespace xzero {

// {{{ ConsoleLogger
class ConsoleLogger : public LogTarget {
 public:
  void trace(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [trace] %s\n", threadId(), msg.c_str());
  }

  void debug(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [debug] %s\n", threadId(), msg.c_str());
  }

  void info(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [info] %s\n", threadId(), msg.c_str());
  }

  void warn(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [warning] %s\n", threadId(), msg.c_str());
  }

  void error(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [error] %s\n", threadId(), msg.c_str());
  }

  int threadId() {
    pthread_t tid = pthread_self();
    auto i = threadMap_.find(tid);
    if (i != threadMap_.end()) {
      return i->second;
    }
    threadMap_[tid] = threadMap_.size() + 1;
    return threadMap_.size();
  }

  std::mutex lock_;
  std::unordered_map<pthread_t, int> threadMap_;

  static ConsoleLogger* get();
};

ConsoleLogger* ConsoleLogger::get() {
  static ConsoleLogger instance;
  return &instance;
}
// }}}
// {{{ LogTarget
LogTarget* LogTarget::console() {
  return ConsoleLogger::get();
}

LogTarget* LogTarget::syslog() {
  return nullptr; // TODO
}
// }}}

} // namespace xzero
