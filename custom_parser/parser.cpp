#include "parser.h"
#include "common.h"
#include <chrono>
#include <thread>

namespace ozma {

BTCUSDT CustomParser::parse(const std::string& message) {
    UNUSED(message);
    return BTCUSDT{};
}

}   // namespace ozma
