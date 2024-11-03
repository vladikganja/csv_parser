#pragma once

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace ozma {

#define UNUSED(X) (void)X

#define ASSERT(expr)                                                                               \
    (static_cast<bool>(expr)                                                                       \
         ? void(0)                                                                                 \
         : throw std::runtime_error{ (std::stringstream{} << "Assertion failed: " << #expr         \
                                                          << "; Line: " << __LINE__                \
                                                          << "; Func: " << __PRETTY_FUNCTION__)    \
                                         .str() })

#define REQUIRE(expr, msg)                                                                         \
    (static_cast<bool>(expr)                                                                       \
         ? void(0)                                                                                 \
         : throw std::runtime_error{                                                               \
               (std::stringstream{} << "Require failed: " << #expr << "; Line: " << __LINE__       \
                                    << "; Func: " << __PRETTY_FUNCTION__ << "\nMessage: " << msg)  \
                   .str() })

#define CASE1(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    default:                                                                                       \
        REQUIRE(false, "Undefined enum option");
#define CASE2(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE1(Name, __VA_ARGS__)
#define CASE3(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE2(Name, __VA_ARGS__)
#define CASE4(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE3(Name, __VA_ARGS__)
#define CASE5(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE4(Name, __VA_ARGS__)
#define CASE6(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE5(Name, __VA_ARGS__)
#define CASE7(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE6(Name, __VA_ARGS__)
#define CASE8(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE7(Name, __VA_ARGS__)
#define CASE9(Name, Case, ...)                                                                     \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE8(Name, __VA_ARGS__)
#define CASE10(Name, Case, ...)                                                                    \
    Case:                                                                                          \
    return #Case;                                                                                  \
    case Name::CASE9(Name, __VA_ARGS__)

#define DECLARE_ENUM(Name, Num, ...)                                                               \
    [[maybe_unused]] const size_t Name##Size = Num;                                                \
    [[maybe_unused]] inline std::string_view Name##Str(Name obj) {                                 \
        switch (obj) {                                                                             \
        case Name::CASE##Num(Name, __VA_ARGS__)                                                    \
        }                                                                                          \
}

using TimePoint = std::chrono::
    time_point<std::chrono::high_resolution_clock, std::chrono::duration<int64_t, std::nano>>;

}   // namespace ozma
