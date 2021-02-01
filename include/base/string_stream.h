#pragma once
#include <string>
#include "string_piece.h"

namespace lon
{
    class SimpleStringStream {
        // << operators

        std::string toString() { return {}; }
        StringPiece toStringPiece(){ return {}; }
    };
} // namespace lon
