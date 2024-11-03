#include "parser.h"
#include "benchmark.h"
#include "common.h"
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include <iostream>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <string_view>
#include <thread>

#include "logger.h"
#include "nlohmann/json.hpp"
#include "simdjson/include/simdjson.h"

namespace ozma {

BTCUSDT MockParser::parse(const std::string& message) {
    UNUSED(message);
    return BTCUSDT{};
}

BTCUSDT NlohmannJsonParser::parse(const std::string& message) {
    BTCUSDT result;
    nlohmann::json jsonData = nlohmann::json::parse(message);

    result.t = jsonData.at("T").get<int64_t>();
    result.u = jsonData.at("u").get<int64_t>();

    auto parseOrders = [](const nlohmann::json& data) -> std::vector<Order> {
        std::vector<Order> orders;
        orders.reserve(64);
        for (const auto& item : data) {
            auto priceStr = item[0].get<std::string_view>();
            auto sizeStr = item[1].get<std::string_view>();
            float price;
            float size;
            std::from_chars(priceStr.begin(), priceStr.end(), price);
            std::from_chars(sizeStr.begin(), sizeStr.end(), size);
            orders.emplace_back(Order{ price, size });
        }
        return orders;
    };

    if (auto finder = jsonData.find("a"); finder != jsonData.end()) {
        result.asks = parseOrders(*finder);
    }
    if (auto finder = jsonData.find("b"); finder != jsonData.end()) {
        result.bids = parseOrders(*finder);
    }
    return result;
}

BTCUSDT SimdJsonParser::parse(const std::string& message) {
    BTCUSDT result;
    simdjson::ondemand::parser parser;
    simdjson::padded_string jsonPadded = simdjson::padded_string(message);
    simdjson::ondemand::document doc = parser.iterate(jsonPadded);

    result.t = doc["T"];
    result.u = doc["u"];

    auto parseOrders = [](simdjson::ondemand::array ordersArray) -> std::vector<Order> {
        std::vector<Order> orders;
        orders.reserve(64);
        for (auto orderElem : ordersArray) {
            simdjson::ondemand::array orderArray = orderElem.get_array();
            auto it = orderArray.begin();
            auto sv = std::string_view(*it);
            float price;
            std::from_chars(sv.begin(), sv.end(), price);
            ++it;
            sv = std::string_view(*it);
            float size;
            std::from_chars(sv.begin(), sv.end(), size);

            orders.emplace_back(Order{ price, size });
        }
        return orders;
    };

    if (auto finder = doc.find_field_unordered("a"); finder.error() == simdjson::SUCCESS) {
        result.asks = parseOrders(finder);
    }
    if (auto finder = doc.find_field_unordered("b"); finder.error() == simdjson::SUCCESS) {
        result.bids = parseOrders(finder);
    }
    return result;
}

BTCUSDT CustomParser::parse(const std::string& message) {
    constexpr static size_t tBeg = 41;
    constexpr static size_t uBeg = 91;
    constexpr static size_t len = 13;
    constexpr static size_t abBeg = 125;
    constexpr static size_t abPadding = 6;

    BTCUSDT result;
    std::from_chars(message.c_str() + tBeg, message.c_str() + tBeg + len, result.t);
    std::from_chars(message.c_str() + uBeg, message.c_str() + uBeg + len, result.u);

    // "b":[["65545.34","0.420"],["65344.2","0.006"],["65548.35","15.034"],["65549.35","5.034"]],"a":[[...]]
    std::vector<Order>* current = nullptr;
    for (size_t i = abBeg; i < message.size();) {
        // ab switch //
        if (!std::isdigit(message[i])) {
            current = message[i] == 'a' ? &result.asks : &result.bids;
            current->reserve(64);
            i += abPadding;
        }
        // price //
        size_t priceLen{};
        for (; i + priceLen < message.size() && message[i + priceLen] != '\"'; priceLen++) {
        }
        float price{};
        std::from_chars(message.c_str() + i, message.c_str() + i + priceLen, price);
        i += priceLen + 3;
        // size //
        size_t sizeLen{};
        for (; i + sizeLen < message.size() && message[i + sizeLen] != '\"'; sizeLen++) {
        }
        float size{};
        std::from_chars(message.c_str() + i, message.c_str() + i + sizeLen, size);
        i += sizeLen + 5;
        // emplace //
        current->emplace_back(Order{ price, size });
    }
    return result;
}

}   // namespace ozma
