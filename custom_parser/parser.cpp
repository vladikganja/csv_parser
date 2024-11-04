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

struct Charset {
    char set[8]{ 0, 0, 0, 0, 0, 0, 0, 0 };
};

std::array<float, 256> parseCharsToFloatsAvx(std::array<Charset, 256>& chars, size_t size) {
    std::array<float, 256> result;
    for (size_t i = 0, resI = 0;; i += 8) {
        __m256i d4bit32[2]{};
        for (size_t s = 0; s < 2; s++) {
            __m256i rawChars = _mm256_loadu_si256(reinterpret_cast<__m256i*>(&chars[i + s * 4]));
            const __m256i ascii0 = _mm256_set1_epi8('0');
            __m256i d1bit8 = _mm256_subs_epu8(rawChars, ascii0);

            const __m256i mult10 = _mm256_setr_epi8(
                10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1,
                10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
            __m256i d2bit16 = _mm256_maddubs_epi16(d1bit8, mult10);

            const __m256i mult100 = _mm256_setr_epi16(
                100, 1, 100, 1, 100, 1, 100, 1, 100, 1, 100, 1, 100, 1, 100, 1);
            d4bit32[s] = _mm256_madd_epi16(d2bit16, mult100);
        }
        __m256i d4bit16 = _mm256_packus_epi32(d4bit32[0], d4bit32[1]);

        const __m256i mult10k = _mm256_setr_epi16(
            10'000, 1, 10'000, 1, 10'000, 1, 10'000, 1, 10'000, 1, 10'000, 1, 10'000, 1, 10'000, 1);
        __m256i d8bit32 = _mm256_madd_epi16(d4bit16, mult10k);

        __m256 f8bit32 = _mm256_cvtepi32_ps(d8bit32);

        const __m256 divMask =
            _mm256_setr_ps(100.f, 1000.f, 100.f, 1000.f, 100.f, 1000.f, 100.f, 1000.f);
        __m256 f8Shuffled = _mm256_div_ps(f8bit32, divMask);
        for (size_t j : {0, 1, 4, 5, 2, 3, 6, 7}) {
            result[resI++] = f8Shuffled[j];
        }
        if (i > size) {
            break;
        }
    }
    return result;
}

BTCUSDT CustomAvxParser::parse(const std::string& message) {
    constexpr static size_t tBeg = 41;
    constexpr static size_t uBeg = 91;
    constexpr static size_t len = 13;
    constexpr static size_t abBeg = 125;
    constexpr static size_t abPadding = 6;

    BTCUSDT result;
    std::from_chars(message.c_str() + tBeg, message.c_str() + tBeg + len, result.t);
    std::from_chars(message.c_str() + uBeg, message.c_str() + uBeg + len, result.u);

    char order[2]{ 0, 0 };
    size_t orderI = 0;
    size_t sideLen[2] { 0, 0 };
    std::array<Charset, 256> chars;
    size_t charsI = 0;
    for (size_t i = abBeg; i < message.size();) {
        // ab switch //
        if (!std::isdigit(message[i])) {
            order[orderI++] = message[i];
            sideLen[0] = charsI;
            i += abPadding;
        }
        // price //
        size_t priceLen{};
        for (; i + priceLen < message.size() && message[i + priceLen] != '\"'; priceLen++) {
        }
        for (size_t backI = i + priceLen - 1, setI = 7; backI >= i; backI--) {
            if (message[backI] != '.') {
                chars[charsI].set[setI--] = message[backI];
            }
        }
        charsI++;
        i += priceLen + 3;
        // size //
        size_t sizeLen{};
        for (; i + sizeLen < message.size() && message[i + sizeLen] != '\"'; sizeLen++) {
        }
        for (size_t backI = i + sizeLen - 1, setI = 7; backI >= i; backI--) {
            if (message[backI] != '.') {
                chars[charsI].set[setI--] = message[backI];
            }
        }
        charsI++;
        i += sizeLen + 5;
    }
    if (sideLen[0] == 0) {
        sideLen[0] = charsI;
    } else {
        sideLen[1] = charsI - sideLen[0];
    }

    auto floats = parseCharsToFloatsAvx(chars, charsI);

    size_t floatsI = 0;
    for (size_t oI = 0; oI < 2; oI++) {
        std::vector<Order>* current = nullptr;
        if (order[oI] == 'a') {
            current = &result.asks;
        } else if (order[oI] == 'b') {
            current = &result.bids;
        } else {
            break;
        }
        current->resize(sideLen[oI] / 2);
        for (size_t i = 0; i < current->size(); i++, floatsI += 2) {
            (*current)[i].price = floats[floatsI];
            (*current)[i].size = floats[floatsI + 1];
        }
    }
    return result;
}

}   // namespace ozma
