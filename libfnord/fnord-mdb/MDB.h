/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MDB_H
#define _FNORD_MDB_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "fnord-base/logging.h"
#include "fnord-base/autoref.h"
#include "fnord-mdb/MDBTransaction.h"

namespace fnord {
namespace mdb {

class MDB : public RefCounted {
public:

  static RefPtr<MDB> open(
      const String& path,
      bool readonly = false,
      size_t maxsize = 1024 * 1024 * 1024); // 1 GiB

  MDB(const MDB& other) = delete;
  MDB& operator=(const MDB& other) = delete;
  ~MDB();

  RefPtr<MDBTransaction> startTransaction(bool readonly = false);

  void setMaxSize(size_t size);

protected:
  MDB(MDB_env* mdb_env);
  void openDBHandle();

  MDB_env* mdb_env_;
  MDB_dbi mdb_handle_;
};

}
}
#endif
