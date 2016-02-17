/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once

namespace stx {
namespace util {

inline uint8_t const* BinaryMessageReader::readUInt8() {
  return static_cast<uint8_t const*>(read(sizeof(uint8_t)));
}

inline uint16_t const* BinaryMessageReader::readUInt16() {
  return static_cast<uint16_t const*>(read(sizeof(uint16_t)));
}

inline uint32_t const* BinaryMessageReader::readUInt32() {
  return static_cast<uint32_t const*>(read(sizeof(uint32_t)));
}

inline uint64_t const* BinaryMessageReader::readUInt64() {
  return static_cast<uint64_t const*>(read(sizeof(uint64_t)));
}

inline void const* BinaryMessageReader::read(size_t size) {
  return static_cast<void const*>(readString(size));
}

inline bool BinaryMessageReader::maybeReadUInt8(uint8_t* val) {
  if (remaining() > sizeof(uint8_t)) {
    *val = *readUInt8();
    return true;
  } else {
    return false;
  }
}

inline bool BinaryMessageReader::maybeReadUInt16(uint16_t* val) {
  if (remaining() > sizeof(uint16_t)) {
    *val = *readUInt16();
    return true;
  } else {
    return false;
  }
}

inline bool BinaryMessageReader::maybeReadUInt32(uint32_t* val) {
  if (remaining() > sizeof(uint32_t)) {
    *val = *readUInt32();
    return true;
  } else {
    return false;
  }
}

inline bool BinaryMessageReader::maybeReadUInt64(uint64_t* val) {
  if (remaining() > sizeof(uint64_t)) {
    *val = *readUInt64();
    return true;
  } else {
    return false;
  }
}

inline uint64_t BinaryMessageReader::readVarUInt() {
  uint64_t value = 0;

  for (int i = 0; ; ++i) {
    auto b = *((const unsigned char*) read(1));

    value |= (b & 0x7fULL) << (7 * i);

    if (!(b & 0x80U)) {
      break;
    }
  }

  return value;
}

inline bool BinaryMessageReader::maybeReadVarUInt(uint64_t* rval) {
  uint64_t value = 0;

  for (int i = 0; ; ++i) {
    if (pos_ >= size_) {
      return false;
    }

    auto b = *((const unsigned char*) read(1));

    value |= (b & 0x7fULL) << (7 * i);

    if (!(b & 0x80U)) {
      break;
    }
  }

  *rval = value;
  return true;
}

inline char const* BinaryMessageReader::readString(size_t size) {
#ifndef NDEBUG
  if ((pos_ + size) > size_) {
    RAISE(kBufferOverflowError, "requested read exceeds message bounds");
  }
#endif

  auto ptr = static_cast<char const*>(ptr_) + pos_;
  pos_ += size;
  return ptr;
}

inline double BinaryMessageReader::readDouble() {
  return IEEE754::fromBytes(
      *static_cast<uint64_t const*>(read(sizeof(uint64_t))));
}

inline bool BinaryMessageReader::maybeReadDouble(double* val) {
  if (remaining() > sizeof(uint64_t)) {
    *val = IEEE754::fromBytes(*readUInt64());
    return true;
  } else {
    return false;
  }
}

inline std::string BinaryMessageReader::readLenencString() {
  auto len = readVarUInt();
  return String((char*) read(len), len);
}

inline bool BinaryMessageReader::maybeReadLenencString(std::string* val) {
  auto old_pos = pos_;

  uint64_t strlen;
  if (!maybeReadVarUInt(&strlen)) {
    return false;
  }

  if (strlen <= remaining()) {
    *val = String((char*) read(strlen), strlen);
    return true;
  } else {
    pos_ = old_pos;
    return false;
  }
}

inline void BinaryMessageReader::rewind() {
  pos_ = 0;
}

inline void BinaryMessageReader::seekTo(size_t pos) {
#ifndef NDEBUG
  if (pos > size_) {
    RAISE(kBufferOverflowError, "requested position exceeds message bounds");
  }
#endif

  pos_ = pos;
}

inline size_t BinaryMessageReader::remaining() const {
  return size_ - pos_;
}

inline size_t BinaryMessageReader::position() const {
  return pos_;
}

template <>
inline uint16_t const* BinaryMessageReader::readValue<uint16_t>() {
  return readUInt16();
}

template <>
inline uint32_t const* BinaryMessageReader::readValue<uint32_t>() {
  return readUInt32();
}

template <>
inline String const* BinaryMessageReader::readValue<String>() {
  auto len = *readUInt32();
  cur_str_ = String(reinterpret_cast<const char*>(read(len)), len);
  return &cur_str_;
}

}
}
