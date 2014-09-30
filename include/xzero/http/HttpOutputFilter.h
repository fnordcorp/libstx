#pragma once

#include <xzero/Api.h>
#include <list>

namespace xzero {

class BufferRef;
class Buffer;

/**
 * Adds an interface to HTTP response output filters.
 *
 * @see HttpOutput
 * @see HttpResponse
 */
class XZERO_API HttpOutputFilter {
 public:
  virtual void filter(const BufferRef& input, Buffer* output) = 0;

  static void applyFilters(const std::list<HttpOutputFilter*>& filters,
                           const BufferRef& input, Buffer* output);
};

} // namespace xzero
