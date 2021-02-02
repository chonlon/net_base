#pragma once
#include "base/expection.h"

namespace lon {
namespace config {
class ConvertFailed : public Exception
{
public:
    ConvertFailed(String what)
        : Exception{what} {
    }
};

}

}
