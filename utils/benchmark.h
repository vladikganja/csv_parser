#pragma once

#include "common.h"

#include "hdr_histogram/include/hdr/hdr_histogram.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>

namespace ozma {

namespace benchmark {

namespace detail {
template <typename T, std::size_t... Is>
constexpr std::array<T, sizeof...(Is)> createArray(T value, std::index_sequence<Is...>) {
    return { { (static_cast<void>(Is), value)... } };
}
}   // namespace detail

template <typename T, std::size_t N>
constexpr std::array<T, N> createArray(const T& value) {
    return detail::createArray(value, std::make_index_sequence<N>());
}

bool hdrPercentilesPrint(
    struct hdr_histogram* h,
    FILE* stream,
    int32_t ticksPerHalfDistance,
    double maxPercentile = 0.99);

template <typename BenchType, size_t BenchSize>
class Benchmark {
public:
    static void init(BenchType bench, const char* name) {
        hdr_init(1, 100'000, 3, &histograms[static_cast<size_t>(bench)]);
        benchNames[static_cast<size_t>(bench)] = name;
    }

    static void start(BenchType bench) {
        measurements[static_cast<size_t>(bench)] = TimePoint::clock::now();
    }

    static void end(BenchType bench) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 TimePoint::clock::now() - measurements[static_cast<size_t>(bench)])
                                 .count();
        hdr_record_value(histograms[static_cast<size_t>(bench)], elapsed);
    }

    static hdr_histogram* getHist(BenchType bench) {
        return histograms[static_cast<size_t>(bench)];
    }

    static std::string histToStr(BenchType bench) {
        const size_t bufferSize = 8192;
        char buffer[bufferSize];
        FILE* memfile = fmemopen(buffer, bufferSize, "w");
        REQUIRE(memfile != nullptr, "Can't open memfile");
        REQUIRE(histograms[static_cast<size_t>(bench)] != nullptr, "Historgam is empty");
        REQUIRE(
            !hdrPercentilesPrint(histograms[static_cast<size_t>(bench)], memfile, 3),
            "Can't build histogram");
        rewind(memfile);
        std::stringstream ss;
        ss << "Benchmark for: " << benchNames[static_cast<size_t>(bench)] << "\n" << buffer;
        fclose(memfile);
        return ss.str();
    }

private:
    static inline std::array<TimePoint, BenchSize> measurements{};
    static inline std::array<hdr_histogram*, BenchSize> histograms =
        createArray<hdr_histogram*, BenchSize>(nullptr);
    static inline std::array<std::string_view, BenchSize> benchNames{};
};

}   // namespace benchmark

// EnumClass, value
// Initializes histogram at first request
#define BENCH_START(BenchType, BenchName)                                                          \
    if (auto stat =                                                                                \
            benchmark::Benchmark<BenchType, BenchType##Size>::getHist(BenchType::BenchName);       \
        stat == nullptr) {                                                                         \
        benchmark::Benchmark<BenchType, BenchType##Size>::init(                                    \
            BenchType::BenchName, #BenchType "::" #BenchName);                                     \
    }                                                                                              \
    benchmark::Benchmark<BenchType, BenchType##Size>::start(BenchType::BenchName)

// EnumClass, value
#define BENCH_END(BenchType, BenchName)                                                            \
    benchmark::Benchmark<BenchType, BenchType##Size>::end(BenchType::BenchName)

// EnumClass, value
#define BENCH_DISTR(BenchType, BenchName)                                                          \
    benchmark::Benchmark<BenchType, BenchType##Size>::histToStr(BenchType::BenchName)

}   // namespace ozma
