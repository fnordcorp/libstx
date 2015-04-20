/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_ARTIFACTINDEX_H
#define _FNORD_EVENTDB_ARTIFACTINDEX_H
#include <fnord-base/stdtypes.h>

namespace fnord {
namespace eventdb {

enum class ArtifactStatus : uint8_t {
  DOWNLOAD,
  PRESENT,
  MISSING,
  IGNORE,
};

struct ArtifactFileRef {
  String filename;
  uint64_t size;
  uint64_t checksum;
};

struct ArtifactRef {
  String name;
  ArtifactStatus status;
  Vector<Pair<String, String>> attributes;
  Vector<ArtifactFileRef> files;

  size_t totalSize() const;
};

class ArtifactIndex {
public:

  ArtifactIndex(
      const String& db_path,
      const String& index_name,
      bool readonly);

  void withIndex(
      bool readonly,
      Function<void (List<ArtifactRef>* index)> fn);

  void runConsistencyCheck(bool check_checksums = false, bool repair = false);

  List<ArtifactRef> listArtifacts();
  void addArtifact(const ArtifactRef& artifact);
  void updateStatus(const String& artifact_name, ArtifactStatus new_status);
  void deleteArtifact(const String& artifact_name);

  const String& basePath() const;

protected:
  List<ArtifactRef> readIndex();
  void writeIndex(const List<ArtifactRef>& index);

  void statusTransition(ArtifactRef* artifact, ArtifactStatus new_status);

  const String db_path_;
  const String index_name_;
  const bool readonly_;
  const String index_file_;
  const String index_lockfile_;
  std::atomic<bool> exists_;
  List<ArtifactRef> cached_;
  uint64_t cached_mtime_;
  std::mutex cached_mutex_;
  std::mutex mutex_;
};

} // namespace eventdb
} // namespace fnord

#endif
