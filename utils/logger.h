#pragma once

#include "common.h"

#include "concurrentqueue/concurrentqueue.h"

#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>

#define INIT_LOGGER(...) ozma::Logger::init(__VA_ARGS__)

#define INFO() *ozma::Logger::getInstance().log(ozma::Logger::Level::Info)
#define WARN() *ozma::Logger::getInstance().log(ozma::Logger::Level::Warning)
#define ERROR() *ozma::Logger::getInstance().log(ozma::Logger::Level::Error)

namespace ozma {

class Logger {
private:
    class LogStream {
    public:
        explicit LogStream(Logger& logger)
            : logger_(logger) {
        }

        template <typename T>
        LogStream& operator<<(const T& value) {
            ss_ << value;
            return *this;
        }

        ~LogStream() {
            ss_ << '\n';
            logger_.writeMsg(ss_.str());
        }

    private:
        Logger& logger_;
        std::stringstream ss_;
    };

public:
    enum class Level { Info, Warning, Error };
    DECLARE_ENUM(Level, 3, Info, Warning, Error)

    enum class Out { Stdout, File };

    static void init(std::initializer_list<Out> outs);

    std::unique_ptr<LogStream> log(Level level);

    static Logger& getInstance();
    static std::thread& getLogThread();
    static void waitLogThread();

private:
    Logger(std::initializer_list<Out> outs);
    ~Logger();

    void writeMsg(std::string&& msg);
    void logThreadFunc();
    void doWriteMsg(const std::string& msg);

    moodycamel::ConcurrentQueue<std::string> logQueue_;
    std::thread logThread_;
    std::optional<std::ofstream> fileLog_;
    bool stdout_ = false;
    bool running_ = true;
};

}   // namespace ozma
