#pragma once

#include "common.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <cmath>
#include <optional>
#include <sstream>

namespace ozma {

namespace benchmark {

constexpr static size_t DIST_ARRAY_SIZE = 30;

template <typename BenchType>
struct DistArray : public std::array<size_t, DIST_ARRAY_SIZE> {
    size_t oneTenthMean;
    size_t measurements;
    BenchType bench;
    std::string_view benchStr;
};

template <typename BenchType, size_t BenchSize>
class Benchmark {
public:
    static void start(BenchType bench) {
        measurements[static_cast<size_t>(bench)] = TimePoint::clock::now();
    }

    static void end(BenchType bench) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 TimePoint::clock::now() - measurements[static_cast<size_t>(bench)])
                                 .count();
        auto& stat = *stats[static_cast<size_t>(bench)];
        stat.timeElapsedNano += elapsed;
        stat.calls++;
        if (stat.calls == CALLS_TO_ENSURE_MEAN) {
            stat.dist.oneTenthMean = stat.timeElapsedNano / (stat.calls * 10);
            stat.dist.measurements += stat.calls - 1;
            stat.dist[elapsed / stat.dist.oneTenthMean] += stat.calls - 1;
        }
        // We have ensured mean, let's estimate distribution
        if (stat.dist.oneTenthMean != 0) {
            stat.dist[std::min(elapsed / stat.dist.oneTenthMean, size_t{ DIST_ARRAY_SIZE - 1 })]++;
            stat.dist.measurements++;
        }
    }

    template <typename Dur = std::chrono::nanoseconds>
    static Dur getAverage(BenchType bench) {
        return std::chrono::duration_cast<Dur>(
            std::chrono::nanoseconds{ stats[static_cast<size_t>(bench)]->timeElapsedNano /
                                      stats[static_cast<size_t>(bench)]->calls });
    }

    static const DistArray<BenchType>& getDistribution(BenchType bench) {
        auto& stat = stats[static_cast<size_t>(bench)];
        REQUIRE(stat, "No measurements for bench");
        REQUIRE(
            stat->calls > CALLS_TO_ENSURE_MEAN, "Not enough measurements to estimate distribution");
        return stat->dist;
    }

private:
    constexpr static inline size_t CALLS_TO_ENSURE_MEAN = 20;
    struct Stat {
        size_t timeElapsedNano;
        size_t calls;
        DistArray<BenchType> dist;
    };
    static inline std::array<TimePoint, BenchSize> measurements{};
    static inline std::array<std::optional<Stat>, BenchSize> stats{};

public:
    static std::optional<Stat>& getStat(BenchType bench) {
        return stats[static_cast<size_t>(bench)];
    }
};

}   // namespace benchmark

// EnumClass, value
// Sets bench type at first request
#define BENCH_START(BenchType, BenchName)                                                          \
    if (auto& statOpt =                                                                            \
            benchmark::Benchmark<BenchType, BenchType##Size>::getStat(BenchType::BenchName);       \
        !statOpt) {                                                                                \
        statOpt.emplace();                                                                         \
        statOpt->dist.bench = BenchType::BenchName;                                                \
        statOpt->dist.benchStr = #BenchName;                                                       \
    }                                                                                              \
    benchmark::Benchmark<BenchType, BenchType##Size>::start(BenchType::BenchName)

// EnumClass, value
#define BENCH_END(BenchType, BenchName)                                                            \
    benchmark::Benchmark<BenchType, BenchType##Size>::end(BenchType::BenchName)

// EnumClass, value, period = std::chrono::nanoseconds
#define BENCH_AVG(BenchType, BenchName, ...)                                                       \
    benchmark::Benchmark<BenchType, BenchType##Size>::getAverage<__VA_ARGS__>(BenchType::BenchName)

// EnumClass, value
#define BENCH_DISTR(BenchType, BenchName)                                                          \
    benchmark::Benchmark<BenchType, BenchType##Size>::getDistribution(BenchType::BenchName)

#define BENCH_DISTR_AVG(BenchType, BenchName)                                                      \
    benchmark::Benchmark<BenchType, BenchType##Size>::getDistribution(BenchType::BenchName)        \
        << "\nAvg time: " << BENCH_AVG(BenchType, BenchName).count() << " ns"

template <typename BenchType>
inline std::stringstream&
operator<<(std::stringstream& ss, const benchmark::DistArray<BenchType>& arr) {
    constexpr size_t lineStart = 16;
    constexpr size_t lineLen = 100;
    const auto max = *std::max_element(arr.begin(), arr.end());
    const auto unitLen = static_cast<double>(max) / lineLen;
    {
        std::stringstream tmp;
        tmp << "\n___________________|" << std::string{ arr.benchStr } << '|';
        auto topLine = tmp.str();
        ss << topLine;
        for (size_t i = topLine.size(); i <= lineStart + lineLen; i++) {
            ss << '_';
        }
        ss << '\n';
    }
    for (size_t i = 0; i < benchmark::DIST_ARRAY_SIZE; i++) {
        std::stringstream tmp;
        tmp << i * arr.oneTenthMean << " ns: ";
        auto prefix = tmp.str();
        ss << prefix;
        for (size_t f = prefix.size(); f < lineStart; f++) {
            ss << ' ';
        }
        size_t u = 0;
        auto end = std::round(arr[i] / unitLen);
        for (; u < end; u++) {
            ss << '#';
        }
        for (; u < lineLen; u++) {
            ss << ' ';
        }
        ss << "|\n";
    }
    for (size_t i = 0; i < lineStart + lineLen; i++) {
        ss << '_';
    }
    ss << "|\n";
    return ss;
}

}   // namespace ozma
