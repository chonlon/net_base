#pragma once
#include <exception>
#include <base/typedef.h>
#include <base/lstring.h>

namespace lon {
    /**
     * \brief Exception base class
     * \exception bad_alloc
     */
    class Exception : public std::exception
    {
    public:
        Exception(String what)
            : what_{what} {
        }

        const char* what() const noexcept override {
            return what_.c_str();   
        }

        ~Exception() = default;
        Exception(const Exception& other) = default;
        Exception(Exception&& other) noexcept = default;
        auto operator=(const Exception& other) -> Exception& = default;
        auto operator=(Exception&& other) noexcept -> Exception& = default;


    protected:
        String what_;
    };

    class ExecFailed : public Exception
    {
    public:
        ExecFailed(String what)
            : Exception{what} {
        }
    };
}
