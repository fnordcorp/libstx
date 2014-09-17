#pragma once

#include <xzero/Api.h>
#include <xzero/http/HeaderField.h>
#include <string>
#include <list>

namespace xzero {

/**
 * Represents a list of headers (key/value pairs) for an HTTP message.
 */
class XZERO_API HeaderFieldList {
 public:
  HeaderFieldList() = default;
  HeaderFieldList(HeaderFieldList&&) = default;
  HeaderFieldList(const HeaderFieldList&) = default;
  HeaderFieldList& operator=(HeaderFieldList&&) = default;
  HeaderFieldList& operator=(const HeaderFieldList&) = default;
  HeaderFieldList(const std::initializer_list<std::pair<std::string, std::string>>& init);

  void push_back(const std::string& name, const std::string& value);
  void overwrite(const std::string& name, const std::string& value);
  void append(const std::string& name, const std::string& value);
  void remove(const std::string& name);

  bool empty() const;
  size_t size() const;
  bool contains(const std::string& name) const;
  bool contains(const std::string& name, const std::string& value) const;
  const std::string& get(const std::string& name) const;

  typedef std::list<HeaderField>::iterator iterator;
  typedef std::list<HeaderField>::const_iterator const_iterator;

  iterator begin() { return entries_.begin(); }
  iterator end() { return entries_.end(); }

  const_iterator begin() const { return entries_.cbegin(); }
  const_iterator end() const { return entries_.cend(); }

  const_iterator cbegin() const { return entries_.cbegin(); }
  const_iterator cend() const { return entries_.cend(); }

  /**
   * Completely removes all entries from this header list.
   */
  void reset();

 private:
  std::list<HeaderField> entries_;
};

inline bool HeaderFieldList::empty() const {
  return entries_.empty();
}

inline size_t HeaderFieldList::size() const {
  return entries_.size();
}

}  // namespace xzero
