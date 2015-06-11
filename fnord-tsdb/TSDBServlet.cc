/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-base/util/binarymessagewriter.h"
#include "fnord-tsdb/TSDBServlet.h"
#include "fnord-tsdb/RecordEnvelope.pb.h"
#include "fnord-json/json.h"
#include <fnord-base/wallclock.h>
#include <fnord-base/thread/wakeup.h>
#include "fnord-msg/MessageEncoder.h"
#include "fnord-msg/MessagePrinter.h"
#include "fnord-msg/msg.h"
#include <fnord-base/util/Base64.h>
#include <fnord-sstable/sstablereader.h>

using namespace fnord;

namespace tsdb {

TSDBServlet::TSDBServlet(TSDBNode* node) : node_(node) {}

void TSDBServlet::handleHTTPRequest(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  req_stream->readBody();
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);
  res.addHeader("Access-Control-Allow-Origin", "*");

  try {
    auto path_parts = StringUtil::split(uri.path(), "/");
    fnord::iputs("parts: $0", path_parts);
    //if (StringUtil::endsWith(uri.path(), "/insert")) {
    //  insertRecords(&req, &res, &uri);
    //  res_stream->writeResponse(res);
    //  return;
    //}

    //if (StringUtil::endsWith(uri.path(), "/fetch_chunk")) {
    //  fetchChunk(&req, &res, res_stream, &uri);
    //  return;
    //}

    //if (StringUtil::endsWith(uri.path(), "/fetch_partition_info")) {
    //  fetchPartitionInfo(&req, &res, &uri);
    //  res_stream->writeResponse(res);
    //  return;
    //}

    //res.setStatus(fnord::http::kStatusNotFound);
    //res.addBody("not found");
    //res_stream->writeResponse(res);
  } catch (const Exception& e) {
    res.setStatus(http::kStatusInternalServerError);
    res.addBody(StringUtil::format("error: $0: $1", e.getTypeName(), e.getMessage()));
    res_stream->writeResponse(res);
  }
}

//void TSDBServlet::insertRecords(
//    const http::HTTPRequest* req,
//    http::HTTPResponse* res,
//    URI* uri) {
//  auto record_list = msg::decode<RecordEnvelopeList>(req->body());
//
//  // FIXPAUL this is slow, we should group records by partition and then do a
//  // batch insert per partition
//  for (const auto& record : record_list.records()) {
//    auto partition = node_->findOrCreatePartition(
//        tsdb_namespace,
//        record.stream_key(),
//        SHA1Hash::fromString(record.partition_key()));
//
//    partition->insertRecord(
//        SHA1Hash::fromString(record.record_id()),
//        Buffer(record.record().data(), record.record().size());
//  }
//
//  res->setStatus(http::kStatusCreated);
//}

//void TSDBServlet::insertRecordsReplication(
//    const http::HTTPRequest* req,
//    http::HTTPResponse* res,
//    URI* uri) {
//  const auto& params = uri->queryParams();
//
//  String stream;
//  if (!URI::getParam(params, "stream", &stream)) {
//    res->setStatus(fnord::http::kStatusBadRequest);
//    res->addBody("missing ?stream=... parameter");
//    return;
//  }
//
//  String chunk_base64;
//  if (!URI::getParam(params, "chunk", &chunk_base64)) {
//    res->setStatus(fnord::http::kStatusBadRequest);
//    res->addBody("missing ?chunk=... parameter");
//    return;
//  }
//
//  String chunk;
//  util::Base64::decode(chunk_base64, &chunk);
//
//  auto& buf = req->body();
//  util::BinaryMessageReader reader(buf.data(), buf.size());
//  Vector<RecordRef> recs;
//  while (reader.remaining() > 0) {
//    auto record_id = *reader.readUInt64();
//    auto len = reader.readVarUInt();
//    auto data = reader.read(len);
//
//    if (len == 0) {
//      RAISEF(kRuntimeError, "empty record: $0", record_id);
//    }
//
//    recs.emplace_back(RecordRef(record_id, 0, Buffer(data, len)));
//  }
//
//  node_->insertRecords(stream, chunk, recs);
//  res->setStatus(http::kStatusCreated);
//}

//void TSDBServlet::fetchChunk(
//    const http::HTTPRequest* req,
//    http::HTTPResponse* res,
//    RefPtr<http::HTTPResponseStream> res_stream,
//    URI* uri) {
//  const auto& params = uri->queryParams();
//
//  String chunk;
//  if (!URI::getParam(params, "chunk", &chunk)) {
//    res->setStatus(fnord::http::kStatusBadRequest);
//    res->addBody("missing ?chunk=... parameter");
//    res_stream->writeResponse(*res);
//    return;
//  }
//
//  size_t sample_mod = 0;
//  size_t sample_idx = 0;
//  String sample_str;
//  if (URI::getParam(params, "sample", &sample_str)) {
//    auto parts = StringUtil::split(sample_str, ":");
//
//    if (parts.size() != 2) {
//      res->setStatus(fnord::http::kStatusBadRequest);
//      res->addBody("invalid ?sample=... parameter, format is <mod>:<idx>");
//      res_stream->writeResponse(*res);
//    }
//
//    sample_mod = std::stoull(parts[0]);
//    sample_idx = std::stoull(parts[1]);
//  }
//
//
//  String chunk_key;
//  util::Base64::decode(chunk, &chunk_key);
//
//  res->setStatus(http::kStatusOK);
//  res->addHeader("Content-Type", "application/octet-stream");
//  res->addHeader("Connection", "close");
//  res_stream->startResponse(*res);
//
//  auto files = node_->listFiles(chunk_key);
//  for (const auto& f : files) {
//    sstable::SSTableReader reader(f);
//    auto cursor = reader.getCursor();
//
//    while (cursor->valid()) {
//      uint64_t* key;
//      size_t key_size;
//      cursor->getKey((void**) &key, &key_size);
//      if (key_size != sizeof(uint64_t)) {
//        RAISE(kRuntimeError, "invalid row");
//      }
//
//      if (sample_mod == 0 || (*key % sample_mod == sample_idx)) {
//        void* data;
//        size_t data_size;
//        cursor->getData(&data, &data_size);
//
//        util::BinaryMessageWriter buf;
//        if (data_size > 0) {
//          buf.appendUInt64(data_size);
//          buf.append(data, data_size);
//          res_stream->writeBodyChunk(Buffer(buf.data(), buf.size()));
//        }
//        res_stream->waitForReader();
//      }
//
//      if (!cursor->next()) {
//        break;
//      }
//    }
//  }
//
//  util::BinaryMessageWriter buf;
//  buf.appendUInt64(0);
//  res_stream->writeBodyChunk(Buffer(buf.data(), buf.size()));
//
//  res_stream->finishResponse();
//}
//
//void TSDBServlet::fetchPartitionInfo(
//    const http::HTTPRequest* req,
//    http::HTTPResponse* res,
//    URI* uri) {
//  const auto& params = uri->queryParams();
//
//  String partition;
//  if (!URI::getParam(params, "partition", &partition)) {
//    res->setStatus(fnord::http::kStatusBadRequest);
//    res->addBody("missing ?partition=... parameter");
//    return;
//  }
//
//  String partition_key;
//  util::Base64::decode(partition, &partition_key);
//
//  auto pinfo = node_->fetchPartitionInfo(partition_key);
//  res->setStatus(http::kStatusOK);
//  res->addHeader("Content-Type", "application/x-protobuf");
//  res->addBody(*msg::encode(pinfo));
//}

}

