#include "parser.h"
#include "common.h"
#include <chrono>
#include <thread>

namespace ozma {

BTCUSDT CustomParser::parse(const std::string& message) {
    UNUSED(message);
    //std::this_thread::sleep_for(std::chrono::nanoseconds{300});
    return BTCUSDT{};
}

}   // namespace ozma
