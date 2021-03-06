#pragma once
#include <string>
#include "lstring.h"

namespace lon
{
    class SimpleStringStream {
        // << operators

        std::string toString() { return {}; }
        StringPiece toStringPiece(){ return {}; }
    };
} // namespace lon
