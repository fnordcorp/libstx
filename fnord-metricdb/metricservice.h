/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_METRIC_SERVICE_H
#define _FNORD_METRIC_SERVICE_H
#include <mutex>
#include <stdlib.h>
#include <string>
#include "fnord-base/datetime.h"
#include "fnord-metricdb/metricrepository.h"
#include "fnord-base/stats/counter.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-tsdb/TSDBClient.h"
#include "fnord-metricdb/Sample.pb.h"

namespace fnord {
namespace metric_service {

class MetricService {
public:

  MetricService(
      const String& tsdb_prefix,
      tsdb::TSDBClient* tsdb);

  /**
   * List all metrics
   */
  std::vector<IMetric*> listMetrics() const;

  /**
   * Insert a sample into a metric
   */
  void insertSample(
      const std::string& metric_key,
      double value,
      const std::vector<std::pair<std::string, std::string>>& labels);

  /**
   * Return all samples for the provided metric and time range
   */
  void scanSamples(
      const std::string& metric_key,
      const DateTime& time_begin,
      const DateTime& time_end,
      Function<void (const metricd::MetricSample& sample)> callback);

  Sample getMostRecentSample(const std::string& metric_key);

protected:

  String tsdb_prefix_;
  tsdb::TSDBClient* tsdb_;
};

} // namespace metric_service
} // namsepace fnord
#endif
