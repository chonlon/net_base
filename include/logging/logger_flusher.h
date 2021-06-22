#pragma once
#include "../base/file.h"
#include "../base/nocopyable.h"
#include "../base/singleton.h"
#include "../base/typedef.h"
#include "logger_filename.h"


#include <iostream>
#include <queue>

#define LON_USING_C_FILE 0


namespace lon {

namespace log {

// TODO 完成定时器之后, 添加按照时间刷新输出文件功能(包括文件名)
enum class FlushFrequency
{
    Mouth,
    Week,
    Day,
    HalfDay,
    Hour
};

class Flusher : public Noncopyable
{
public:
    virtual ~Flusher() = default;

    /**
     * \brief 如果是同步输出, flush可能阻塞在write文件上, 如果是异步输出,
     * flush可能出现锁争夺, 对于超高频率写日志情况应使用异步方式
     * \param str
     * 输出至log文件
     */
    virtual void flush(const String& str) = 0;


    virtual void setFilePattern(const String& pattern) = 0;

protected:
    virtual void setFileName(const char* filename) = 0;
};

class DirectFlusher
{
public:
    virtual ~DirectFlusher() = default;
};

class AsyncFlusher
{
public:
    AsyncFlusher() = default;

    virtual ~AsyncFlusher() = default;

