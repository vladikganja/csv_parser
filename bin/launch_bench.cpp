#include "launch_bench.h"

#include "common.h"
#include "logger.h"
#include "parser.h"
#include "benchmark.h"

#include <sched.h>
#include "fastcsv/csv.h"

#include "rapidcsv/src/rapidcsv.h"

#include "vinces/single_include/csv.hpp"
#include <chrono>
#include <thread>
#include <optional>
#include <stdexcept>

namespace ozma {

namespace {

void setThreadAffinity() {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(std::thread::hardware_concurrency() - 1, &cpuset);
    pthread_t currentThread = pthread_self();
    if (pthread_setaffinity_np(currentThread, sizeof(cpu_set_t), &cpuset)) {
        throw std::runtime_error{ "pthread_setaffinity_np error" };
    }
}

void setThreadPriority() {
    pthread_t currentThread = pthread_self();
    sched_param schParams;
    schParams.sched_priority = sched_get_priority_max(SCHED_RR);
    if (pthread_setschedparam(currentThread, SCHED_RR, &schParams)) {
        throw std::runtime_error{ "pthread_setschedparam error" };
    }
}

void lockMemory() {
    if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
        throw std::runtime_error{ "mlockall error" };
    }
}

void prepare() {
    setThreadAffinity();
    setThreadPriority();
    lockMemory();
}

const std::string FILE_PATH = "./csv/data.csv";

using Fastcsv = io::CSVReader<4, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '\"'>>;
using Rapidcsv = rapidcsv::Document;
using Vinces = csv::CSVReader;

enum class ReaderType { Fastcsv, Rapidcsv, Vinces };
DECLARE_ENUM(ReaderType, 3, Fastcsv, Rapidcsv, Vinces);

enum class ParserType { NlohmannJson, SimdJson, Custom, CustomAvx };
DECLARE_ENUM(ParserType, 4, NlohmannJson, SimdJson, Custom, CustomAvx);

template <typename ReaderT>
class Reader;

template <>
class Reader<Fastcsv> {
public:
    Reader()
        : reader_(FILE_PATH) {
    }

    std::optional<std::string> readLine() { 
        valid_ = reader_.read_row(body_, id_, b1_, b2_);
        if (valid_ && (id_ == BTCUSDT::iD1 || id_ == BTCUSDT::iD2)) {
            return std::optional<std::string>{std::move(body_)};
        }
        return std::nullopt;
    }

    bool valid() const {
        return valid_;
    }

private:
    Fastcsv reader_;
    std::string body_;
    int32_t id_, b1_, b2_;
    bool valid_ = true;
};

template <>
class Reader<Rapidcsv> {
private:
    const static inline size_t DATA_COL = 0;
    const static inline size_t ID_COL = 1;

public:
    Reader()
        : reader_(FILE_PATH)
        , lines_(reader_.GetRowCount())
        , line_(0) {
    }

    std::optional<std::string> readLine() {
        const auto id = reader_.GetCell<int32_t>(ID_COL, line_);
        if (id == BTCUSDT::iD1 || id == BTCUSDT::iD2) {
            return reader_.GetCell<std::string>(DATA_COL, line_++);
        }
        line_++;
        return std::nullopt;
    }

    bool valid() const {
        return line_ < lines_;
    }

private:
    Rapidcsv reader_;
    size_t lines_;
    size_t line_;
};

template <>
class Reader<Vinces> {
private:
    const static inline size_t DATA_COL = 0;
    const static inline size_t ID_COL = 1;

public:
    Reader()
        : reader_(FILE_PATH), cur_(reader_.begin()) {
    }

    std::optional<std::string> readLine() {
        if ((*cur_)[ID_COL] == BTCUSDT::iD1 || (*cur_)[ID_COL] == BTCUSDT::iD2) {
            return (*cur_++)[DATA_COL].get();
        }
        ++cur_;
        return std::nullopt;
    }

