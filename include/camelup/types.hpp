#pragma once

#include <cstdint>

namespace camelup {

using PlayerId = std::uint8_t;
using CamelId = std::uint8_t;

inline constexpr int kCamelCount = 5;
inline constexpr int kBoardTiles = 17;
inline constexpr int kMaxPlayers = 8;
inline constexpr bool kCrazyCamelsEnabled = false;  // Camel Up v1

enum Camel : CamelId {
    Blue = 0,
    Green = 1,
    Yellow = 2,
    Orange = 3,
    White = 4
};

}  // namespace camelup
