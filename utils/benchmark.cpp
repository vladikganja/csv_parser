#include "benchmark.h"

namespace ozma {

std::stringstream& operator<<(std::stringstream& ss, const benchmark::Benchmark::DistArray& arr) {
    constexpr size_t lineStart = 12;
    constexpr size_t lineLen = 30;
    const auto max = *std::max_element(arr.begin(), arr.end());
    const auto unitLen = max / lineLen;
    {
        std::stringstream tmp;
        tmp << "\n______________|" << benchmark::BenchStr(arr.bench) << '|';
        auto topLine = tmp.str();
        ss << topLine;
        for (size_t i = topLine.size(); i <= lineStart + lineLen; i++) {
            ss << '_';
        }
        ss << '\n';
    }
    for (size_t i = 0; i < benchmark::Benchmark::DIST_ARRAY_SIZE; i++) {
        std::stringstream tmp;
        tmp << i * arr.oneTenthMean << " ns: ";
        auto prefix = tmp.str();
        ss << prefix;
        for (size_t f = prefix.size(); f < lineStart; f++) {
            ss << ' ';
        }
        size_t u = 0;
        for (; u < arr[i] / unitLen; u++) {
            ss << '#';
        }
        for (; u < lineLen; u++) {
            ss << ' ';
        }
        ss << "|\n";
    }
    ss << "__________________________________________|\n";
    ss << benchmark::BenchStr(arr.bench)
       << " average time: " << benchmark::Benchmark::getAverage(arr.bench).count() << " ns\n";
    return ss;
}
}   // namespace ozma
