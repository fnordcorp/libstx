/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/analysis/TokenStream.h"

namespace fnord {
namespace fts {

TokenStream::TokenStream() {}

TokenStream::TokenStream(
    const AttributeSourcePtr& input) :
    AttributeSource(input) {}

TokenStream::TokenStream(
    const AttributeFactoryPtr& factory) :
    AttributeSource(factory) {}

TokenStream::~TokenStream() {}

void TokenStream::end() {}

void TokenStream::reset() {}

void TokenStream::close() {}

bool TokenStream::incrementToken() {
  return false;
}

void TokenStream::addToken(const fnord::String token) {
}

}
}