    void flush(const String& str) {
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
    // TODO max_size?
    bool stop_ = false;
    Mutex mutex_;
    ConditionVar condition_var_;  // TODO 由于是单线程,
                                  // 可以考虑使用atomic+memory_order替代.
    Thread thread_;
    std::queue<std::string> log_pool_;
};


class StdoutFlusher : public Flusher
{
public:
    void setFilePattern(const String& pattern) override {}

protected:
    void setFileName(const char* filename) override {}
};

class FileFlusher : public Flusher
#if LON_USING_C_FILE
{
public:
    FileFlusher(const char* filename) {}

    FileFlusher(const String& pattern) {}

    void setFilePattern(const String& pattern) override {
        filename_ = logFileNameParse(pattern);
    }

protected:
    FILE* file_ = nullptr;
    LogFilenameData filename_;
};
#else
{
public:
    FileFlusher(const char* filename) : file_{} {
        createDir(filename);
        file_ = WritableFile(filename);
    }

    FileFlusher(const String& pattern) : file_{} {
        setFilePattern(pattern);
        String filename = logFileNameGenerate(filename_);
        createDir(filename);
        setFileName(filename.c_str());
    }

    void setFilePattern(const String& pattern) override {
        filename_ = logFileNameParse(pattern);
    }

protected:
    void setFileName(const char* filename) override {
        file_ = WritableFile{filename};
    }

protected:
    WritableFile file_;
    LogFilenameData filename_{};

private:
    static void createDir(const String& fullfilename) {
        size_t last  = fullfilename.find_last_of('/');
        auto dirname = fullfilename.substr(0, last);
        if ((lon::createDir(dirname.c_str())) != 0) {
            std::cerr << fmt::format(
                "create dir err:{} with dir:{}", std::strerror(errno), dirname);
        }
    }
};
#endif

/**
 * \brief 未加锁, 日志输出至标准输出
 */
class SimpleStdoutFlusher
    : public StdoutFlusher
    , public DirectFlusher
{
public:
    ~SimpleStdoutFlusher() override {}

    void flush(const String& str) override {
        std::cout << str;
    }

    void setFilePattern(const String& pattern) override {}
};

/**
 * \brief 未加锁, 日志输出至指定文件
 */
class SimpleFileFlusher
    : public FileFlusher
    , public DirectFlusher
     
{
public:
    SimpleFileFlusher(const char* filename) : FileFlusher{filename} {}

    SimpleFileFlusher(const String& pattern) : FileFlusher{pattern} {}


    ~SimpleFileFlusher() override {}

#if LON_USING_C_FILE
    void flush(const String& str) override {
        ::fwrite(str.c_str(), str.size(), 1, file_);
    }
#else
    void flush(const String& str) override {
        file_.append(str);
    }
#endif
};


/**
 * \brief 加锁, 日志输出至标准输出
 */
class ProtectedStdoutFlusher
    : public StdoutFlusher
    , public DirectFlusher
{
public:
    ~ProtectedStdoutFlusher() override {}

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
class ProtectedFileFlusher
    : public FileFlusher
    , public DirectFlusher

{
public:
    ProtectedFileFlusher(const char* filename) : FileFlusher{filename} {}

    ProtectedFileFlusher(const String& pattern) : FileFlusher{pattern} {}

    ~ProtectedFileFlusher() override {}

#if LON_USING_C_FILE
    void flush(const String& str) override {
        std::lock_guard<Mutex> locker{mutex_};
        ::fwrite(str.c_str(), str.size(), 1, file_);
    }
#else
    void flush(const String& str) override {
        std::lock_guard<Mutex> locker{mutex_};
        file_.append(str);
    }
#endif
private:
    Mutex mutex_{};
};


class AsyncStdoutLogFlusher
    : public StdoutFlusher
    , public AsyncFlusher
{
public:
    AsyncStdoutLogFlusher() {
        thread_ = Thread([this]() {
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

    void flush(const String& str) override {
        AsyncFlusher::flush(str);
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


class AsyncFileLogFlusher
    : public FileFlusher
    , public AsyncFlusher

{
public:
    AsyncFileLogFlusher(const char* filename) : FileFlusher{filename} {
        initThread();
    }

    AsyncFileLogFlusher(const String& pattern) : FileFlusher{pattern} {}


    void flush(const String& str) override {
        AsyncFlusher::flush(str);
    }


#if LON_USING_C_FILE
    ~AsyncFileLogFlusher() override {
        ::fclose(file_);

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
            auto log = log_pool_.front();
            ::fwrite(log.c_str(), log.size(), 1, file_);
            log_pool_.pop();
        }
    }
#else
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
#endif
private:
    void initThread() {
        thread_ = Thread([this]() {
            while (!stop_) {
                {
                    std::unique_lock<Mutex> locker{mutex_};
                    condition_var_.wait(locker);
                }

                doFlush();
            }
        });
    }
};

struct StringFlusher
    : public DirectFlusher
    , public Flusher
{
    String log;

    void flush(const String& str) override {
        log.append(str);
    }
    void setFilePattern(const String& pattern) override {}

protected:
    void setFileName(const char* filename) override {}
};


class _LogFlusherManager
{
public:
    using MakerFunc =
        std::function<std::unique_ptr<Flusher>(const String& pattern)>;

    _LogFlusherManager() {
        //注册已有Flusher

#define LON_XX(name)                                      \
    rigisterFlushMaker(#name, [](const String& pattern) { \
        return std::make_unique<name>(pattern);           \
    });
        LON_XX(SimpleFileFlusher)
        LON_XX(ProtectedFileFlusher)
        LON_XX(AsyncFileLogFlusher)
#undef LON_XX
#define LON_XX(name)                                                       \
    rigisterFlushMaker(#name, []([[maybe_unused]] const String& pattern) { \
        return std::make_unique<name>();                                   \
    });
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
    std::unique_ptr<Flusher> getLogFlusher(const String& key,
                                           const String& pattern) {
        String convert;
        std::transform(
            key.begin(), key.end(), std::back_inserter(convert), ::toupper);
        if (auto iter = flushers_maker_.find(convert);
            iter != flushers_maker_.end())
            return iter->second(pattern);
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
        if (auto iter = flushers_maker_.find(key);
            iter != flushers_maker_.end())
            return false;
        String convert;
        std::transform(
            key.begin(), key.end(), std::back_inserter(convert), ::toupper);
        flushers_maker_.emplace(convert, std::move(func));
        return true;
    }

private:
    std::unordered_map<String, MakerFunc> flushers_maker_;
};


using FlusherManager = Singleton<_LogFlusherManager>;

}  // namespace log
}  // namespace lon

#undef LON_USING_C_FILE
