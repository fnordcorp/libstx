/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/analysis/LetterTokenizer.h"
#include "fnord-fts/util/MiscUtils.h"
#include "fnord-fts/util/UnicodeUtils.h"

namespace fnord {
namespace fts {

LetterTokenizer::LetterTokenizer(const ReaderPtr& input) : CharTokenizer(input) {
}

LetterTokenizer::LetterTokenizer(const AttributeSourcePtr& source, const ReaderPtr& input) : CharTokenizer(source, input) {
}

LetterTokenizer::LetterTokenizer(const AttributeFactoryPtr& factory, const ReaderPtr& input) : CharTokenizer(factory, input) {
}

LetterTokenizer::~LetterTokenizer() {
}

bool LetterTokenizer::isTokenChar(wchar_t c) {
    return UnicodeUtil::isAlpha(c);
}

}

}
