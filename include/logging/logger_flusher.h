#pragma once
#include "base/file.h"
#include "base/nocopyable.h"
#include "base/typedef.h"

#include <iostream>
#include <queue>

#define LON_USING_C_FILE 0


namespace lon {


class LogFlusher : public Noncopyable
{
public:
    virtual ~LogFlusher() = default;

    /**
     * \brief 如果是同步输出, flush可能阻塞在write文件上, 如果是异步输出, flush可能出现锁争夺, 对于超高频率写日志情况应使用异步方式
     * \param str 输出至log文件
     */
    virtual void flush(const String& str) = 0;
};

class DirectFlusher : public LogFlusher
{
public:
    ~DirectFlusher() override {
    }
};

class AsyncFlusher : public LogFlusher
{
public:
    AsyncFlusher() {
        // thread_ = Thread([this]()
        // {
        //     while (!stop_) {
        //         {
        //             std::unique_lock<Mutex> locker{mutex_};
        //             condition_var_.wait(locker);
        //         }
        //         if(!stop_)
        //             doFlush();
        //     }
        // });
    }

    ~AsyncFlusher() override {
    }

    void flush(const String& str) override {
        {
            std::lock_guard<Mutex> locker{mutex_};
            log_pool_.push(str);
        }
        condition_var_.notify_one();
    }

    size_t size() {
        std::lock_guard<Mutex> locker{mutex_};
        return log_pool_.size();
    }

protected:
    virtual void doFlush() = 0;
    //TODO max_size?
    bool stop_ = false;
    Mutex mutex_;
    ConditionVar condition_var_; //TODO 由于是单线程, 可以考虑使用atomic+memory_order替代.
    Thread thread_;
    std::queue<std::string> log_pool_;
};


/**
 * \brief 未加锁, 日志输出至标准输出
 */
class SimpleStdoutFlusher : public DirectFlusher
{
public:
    ~SimpleStdoutFlusher() override {
    }

    void flush(const String& str) override {
        std::cout << str;
    }
};

/**
 * \brief 未加锁, 日志输出至指定文件
 */
class SimpleFileFlusher : public DirectFlusher
#if LON_USING_C_FILE
{
public:


    SimpleFileFlusher(const char* filename) {
        file_ = ::fopen(filename, "a");
    }

    ~SimpleFileFlusher() override {
        ::fclose(file_);
    }


    void flush(const String& str) override {
        ::fwrite(str.c_str(), str.size(), 1, file_);
    }

private:
    FILE* file_ = nullptr;
};
#else
{
public:
    SimpleFileFlusher(const char* filename)
        : file_{filename} {
    }

    ~SimpleFileFlusher() override {
    }

    void flush(const String& str) override {
        file_.append(str);
    }

private:
    WritableFile file_;
};

#endif

/**
 * \brief 加锁, 日志输出至标准输出
 */
class ProtectedStdoutFlusher : public DirectFlusher
{
public:
    ~ProtectedStdoutFlusher() override {
    }

    void flush(const String& str) override {
        std::lock_guard<Mutex> locker{mutex_};
        std::cout << str;
    }

private:
    Mutex mutex_{};
};


/**
 * \brief 未加锁, 日志输出至指定文件
 */
class ProtectedFileFlusher : public DirectFlusher
#if LON_USING_C_FILE
{
public:
    ProtectedFileFlusher(const char* filename) {
        file_ = ::fopen(filename, "a");
    }

    ~ProtectedFileFlusher() override {
        ::fclose(file_);
    }


    void flush(const String& str) override {
        std::lock_guard<Mutex> locker{ mutex_ };
        ::fwrite(str.c_str(), str.size(), 1, file_);
    }
private:
    FILE* file_ = nullptr;
    Mutex mutex_{};
};

#else
{
public:
    ProtectedFileFlusher(const char* filename)
        : file_{filename} {
    }

    ~ProtectedFileFlusher() override {
    }

    void flush(const String& str) override {
        std::lock_guard<Mutex> locker{mutex_};
        file_.append(str);
    }

private:
    WritableFile file_;
    Mutex mutex_{};
};
#endif

class AsyncStdoutLogFlusher : public AsyncFlusher
{
private:
    AsyncStdoutLogFlusher() {
        thread_ = Thread([this]()
        {
            while (!stop_) {
                {
                    std::unique_lock<Mutex> locker{mutex_};
                    condition_var_.wait(locker);
                }

                doFlush();
            }
        });
    }

    ~AsyncStdoutLogFlusher() override {
        {
            std::lock_guard<Mutex> locker{mutex_};
            stop_ = true;
        }

        condition_var_.notify_one();
        thread_.join();
    }

    void doFlush() override {
        std::lock_guard<Mutex> locker{mutex_};
        while (!log_pool_.empty()) {
            std::cout << log_pool_.front();
            log_pool_.pop();
        }
    }
};


class AsyncFileLogFlusher : public AsyncFlusher
#if LON_USING_C_FILE
{
public:
    AsyncFileLogFlusher(const char* filename) {
        file_ = ::fopen(filename, "a");

        thread_ = Thread([this]()
            {
                while (!stop_) {
                    {
                        std::unique_lock<Mutex> locker{ mutex_ };
                        condition_var_.wait(locker);
                    }

                    doFlush();
                }
            });
    }

    ~AsyncFileLogFlusher() override {
        ::fclose(file_);

        {
            std::lock_guard<Mutex> locker{ mutex_ };
            stop_ = true;
        }

        condition_var_.notify_one();
        thread_.join();
    }
private:
    void doFlush() override {
        std::lock_guard<Mutex> locker{ mutex_ };
        while (!log_pool_.empty()) {
            auto log = log_pool_.front();
            ::fwrite(log.c_str(), log.size(), 1, file_);
            log_pool_.pop();
        }
    }
private:
    FILE* file_ = nullptr;
};
#else
{
public:
    AsyncFileLogFlusher(const char* filename)
        : file_{filename} {
        thread_ = Thread([this]()
            {
                while (!stop_) {
                    {
                        std::unique_lock<Mutex> locker{ mutex_ };
                        condition_var_.wait(locker);
                    }

                    doFlush();
                }
            });
    }

    ~AsyncFileLogFlusher() override {
        {
            std::lock_guard<Mutex> locker{ mutex_ };
            stop_ = true;
        }

        condition_var_.notify_one();
        thread_.join();
    }

private:
    void doFlush() override {
        std::lock_guard<Mutex> locker{mutex_};
        while (!log_pool_.empty()) {
            file_.append(log_pool_.front());
            log_pool_.pop();
        }
    }

    WritableFile file_;
};
#endif

}

#undef LON_USING_C_FILE
