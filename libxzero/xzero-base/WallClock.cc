// This file is part of the "libxzero" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libxzero is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <xzero-base/WallClock.h>
#include <xzero-base/DateTime.h>
#include <xzero-base/sysconfig.h>
#include <xzero-base/RuntimeError.h>
#include <sys/time.h>
#include <ctime>

namespace xzero {

class SimpleClock : public WallClock {
 public:
  DateTime get() const override;
};

DateTime SimpleClock::get() const {
  struct timeval tv;
  struct timezone tz;

  int rc = gettimeofday(&tv, &tz);
  if (rc < 0)
    RAISE_ERRNO(errno);

  return DateTime(
      static_cast<double>(tv.tv_sec) +
      TimeSpan::fromMicroseconds(tv.tv_usec).value());
}

#ifdef HAVE_CLOCK_GETTIME
class HighPrecisionClock : public WallClock {
 public:
  explicit HighPrecisionClock(int clkid);
  DateTime get() const override;

 private:
  int clkid_;
};

HighPrecisionClock::HighPrecisionClock(int clkid)
    : clkid_(clkid) {
}

DateTime HighPrecisionClock::get() const {
  timespec ts;
  memset(&ts, 0, sizeof(ts));
  if (clock_gettime(clkid_, &ts) < 0)
    return DateTime(std::time(nullptr));

  return DateTime(
      static_cast<double>(ts.tv_sec) +
      TimeSpan::fromNanoseconds(ts.tv_nsec).value());
}
#endif

WallClock* WallClock::system() {
#ifdef HAVE_CLOCK_GETTIME
  static HighPrecisionClock clock(CLOCK_REALTIME);
  return &clock;
#else
  static SimpleClock bc;
  return &bc;
#endif
}

WallClock* WallClock::monotonic() {
#ifdef HAVE_CLOCK_GETTIME
  static HighPrecisionClock clock(CLOCK_MONOTONIC);
  return &clock;
#else
  static SimpleClock bc;
  return &bc;
#endif
}

} // namespace xzero
