/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DOCTABLE_TABLEREADER_H
#define _FNORD_DOCTABLE_TABLEREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-doctable/TableArena.h>
#include <fnord-doctable/TableSnapshot.h>
#include "fnord-sstable/sstablereader.h"
#include "fnord-cstable/CSTableReader.h"

namespace fnord {
namespace doctable {

class TableReader : public RefCounted {
public:

  static RefPtr<TableReader> open(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema);

  const String& name() const;
  const String& basePath() const;
  const msg::MessageSchema& schema() const;

  RefPtr<TableSnapshot> getSnapshot();

  size_t fetchRecords(
      const String& replica,
      uint64_t start_sequence,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn);

protected:

  TableReader(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema,
      uint64_t head_generation);

  size_t fetchRecords(
      const TableSegmentRef& chunk,
      size_t offset,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn);

  String name_;
  String replica_id_;
  String db_path_;
  msg::MessageSchema schema_;
  std::mutex mutex_;
  uint64_t head_gen_;
};

} // namespace doctable
} // namespace fnord

#endif
