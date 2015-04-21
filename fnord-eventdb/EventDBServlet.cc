/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-eventdb/EventDBServlet.h"
#include "fnord-json/json.h"

namespace fnord {
namespace eventdb {

EventDBServlet::EventDBServlet(TableRepository* tables) : tables_(tables) {}

void EventDBServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  try {
    if (StringUtil::endsWith(uri.path(), "/insert")) {
      return insertRecord(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/commit")) {
      return commitTable(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/merge")) {
      return mergeTable(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/gc")) {
      return gcTable(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/info")) {
      return tableInfo(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/snapshot")) {
      return tableSnapshot(req, res, &uri);
    }

    res->setStatus(fnord::http::kStatusNotFound);
    res->addBody("not found");
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.getMessage()));
  }
}

void EventDBServlet::insertRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);

  if (tbl->arenaSize() > 50000) {
    res->setStatus(http::kStatusServiceUnavailable);
    res->addBody("too many uncommitted records, retry in a few seconds");
  } else {
    tbl->addRecords(req->body());
    res->setStatus(http::kStatusCreated);
  }
}

void EventDBServlet::commitTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  auto n = tbl->commit();

  res->setStatus(http::kStatusOK);
  res->addBody(StringUtil::toString(n));
}

void EventDBServlet::mergeTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  tbl->merge();

  res->setStatus(http::kStatusOK);
}

void EventDBServlet::gcTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  tbl->gc();

  res->setStatus(http::kStatusOK);
}

void EventDBServlet::tableInfo(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto snap = tables_->getSnapshot(table);

  HashMap<String, uint64_t> per_replica_head;
  HashMap<String, uint64_t> artifacts;
  HashMap<String, HashMap<String, String>> per_replica;
  uint64_t num_recs_commited = 0;
  uint64_t num_recs_arena = 0;

  for (const auto& c : snap->head->chunks) {
    auto chunkname = table + "." + c.replica_id + "." + c.chunk_id;

    num_recs_commited += c.num_records;
    artifacts[chunkname] = c.num_records;

    auto seq = (c.start_sequence + c.num_records) - 1;
    if (seq > per_replica_head[c.replica_id]) {
      per_replica_head[c.replica_id] = seq;
      per_replica[c.replica_id]["head_sequence"] = StringUtil::toString(seq);
    }
  }

  auto my_head = per_replica_head[tables_->replicaID()];
  for (const auto& a : snap->arenas) {
    if (a->startSequence() > my_head) {
      num_recs_arena += a->size();
    }
  }


  Vector<String> missing_chunks;
  uint64_t bytes_total = 0;
  uint64_t bytes_present = 0;
  uint64_t bytes_downloading = 0;
  uint64_t bytes_missing = 0;
  uint64_t num_chunks_present = 0;
  uint64_t num_chunks_downloading = 0;
  uint64_t num_chunks_missing = 0;
  uint64_t num_recs_present = 0;
  uint64_t num_recs_downloading = 0;
  uint64_t num_recs_missing = 0;

  auto artifactlist = tables_->artifactIndex()->listArtifacts();
  for (const auto& a : artifactlist) {
    if (artifacts.count(a.name) > 0) {
      bytes_total += a.totalSize();

      switch (a.status) {

        case ArtifactStatus::PRESENT:
          bytes_present += a.totalSize();
          ++num_chunks_present;
          num_recs_present += artifacts[a.name];
          break;

        case ArtifactStatus::DOWNLOAD:
          bytes_downloading += a.totalSize();
          ++num_chunks_downloading;
          num_recs_downloading += artifacts[a.name];
          break;

        default:
        case ArtifactStatus::MISSING:
          missing_chunks.emplace_back(a.name);
          bytes_missing += a.totalSize();
          ++num_chunks_missing;
          num_recs_missing += artifacts[a.name];
          break;

      }
    }
  }

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream j(res->getBodyOutputStream());

  j.beginObject();
  j.addObjectEntry("table");
  j.addString(table);
  j.addComma();
  j.addObjectEntry("num_records_total");
  j.addInteger(num_recs_commited + num_recs_arena);
  j.addComma();
  j.addObjectEntry("num_records_commited");
  j.addInteger(num_recs_commited);
  j.addComma();
  j.addObjectEntry("num_records_present");
  j.addInteger(num_recs_present);
  j.addComma();
  j.addObjectEntry("num_records_downloading");
  j.addInteger(num_recs_downloading);
  j.addComma();
  j.addObjectEntry("num_records_stage");
  j.addInteger(num_recs_arena);
  j.addComma();
  j.addObjectEntry("num_records_missing");
  j.addInteger(num_recs_missing);
  j.addComma();
  j.addObjectEntry("bytes_total");
  j.addInteger(bytes_total);
  j.addComma();
  j.addObjectEntry("bytes_present");
  j.addInteger(bytes_present);
  j.addComma();
  j.addObjectEntry("bytes_downloading");
  j.addInteger(bytes_downloading);
  j.addComma();
  j.addObjectEntry("bytes_missing");
  j.addInteger(bytes_missing);
  j.addComma();
  j.addObjectEntry("num_chunks");
  j.addInteger(snap->head->chunks.size());
  j.addComma();
  j.addObjectEntry("num_chunks_present");
  j.addInteger(num_chunks_present);
  j.addComma();
  j.addObjectEntry("num_chunks_downloading");
  j.addInteger(num_chunks_downloading);
  j.addComma();
  j.addObjectEntry("num_chunks_missing");
  j.addInteger(num_chunks_missing);
  j.addComma();
  j.addObjectEntry("replicas");
  json::toJSON(per_replica, &j);
  j.addComma();
  j.addObjectEntry("missing_chunks");
  json::toJSON(missing_chunks, &j);
  j.endObject();
}

void EventDBServlet::tableSnapshot(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto snap = tables_->getSnapshot(table);

  Buffer buf;
  snap->head->encode(&buf);

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/octet-stream");
  res->addBody(buf);
}

}
}
