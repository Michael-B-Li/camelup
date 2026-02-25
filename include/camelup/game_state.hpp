#pragma once

#include <array>
#include <vector>

#include "camelup/types.hpp"

namespace camelup {

struct DesertTilePlacement {
    int tile{-1};
    int move_delta{1};
};

struct LegTicket {
    CamelId camel{0};
    int value{0};
};

struct FinalBetCard {
    PlayerId player{0};
    CamelId camel{0};
};

struct GameState {
    std::array<std::vector<CamelId>, kBoardTiles> board{};
    std::array<int, kMaxPlayers> money{};
    std::array<bool, kCamelCount> die_available{};
    std::array<DesertTilePlacement, kMaxPlayers> desert_tiles{};
    std::array<int, kBoardTiles> desert_tile_owner{};

    std::array<int, kCamelCount> leg_tickets_remaining{};
    std::array<std::array<int, kLegTicketCount>, kCamelCount> leg_ticket_values{};
    std::array<std::vector<LegTicket>, kMaxPlayers> player_leg_tickets{};

    std::vector<FinalBetCard> winner_bet_stack{};
    std::vector<FinalBetCard> loser_bet_stack{};
    std::array<std::array<bool, kCamelCount>, kMaxPlayers> winner_bet_card_available{};
    std::array<std::array<bool, kCamelCount>, kMaxPlayers> loser_bet_card_available{};

    PlayerId current_player{0};
    int player_count{2};
    int leg_number{1};
    bool terminal{false};
};

}  // namespace camelup
