#pragma once

#include "expection.h"
#include "base/typedef.h"
#include "base/macro.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <fmt/format.h>


namespace lon {
constexpr int G_FileBufferSize = 65535;

#if defined(LON_HAVE_O_CLOEXEC)
constexpr const int G_OpenBaseFlags = O_CLOEXEC;
#else
constexpr const int G_OpenBaseFlags = 0;
#endif  // defined(HAVE_O_CLOEXEC)

class WritableFile
{
public:
    bool append(StringPiece str) {
        size_t write_len       = str.size();
        const char* write_data = str.data();

        size_t copy_size = std::min(write_len, G_FileBufferSize - pos_);
        std::memcpy(buf_ + pos_, write_data, copy_size);
        write_data += copy_size;
        write_len -= copy_size;
        pos_ += copy_size;
        if (write_len == 0) {
            return true;
        }

        if (!flushBuffer()) {
            return false;
        }

        // Small writes go to buffer, large writes are written directly.
        if (write_len < G_FileBufferSize) {
            std::memcpy(buf_, write_data, write_len);
            pos_ = write_len;
            return true;
        }
        return WritableFile::write(write_data, write_len);
    }

    LON_NODISCARD bool closeFile() {
        bool result = flushBuffer();
        const int close_result = ::close(fd_);
        if(close_result < 0) {
            return false;
        }
        return result;
    }


    WritableFile(const String& _file_name, int o_flags = O_APPEND | O_WRONLY | O_CREAT | G_OpenBaseFlags)
        : pos_{0},
          fd_{-1},
          file_name_{_file_name} {
        fd_ = ::open(file_name_.c_str(), o_flags, 0644);
        if(fd_ < 0) {
            throw ExecFailed(fmt::format("exec open failed with err:{}", strerror(errno)));
        }
    }

    ~WritableFile() {
        
        if(fd_ >= 0)
            [[maybe_unused]]bool result = closeFile();
    }

private:
    bool flushBuffer() {
        bool result = WritableFile::write(buf_, pos_);
        pos_        = 0;
        return result;
    }

    bool write(const char* buf, size_t size) {
        while (size > 0) {
            ssize_t write_result = ::write(fd_, buf, size);
            if (write_result < 0) {
                if (errno == EINTR) {
                    continue; // Retry
                } else {
                    std::cerr << "file write failed with: "<< std::strerror(errno);
                    return false;
                }
            }
            buf += write_result;
            size -= write_result;
        }
        return true;
    }

private:
    char buf_[G_FileBufferSize]{};
    size_t pos_;
    int fd_;
    String file_name_;
};


}
