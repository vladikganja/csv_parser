#pragma once

#include <string>
#include <vector>

namespace ozma {

enum class Side { Asks, Bids };

struct Order {
    double price;
    double size;
};

struct BTCUSDT {
    static inline int32_t iD1 = 256;
    static inline int32_t iD2 = 257;
    uint64_t t;
    uint64_t u;
    std::vector<Order> orders;
    Side side;
};

class CustomParser {
public:
    BTCUSDT parse(const std::string& message);
};

}   // namespace ozma
