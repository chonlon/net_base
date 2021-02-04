#pragma once
#include "base/file.h"
#include "base/nocopyable.h"
#include "base/singleton.h"
#include "base/typedef.h"

#include <iostream>
#include <queue>

#define LON_USING_C_FILE 0


namespace lon {

namespace log {


class LogFlusher : public Noncopyable
{
public:
    virtual ~LogFlusher() = default;

    /**
     * \brief 如果是同步输出, flush可能阻塞在write文件上, 如果是异步输出, flush可能出现锁争夺, 对于超高频率写日志情况应使用异步方式
     * \param str 输出至log文件
     */
    virtual void flush(const String& str) = 0;

    virtual void setFilePattern(const String& pattern) = 0;

protected:
    virtual void setFileName(const char* filename) = 0;
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
    AsyncFlusher() = default;

    ~AsyncFlusher() override {
    }

    void flush(const String& str) override {
        {
            std::lock_guard<Mutex> locker{mutex_};
            log_pool_.push(str);
        }
        condition_var_.notify_one();
    }

    virtual size_t size() {
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


class LogStdoutFlusher : public LogFlusher
{
};

class LogFileFlusher : public LogFlusher
{
public:
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

    void setFilePattern(const String& pattern) override {
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
public:
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

private:
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
                    std::unique_lock<Mutex> locker{mutex_};
                    condition_var_.wait(locker);
                }

                doFlush();
            }
        });
    }

    ~AsyncFileLogFlusher() override {
        {
            std::lock_guard<Mutex> locker{mutex_};
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

class _LogFlusherManager
{
public:
    using MakerFunc = std::function<std::unique_ptr<LogFlusher>(
        const char* filename)>;

    _LogFlusherManager() {
#define LON_XX(name) \
            flushers_maker_.emplace(#name, [](const char* filename){ return std::make_unique<name>(filename); });
        LON_XX(SimpleFileFlusher)
        LON_XX(ProtectedFileFlusher)
        LON_XX(AsyncFileLogFlusher)
#undef LON_XX
#define LON_XX(name) \
            flushers_maker_.emplace(#name, []([[maybe_unused]] const char*  filename){ return std::make_unique<name>(); });
        LON_XX(SimpleStdoutFlusher)
        LON_XX(ProtectedStdoutFlusher)
        LON_XX(AsyncStdoutLogFlusher)
#undef LON_XX
    }

    /**
     * @brief Get the Log Flusher object
     *
     * @param key 大小写不敏感
     * @return std::unique_ptr<LogFlusher> LogFlusher的指针
     */
    std::unique_ptr<LogFlusher> getLogFlusher(const String& key,
                                              const char* filename) {
        String convert;
        std::transform(key.begin(),
                       key.end(),
                       std::back_inserter(convert),
                       ::toupper);
        if (auto iter = flushers_maker_.find(convert); iter != flushers_maker_.
            end())
            return iter->second(filename);
        return nullptr;
    }

    /**
     * @brief 注册Flusher的生成器
     *
     * @param key 大小写不敏感
     * @param func 生成器函数
     * @return true 注册成功
     * @return false 内部有同名key, 注册失败
     */
    bool rigisterFlushMaker(const String& key, MakerFunc func) {
        if (auto iter = flushers_maker_.find(key); iter != flushers_maker_.end()
        )
            return false;
        String convert;
        std::transform(key.begin(),
                       key.end(),
                       std::back_inserter(convert),
                       ::toupper);
        flushers_maker_.emplace(key, std::move(func));
    }

private:
    std::unordered_map<String, MakerFunc> flushers_maker_;
};


using LogFlusherManager = Singleton<_LogFlusherManager>;

}
}

#undef LON_USING_C_FILE
