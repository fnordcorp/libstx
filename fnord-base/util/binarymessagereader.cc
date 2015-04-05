/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/exception.h>

namespace fnord {
namespace util {

BinaryMessageReader::BinaryMessageReader(
    void const* buf,
    size_t buf_len) :
    ptr_(buf),
    size_(buf_len),
    pos_(0) {}

uint8_t const* BinaryMessageReader::readUInt8() {
  return static_cast<uint8_t const*>(read(sizeof(uint8_t)));
}

uint16_t const* BinaryMessageReader::readUInt16() {
  return static_cast<uint16_t const*>(read(sizeof(uint16_t)));
}

uint32_t const* BinaryMessageReader::readUInt32() {
  return static_cast<uint32_t const*>(read(sizeof(uint32_t)));
}

uint64_t const* BinaryMessageReader::readUInt64() {
  return static_cast<uint64_t const*>(read(sizeof(uint64_t)));
}

void const* BinaryMessageReader::read(size_t size) {
  return static_cast<void const*>(readString(size));
}

template <>
uint16_t const* BinaryMessageReader::readValue<uint16_t>() {
  return readUInt16();
}

template <>
uint32_t const* BinaryMessageReader::readValue<uint32_t>() {
  return readUInt32();
}

char const* BinaryMessageReader::readString(size_t size) {
#ifndef FNORD_NODBEUG
  if ((pos_ + size) > size_) {
    RAISE(kBufferOverflowError, "requested read exceeds message bounds");
  }
#endif

  auto ptr = static_cast<char const*>(ptr_) + pos_;
  pos_ += size;
  return ptr;
}

void BinaryMessageReader::rewind() {
  pos_ = 0;
}

void BinaryMessageReader::seekTo(size_t pos) {
#ifndef FNORD_NODBEUG
  if (pos > size_) {
    RAISE(kBufferOverflowError, "requested position exceeds message bounds");
  }
#endif

  pos_ = pos;
}

size_t BinaryMessageReader::remaining() const {
  return size_ - pos_;
}

}
}

