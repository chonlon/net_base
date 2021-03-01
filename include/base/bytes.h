#pragma once
#include <cstddef>
#include <cstdint>
#include <bits/stringfwd.h>
#include <string>

namespace lon {
    static std::string toHex(const uint8_t* src, std::size_t len) {
        static const char* const lut = "0123456789abcdef";
        std::string out(len * 2, 0);
        for (std::size_t i = 0; i < len; i++) {
            const unsigned char c = src[i];
            out[i * 2 + 0] = lut[c >> 4];
            out[i * 2 + 1] = lut[c & 15];
        }
        return out;
    }
}
