#pragma once

#include <array>
#include <vector>

#include "camelup/types.hpp"

namespace camelup {

struct GameState {
    std::array<std::vector<CamelId>, kBoardTiles> board{};
    std::array<int, kMaxPlayers> money{};
    std::array<bool, kCamelCount> die_available{};
    // TODO: Add full Camel Up v1 state (desert tiles, leg ticket decks, winner/loser bet piles, payout tracking)

    PlayerId current_player{0};
    int player_count{2};
    int leg_number{1};
    bool terminal{false};
};

}  // namespace camelup
