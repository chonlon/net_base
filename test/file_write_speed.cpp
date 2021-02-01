#include <base/print_helper.h>
#include <base/chrono_helper.h>
#include <fmt/format.h>
#include <fmt/compile.h>
#include <fmt/os.h>
#include <fstream>
#include <functional>
#include <sstream>
#include <thread>
#include <fmt/format-inl.h>
#include "base/file.h"

#if WIN32
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

using namespace  lon;


const char write_str[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$ % &\'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c";
constexpr int size = sizeof(write_str) - 1;
constexpr int loop_time = 1000000;

void dataMeasure(std::function<size_t(void)> writer) {

    size_t time_span;
    size_t total_size = 0;
    {
        lon::measure::GetTimeSpan<> time_span_getter(&time_span);
        total_size = writer();
    }

    fmt::print("write {} bytes in {} ms, speed: {:.3f}Mib/s\n", total_size, time_span, static_cast<double>(total_size) / 1024 / 1024 / static_cast<double>(time_span) * 1000);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

size_t fmtWrite() {
    const char* filename = "/tmp/speed_fmt.txt";
    // auto out = fmt::output_file(filename);
    FILE* fptr = fopen(filename, "w");
    for (int i = 0; i < loop_time; ++i) {
        fmt::print(fptr, "{}", write_str);
        // out.print("{}", write_str);
    }
    return loop_time * size;
}

size_t stdWrite() {
    const char* filename = "/tmp/speed_std.txt";
    std::ofstream _ofstream(filename, std::ios::trunc);
    for (int i = 0; i < loop_time; ++i) {
        _ofstream << write_str;
    }
    return loop_time * size;
}

size_t cWrite() {
    const char* filename = "/tmp/speed_c.txt";
    FILE* fptr = fopen(filename, "w");
    for (int i = 0; i < loop_time; ++i) {
        fwrite(write_str, size, 1, fptr);
    }
    return loop_time * size;
}

#if WIN32
void windowsApiWrite() {
    const char* filename = "/tmp/speed_raw.txt";

    HANDLE hFile;
    DWORD dwBytesWritten = 0;
    BOOL bErrorFlag = FALSE;

    printf("\n");

    hFile = CreateFile(filename,                // name of the write
        GENERIC_WRITE,          // open for writing
        0,                      // do not share
        NULL,                   // default security
        CREATE_ALWAYS,          // create new file only
        FILE_ATTRIBUTE_NORMAL,  // normal file
        NULL);                  // no attr. template

    if (hFile == INVALID_HANDLE_VALUE)
    {
        fmt::print("create file error\n");
        return;
    }

    char buffer[1024 * 4]{};
    int buffered_size = 0;
    for (int i = 0; i < loop_time; ++i) {
        if (buffered_size + size <= 1024 * 4) {
            //sprintf(buffer + buffered_size, "%s", )
            strcpy(buffer + buffered_size, write_str);
            buffered_size += size;
        }
        else {
            bErrorFlag = WriteFile(
                hFile,           // open file handle
                buffer,      // start of data to write
                buffered_size,  // number of bytes to write
                &dwBytesWritten, // number of bytes that were written
                NULL);            // no overlapped structure
            strcpy(buffer, write_str);
            buffered_size = size;

        }

    }



    if (FALSE == bErrorFlag)
    {
        fmt::print("write error\n");
        printf("Terminal failure: Unable to write to file.\n");
    }

    CloseHandle(hFile);
}
#else
void unixApiWrite() {
    const char* filename = "/tmp/speed_raw.txt";

    int fd = ::open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    char buffer[1024 * 4]{};
    int buffered_size = 0;
    for (int i = 0; i < loop_time; ++i) {
        if (buffered_size + size <= 1024 * 4) {
            //sprintf(buffer + buffered_size, "%s", )ls
            strcpy(buffer + buffered_size, write_str);
            buffered_size += size;
        }
        else {
            ssize_t rst = ::write(fd, buffer, buffered_size);
            if(rst == -1) fmt::system_error(errno, "write failed");
            strcpy(buffer, write_str);
            buffered_size = size;

        }

    }
}
#endif

size_t rawWrite() {
#ifdef WIN32
    windowsApiWrite();
#else
    unixApiWrite();
#endif
    return loop_time * size;
}

size_t sstreamWrite() {
    std::stringstream ss;
    for (int i = 0; i < loop_time; ++i) {
        ss << write_str;
    }
    return loop_time * size;
}

size_t fmtToWrite() {
    std::string s;

    for (int i = 0; i < loop_time; ++i) {
        fmt::format_to(std::back_inserter(s), "{}", write_str);
    }
    return loop_time * size;
}


size_t mineWrite() {
    const char* filename = "/tmp/mine.txt";
    lon::WritableFile f(filename);
    for (int i = 0; i < loop_time; ++i) {
        [[maybe_unused]] bool result = f.append(write_str);
    }
    return loop_time * size;
}


int main() {
#define LON_XX(name) \
    { \
        CaseMarker cm(#name " "); \
        dataMeasure(&name##Write); \
    }

    LON_XX(fmt)
        LON_XX(std)
        LON_XX(c)
        LON_XX(raw)
        LON_XX(mine)
        LON_XX(fmtTo)
        LON_XX(sstream)
#undef LON_XX
        return 0;
}
