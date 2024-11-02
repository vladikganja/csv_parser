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

const std::string FILE_PATH = "./csv/data.csv";

using Fastcsv = io::CSVReader<4, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '\"'>>;
using Rapidcsv = rapidcsv::Document;
using Vinces = csv::CSVReader;

enum class Reader { Fastcsv, Rapidcsv, Vinces };
DECLARE_ENUM(Reader, 3, Fastcsv, Rapidcsv, Vinces);

template <typename Reader>
class Parser;

template <>
class Parser<Fastcsv> {
public:
    Parser()
        : reader_(FILE_PATH) {
    }

    std::optional<BTCUSDT> parseLine() { 
        valid_ = reader_.read_row(body_, id_, b1_, b2_);
        if (valid_ && (id_ == BTCUSDT::iD1 || id_ == BTCUSDT::iD2)) {
            return parser_.parse(body_);
        }
        return std::nullopt;
    }

    bool valid() const {
        return valid_;
    }

private:
    Fastcsv reader_;
    CustomParser parser_;
    std::string body_;
    int32_t id_, b1_, b2_;
    bool valid_;
};

template <>
class Parser<Rapidcsv> {
private:
    const static inline size_t DATA_COL = 0;
    const static inline size_t ID_COL = 1;

public:
    Parser()
        : reader_(FILE_PATH), lines_(reader_.GetRowCount()), line_(0) {
    }

    std::optional<BTCUSDT> parseLine() {
        const auto id = reader_.GetCell<int32_t>(ID_COL, line_);
        if (id == BTCUSDT::iD1 || id == BTCUSDT::iD2) {
            return parser_.parse(reader_.GetCell<std::string>(DATA_COL, line_++));
        }
        line_++;
        return std::nullopt;
    }

    bool valid() const {
        return line_ < lines_;
    }

private:
    Rapidcsv reader_;
    CustomParser parser_;
    size_t lines_;
    size_t line_;
};

template <>
class Parser<Vinces> {
private:
    const static inline size_t DATA_COL = 0;
    const static inline size_t ID_COL = 1;

public:
    Parser()
        : reader_(FILE_PATH), cur_(reader_.begin()) {
    }

    std::optional<BTCUSDT> parseLine() {
        if ((*cur_)[ID_COL] == BTCUSDT::iD1 || (*cur_)[ID_COL] == BTCUSDT::iD2) {
            return parser_.parse((*cur_++)[DATA_COL]);
        }
        ++cur_;
        return std::nullopt;
    }

    bool valid() const {
        return cur_ != reader_.end();
    }

private:
    Vinces reader_;
    CustomParser parser_;
    Vinces::iterator cur_;
};

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

}   // namespace

void launch() {
    prepare();

    {
        Parser<Rapidcsv> parser;
        for (; parser.valid();) {
            BENCH_START(Reader, Rapidcsv);
            auto bts = parser.parseLine();
            UNUSED(bts);
            BENCH_END(Reader, Rapidcsv);
        }
        INFO() << BENCH_DISTR_AVG(Reader, Rapidcsv);
    }

    {
        Parser<Fastcsv> parser;
        for (; parser.valid();) {
            BENCH_START(Reader, Fastcsv);
            auto bts = parser.parseLine();
            UNUSED(bts);
            BENCH_END(Reader, Fastcsv);
        }
        INFO() << BENCH_DISTR_AVG(Reader, Fastcsv);
    }

    {
        Parser<Vinces> parser;
        for (; parser.valid();) {
            BENCH_START(Reader, Vinces);
            auto bts = parser.parseLine();
            UNUSED(bts);
            BENCH_END(Reader, Vinces);
        }
        INFO() << BENCH_DISTR_AVG(Reader, Vinces);
    }
}

}   // namespace ozma
