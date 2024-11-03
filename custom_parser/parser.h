#pragma once

#include "common.h"
#include <sstream>
#include <string>
#include <vector>

namespace ozma {

enum class Parsing { Custom };
DECLARE_ENUM(Parsing, 1, Custom);

struct Order {
    float price{};
    float size{};
};

struct BTCUSDT {
    static inline int32_t iD1 = 256;
    static inline int32_t iD2 = 257;
    int64_t t{};
    int64_t u{};
    std::vector<Order> asks;
    std::vector<Order> bids;
};

inline std::stringstream& operator<<(std::stringstream& ss, BTCUSDT btc) {
    ss << "T: " << btc.t << ", u: " << btc.u;
    ss << "\nasks:\n";
    for (const auto& [price, size] : btc.asks) {
        ss << "[" << price << "," << size << "]\n";
    }
    ss << "bids:\n";
    for (const auto& [price, size] : btc.bids) {
        ss << "[" << price << "," << size << "]\n";
    }
    return ss;
}

class MockParser {
public:
    static BTCUSDT parse(const std::string& message);
};

class NlohmannJsonParser {
public:
    static BTCUSDT parse(const std::string& message);
};

class SimdJsonParser {
public:
    static BTCUSDT parse(const std::string& message);
};

class CustomParser {
public:
    static BTCUSDT parse(const std::string& message);
};

}   // namespace ozma
