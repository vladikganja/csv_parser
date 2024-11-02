#include "logger.h"

#include <filesystem>
#include <iostream>
#include <thread>

namespace ozma {

namespace {

const std::string FILE_LOG = "./logs/common.log";

Logger* logger = nullptr;

std::string getCurrentTime() {
    time_t now = time(nullptr);
    tm* timeInfo = localtime(&now);
    std::stringstream ss;
    ss << timeInfo->tm_year + 1900 << "-" << std::setfill('0') << std::setw(2)
       << timeInfo->tm_mon + 1 << "-" << std::setfill('0') << std::setw(2) << timeInfo->tm_mday
       << " " << std::setfill('0') << std::setw(2) << timeInfo->tm_hour << ":" << std::setfill('0')
       << std::setw(2) << timeInfo->tm_min << ":" << std::setfill('0') << std::setw(2)
       << timeInfo->tm_sec;
    return ss.str();
}

}   // namespace

void Logger::init(std::initializer_list<Out> outs) {
    static Logger singleton(outs);
    logger = &singleton;
}

Logger::Logger(std::initializer_list<Out> outs) {
    logThread_ = std::thread(&Logger::logThreadFunc, this);
    for (auto out : outs) {
        switch (out) {
        case Out::Stdout:
            stdout_ = true;
            break;
        case Out::File:
            std::filesystem::create_directories("./logs");
            fileLog_.emplace(FILE_LOG);
            if (!fileLog_->is_open()) {
                throw std::runtime_error{ "Can't initialize common.log" };
            }
            break;
        default:
            throw std::runtime_error{ "Undefined log event" };
        }
    }
}

Logger::~Logger() {
    running_ = false;
    logThread_.join();
    if (fileLog_) {
        fileLog_->close();
    }
}

Logger& Logger::getInstance() {
    REQUIRE(logger, "Logger is not initialized!");
    return *logger;
}

std::thread& Logger::getLogThread() {
    return getInstance().logThread_;
}

void Logger::waitLogThread() {
    while (getInstance().logQueue_.size_approx() > 0) {
        std::this_thread::yield();
    }
}

std::unique_ptr<Logger::LogStream> Logger::log(Level level) {
    auto stream = std::make_unique<LogStream>(*this);
    *stream << getCurrentTime() << "\t" << LevelStr(level) << ": ";
    return stream;
}

void Logger::writeMsg(std::string&& msg) {
    logQueue_.enqueue(std::move(msg));
}

void Logger::logThreadFunc() {
    while (running_ || logQueue_.size_approx() > 0) {
        std::string msg;
        while (!logQueue_.try_dequeue(msg) && running_) {
            std::this_thread::yield();
        }
        doWriteMsg(msg);
    }
}

void Logger::doWriteMsg(const std::string& msg) {
    if (stdout_) {
        std::cout << msg;
    }
    if (fileLog_) {
        *fileLog_ << msg;
    }
}

}   // namespace ozma
