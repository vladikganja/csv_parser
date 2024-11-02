#pragma once

#include "common.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <sstream>

namespace ozma {

namespace benchmark {

enum class Bench { Generation, Serialization, Deserialization, Verification, Processing };
DECLARE_ENUM(Bench, 5, Generation, Serialization, Deserialization, Verification, Processing)

class Benchmark {
public:
    constexpr static size_t DIST_ARRAY_SIZE = 20;
    struct DistArray : public std::array<size_t, DIST_ARRAY_SIZE> {
        size_t oneTenthMean;
        size_t measurements;
        Bench bench;
    };
    static void start(Bench bench) {
        measurements[static_cast<size_t>(bench)] = TimePoint::clock::now();
    }

    static void end(Bench bench) {
        auto& stat = stats[static_cast<size_t>(bench)];
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 TimePoint::clock::now() - measurements[static_cast<size_t>(bench)])
                                 .count();
        stat.timeElapsedNano += elapsed;
        stat.calls++;
        if (stat.dist.oneTenthMean == 0 && stat.calls == CALLS_TO_ENSURE_MEAN) {
            stat.dist.oneTenthMean = stat.timeElapsedNano / (stat.calls * 10);
            stat.dist.bench = bench;
        }
        // We have ensured mean, let's estimate distributiln
        if (stat.dist.oneTenthMean != 0) {
            stat.dist[std::min(elapsed / stat.dist.oneTenthMean, size_t{ DIST_ARRAY_SIZE - 1 })]++;
            stat.dist.measurements++;
        }
    }

    static std::chrono::nanoseconds getAverage(Bench bench) {
        return std::chrono::nanoseconds{ stats[static_cast<size_t>(bench)].timeElapsedNano /
                                         stats[static_cast<size_t>(bench)].calls };
    }

    static const DistArray& getDistribution(Bench bench) {
        auto& stat = stats[static_cast<size_t>(bench)];
        REQUIRE(
            stat.calls > CALLS_TO_ENSURE_MEAN, "Not enough measurements to estimate distribution");
        return stat.dist;
    }

private:
    constexpr static inline size_t CALLS_TO_ENSURE_MEAN = 1000;
    struct Stat {
        size_t timeElapsedNano;
        size_t calls;
        DistArray dist;
    };
    static inline std::array<TimePoint, 5> measurements{};
    static inline std::array<Stat, 5> stats{};
};

}   // namespace benchmark

std::stringstream& operator<<(std::stringstream& ss, const benchmark::Benchmark::DistArray& arr);

}   // namespace ozma