    bool valid() const {
        return cur_ != reader_.end();
    }

private:
    Vinces reader_;
    Vinces::iterator cur_;
};

}   // namespace

struct hdr_histogram* histogram;

void launch() {
    prepare();

    {
        Reader<Fastcsv> reader;
        for (; reader.valid();) {
            BENCH_START(ReaderType, Fastcsv);
            auto data = reader.readLine();
            BENCH_END(ReaderType, Fastcsv);
            if (data) {
                BENCH_START(ParserType, NlohmannJson);
                auto btc2 = NlohmannJsonParser::parse(*data);
                BENCH_END(ParserType, NlohmannJson);
                //INFO() << btc2;

                BENCH_START(ParserType, SimdJson);
                auto btc3 = SimdJsonParser::parse(*data);
                BENCH_END(ParserType, SimdJson);
                //INFO() << btc3;

                BENCH_START(ParserType, Custom);
                auto btc4 = CustomParser::parse(*data);
                BENCH_END(ParserType, Custom);
                //INFO() << btc4;

                BENCH_START(ParserType, CustomAvx);
                auto btc5 = CustomAvxParser::parse(*data);
                BENCH_END(ParserType, CustomAvx);
                //INFO() << btc5;
            }
        }
    }

    {
        Reader<Vinces> reader;
        for (; reader.valid();) {
            BENCH_START(ReaderType, Vinces);
            auto data = reader.readLine();
            BENCH_END(ReaderType, Vinces);
            if (data) {
                BENCH_START(ParserType, NlohmannJson);
                auto btc2 = NlohmannJsonParser::parse(*data);
                BENCH_END(ParserType, NlohmannJson);
                //INFO() << btc2;

                BENCH_START(ParserType, SimdJson);
                auto btc3 = SimdJsonParser::parse(*data);
                BENCH_END(ParserType, SimdJson);
                //INFO() << btc3;

                BENCH_START(ParserType, Custom);
                auto btc4 = CustomParser::parse(*data);
                BENCH_END(ParserType, Custom);
                //INFO() << btc4;

                BENCH_START(ParserType, CustomAvx);
                auto btc5 = CustomAvxParser::parse(*data);
                BENCH_END(ParserType, CustomAvx);
                //INFO() << btc5;
            }
        }
    }

    {
        Reader<Rapidcsv> reader;
        for (; reader.valid();) {
            BENCH_START(ReaderType, Rapidcsv);
            auto data = reader.readLine();
            BENCH_END(ReaderType, Rapidcsv);
            if (data) {
                BENCH_START(ParserType, NlohmannJson);
                auto btc2 = NlohmannJsonParser::parse(*data);
                BENCH_END(ParserType, NlohmannJson);
                //INFO() << btc2;

                BENCH_START(ParserType, SimdJson);
                auto btc3 = SimdJsonParser::parse(*data);
                BENCH_END(ParserType, SimdJson);
                //INFO() << btc3;

                BENCH_START(ParserType, Custom);
                auto btc4 = CustomParser::parse(*data);
                BENCH_END(ParserType, Custom);
                //INFO() << btc4;

                BENCH_START(ParserType, CustomAvx);
                auto btc5 = CustomAvxParser::parse(*data);
                BENCH_END(ParserType, CustomAvx);
                //INFO() << btc5;
            }
        }
    }

    INFO() << BENCH_DISTR(ReaderType, Fastcsv);
    INFO() << BENCH_DISTR(ReaderType, Vinces);
    INFO() << BENCH_DISTR(ReaderType, Rapidcsv);
    INFO() << BENCH_DISTR(ParserType, NlohmannJson);
    INFO() << BENCH_DISTR(ParserType, SimdJson);
    INFO() << BENCH_DISTR(ParserType, Custom);
    INFO() << BENCH_DISTR(ParserType, CustomAvx);
}

}   // namespace ozma
