/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "fnord-base/stdtypes.h"
#include "fnord-base/autoref.h"

using namespace fnord;

namespace tsdb {

template <typename _RowType, typename _ParamType, typename _ResultType>
class TSDBTableScanlet : public RefCounted {
public:
  typedef _RowType RowType;
  typedef _ParamType ParamType;
  typedef _ResultType ResultType;

  virtual ~TSDBTableScanlet() {};

  virtual void scan(const RowType& row) = 0;
  virtual void merge(const ResultType& other) = 0;

  virtual Set<String> requiredFields() const {
    return Set<String>{};
  }

  ResultType* result() {
    return &result_;
  }

protected:
  ResultType result_;
};

} // namespace tsdb
