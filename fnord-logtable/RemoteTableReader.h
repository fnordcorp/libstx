/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_REMOTETABLEREADER_H
#define _FNORD_LOGTABLE_REMOTETABLEREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-http/HTTPConnectionPool.h>
#include <fnord-logtable/TableSnapshot.h>

namespace fnord {
namespace logtable {

class RemoteTableReader : public RefCounted {
public:

  RemoteTableReader(
      const String& table_name,
      const msg::MessageSchema& schema,
      const URI& uri,
      http::HTTPConnectionPool* http);

  const String& name() const;
  const msg::MessageSchema& schema() const;

  RefPtr<TableSnapshot> getSnapshot();

  size_t fetchRecords(
      const String& replica,
      uint64_t start_sequence,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn);

protected:
  String name_;
  msg::MessageSchema schema_;
  URI uri_;
  http::HTTPConnectionPool* http_;
};

} // namespace logtable
} // namespace fnord

#endif

