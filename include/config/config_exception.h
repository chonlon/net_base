#pragma once
#include "../base/expection.h"

namespace lon {
namespace config {

class KeyNotFound : public Exception
{
public:
    KeyNotFound(String what)
        : Exception{what} {
    }
};

class ConvertFailed : public Exception
{
public:
    ConvertFailed(String what)
        : Exception{what} {
    }
};

}

}
