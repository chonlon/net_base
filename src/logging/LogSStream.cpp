//
// Created by lon on 2021/7/24.
//

#include "logging/LogSStream.h"

namespace lon::log {
    thread_local LogSStreamBuf T_log_sstream;


    const char digits[] = "9876543210123456789";
    const char *zero = digits + 9;
    static_assert(sizeof(digits) == 20, "wrong number of digits");

    const char digitsHex[] = "0123456789ABCDEF";
    static_assert(sizeof digitsHex == 17, "wrong number of digitsHex");

// Efficient Integer to String Conversions, by Matthew Wilson.
    template<typename T>
    size_t convert(char buf[], T value) {
        T i = value;
        char *p = buf;

        do {
            int lsd = static_cast<int>(i % 10);
            i /= 10;
            *p++ = zero[lsd];
        } while (i != 0);

        if (value < 0) {
            *p++ = '-';
        }
        *p = '\0';
        std::reverse(buf, p);

        return static_cast<size_t>(p - buf);
    }

    size_t convertHex(char buf[], uintptr_t value) {
        uintptr_t i = value;
        char *p = buf;

        do {
            int lsd = static_cast<int>(i % 16);
            i /= 16;
            *p++ = digitsHex[lsd];
        } while (i != 0);

        *p = '\0';
        std::reverse(buf, p);

        return static_cast<size_t>(p - buf);
    }



    LogSStream::~LogSStream() {
        T_log_sstream.free();
    }

    void LogSStream::append(char c) noexcept {
        T_log_sstream.append(c);
    }

    template<typename T, typename>
    LogSStream &LogSStream::operator<<(const T &n) {
        T_log_sstream.pos_ += static_cast<int>(convert(T_log_sstream.getTail(), n));
        return *this;
    }

    LogSStream &LogSStream::operator<<(const String &s) {
        T_log_sstream.append(s.data(), s.size());
        return *this;
    }

    LogSStream &LogSStream::operator<<(StringPiece slice) {
        T_log_sstream.append(slice.data(), slice.size());
        return *this;
    }

    String LogSStream::str() {
        return lon::String(T_log_sstream.buf_, static_cast<unsigned long>(T_log_sstream.pos_));
    }

    StringPiece LogSStream::getSlice() {
        return lon::StringPiece(T_log_sstream.buf_, static_cast<unsigned long>(T_log_sstream.pos_));
    }

    LogSStream &LogSStream::operator<<(double n) {
        T_log_sstream.pos_ += sprintf(T_log_sstream.getTail(), "%f", n);
        return *this;
    }

    LogSStream &LogSStream::operator<<(char c) {
        append(c);
        return *this;
    }

    LogSStream &LogSStream::operator<<(const char *str) {
        LogSStream::operator<<(StringPiece(str));
        return *this;
    }

    template LogSStream& LogSStream::operator<<(const uint8_t&);
    template LogSStream& LogSStream::operator<<(const int8_t&);
    template LogSStream& LogSStream::operator<<(const uint16_t&);
    template LogSStream& LogSStream::operator<<(const int16_t&);
    template LogSStream& LogSStream::operator<<(const uint32_t&);
    template LogSStream& LogSStream::operator<<(const int32_t&);
    template LogSStream& LogSStream::operator<<(const uint64_t&);
    template LogSStream& LogSStream::operator<<(const int64_t&);

}


