// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <xzero/Api.h>
#include <xzero/Buffer.h>
#include <cstdint>
#include <unistd.h>

namespace xzero {

/**
 * Basic abstraction of an open file handle, its range, and auto-close feature.
 *
 * This FileRef represents an open file that is to be read starting
 * from the given @c offset up to @c size bytes.
 *
 * If the FileRef was initialized with auto-close set to on, its
 * underlying resource file descriptor will be automatically closed.
 */
struct XZERO_API FileRef {
 private:
  FileRef(const FileRef&) = delete;
  FileRef& operator=(const FileRef&) = delete;

 public:
  /** General move semantics for FileRef(FileRef&&). */
  FileRef(FileRef&& ref)
      : fd_(ref.fd_),
        offset_(ref.offset_),
        size_(ref.size_),
        close_(ref.close_) {
    ref.fd_ = -1;
    ref.close_ = false;
  }

  /** General move semantics for operator=. */
  FileRef& operator=(FileRef&& ref) {
    fd_ = ref.fd_;
    offset_ = ref.offset_;
    size_ = ref.size_;
    close_ = ref.close_;

    ref.fd_ = -1;
    ref.close_ = false;

    return *this;
  }

  /**
   * Initializes given FileRef.
   *
   * @param fd Underlying resource file descriptor.
   * @param offset The offset to start reading from.
   * @param size Number of bytes to read.
   * @param close Whether or not to close the underlying file desriptor upon
   *              object destruction.
   */
  FileRef(int fd, off_t offset, size_t size, bool close)
      : fd_(fd), offset_(offset), size_(size), close_(close) {}

  /**
   * Conditionally closes the underlying resource file descriptor.
   */
  ~FileRef() {
    if (close_) {
      ::close(fd_);
    }
  }

  int handle() const noexcept { return fd_; }

  off_t offset() const noexcept { return offset_; }
  void setOffset(off_t n) { offset_ = n; }

  size_t size() const noexcept { return size_; }
  void setSize(size_t n) { size_ = n; }

  void fill(Buffer* output) const;

 private:
  int fd_;
  off_t offset_;
  size_t size_;
  bool close_;
};

} // namespace xzero
