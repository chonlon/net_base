//
// Created by lon on 2021/7/24.
//

#pragma once

#include <cassert>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <type_traits>
#include "../base/typedef.h"
#include "../base/macro.h"

namespace lon {
    namespace log {
        constexpr int LogBufferMaxLen = lon::data::K * 5;


        class LogBufAlloc {
        public:
            LON_NODISCARD inline static
            void *alloc(size_t len) noexcept {
                return malloc(len);
            }

            inline static
            void dealloc(void *buf) noexcept {
                free(buf);
            }
        };

        template<typename Alloc = LogBufAlloc, typename DataType = char>
        struct LogSStreamBuf final {
        public:
            LogSStreamBuf() {
                buf_ = static_cast<char *>(Alloc::alloc(LogBufferMaxLen));
                ::bzero(buf_, LogBufferMaxLen);
            }

            ~LogSStreamBuf() {
                Alloc::dealloc(buf_);
            }

            LON_ALWAYS_INLINE
            void free() noexcept {
                pos_ = 0;
            }

            LON_ALWAYS_INLINE
            char *get() noexcept {
                return buf_;
            }

            char* getTail() noexcept {
                return buf_ + pos_;
            }

            LON_ALWAYS_INLINE
            void append(char c) noexcept {
                assert(pos_ < LogBufferMaxLen);
                auto cur = buf_ + pos_++;
                *cur = c;
            }

            LON_ALWAYS_INLINE
            void append(const char *buf, size_t size) {
                ::memcpy(buf_ + pos_, buf, size);
                pos_ += static_cast<int>(size);
            }

            char *buf_ = nullptr;
            int pos_ = 0;
        };

        class LogSStream final {
        public:
            ~LogSStream();

            LogSStream &operator<<(char c);

            template<typename T, typename ISI= std::enable_if_t<std::is_integral_v<T>>>
            LogSStream &operator<<(const T &n);

            LogSStream &operator<<(double n);

            LogSStream &operator<<(const String &s);

            LogSStream &operator<<(StringPiece slice);

            LogSStream &operator<<(const char* str);

            String str();

            StringPiece getSlice();

        private:
            void append(char c) noexcept;
        };

    }
}