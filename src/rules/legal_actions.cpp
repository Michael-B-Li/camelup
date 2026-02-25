#include "camelup/rules/legal_actions.hpp"

namespace camelup::rules {

namespace {

// True when a tile has another player's desert tile that blocks placement adjacency checks
bool has_blocking_desert_tile(const GameState& state, int tile, PlayerId current_player) {
    if (tile < 0 || tile >= kBoardTiles) {
        return false;
    }

    const int owner = state.desert_tile_owner[tile];
    if (owner < 0) {
        return false;
    }

    // Current player may move their own tile, so it does not block their legality checks
    if (owner == static_cast<int>(current_player) && state.desert_tiles[current_player].tile == tile) {
        return false;
    }

    return true;
}

// Placement limits for desert tiles in this engine
bool is_legal_desert_tile_placement(const GameState& state, int tile, PlayerId current_player) {
    // Cannot place on start or finish tile
    if (tile <= 0 || tile >= kBoardTiles - 1) {
        return false;
    }
    // Cannot place on a tile occupied by camels
    if (!state.board[tile].empty()) {
        return false;
    }
    // Cannot place on or adjacent to another blocking desert tile
    if (has_blocking_desert_tile(state, tile, current_player)) {
        return false;
    }
    if (has_blocking_desert_tile(state, tile - 1, current_player)) {
        return false;
    }
    if (has_blocking_desert_tile(state, tile + 1, current_player)) {
        return false;
    }
    return true;
}

}  // namespace

// Build full legal action list for the current player in current state
std::vector<Action> legal_actions(const GameState& state) {
    // No actions once game is terminal
    if (state.terminal) {
        return {};
    }

    std::vector<Action> actions;
    // Reserve near upper bound to avoid repeated reallocations
    actions.reserve(1 + ((kBoardTiles - 2) * 2) + kCamelCount * 3);

    // Rolling is always offered
    actions.push_back(Action::roll_die());

    const PlayerId current_player = state.current_player;
    // Defensive guard for malformed state
    if (static_cast<int>(current_player) >= state.player_count) {
        return actions;
    }

    // For each legal tile add both oasis (+1) and mirage (-1) options
    for (int tile = 1; tile < kBoardTiles - 1; ++tile) {
        if (!is_legal_desert_tile_placement(state, tile, current_player)) {
            continue;
        }
        actions.push_back(Action::place_desert_tile(tile, 1));
        actions.push_back(Action::place_desert_tile(tile, -1));
    }

    // Leg ticket action exists only while tickets remain for that camel
    for (CamelId camel = 0; camel < static_cast<CamelId>(kCamelCount); ++camel) {
        if (state.leg_tickets_remaining[camel] > 0) {
            actions.push_back(Action::take_leg_ticket(camel));
        }
    }

    // Final bet actions depend on per-player card availability
    for (CamelId camel = 0; camel < static_cast<CamelId>(kCamelCount); ++camel) {
        if (state.winner_bet_card_available[current_player][camel]) {
            actions.push_back(Action::bet_winner(camel));
        }
        if (state.loser_bet_card_available[current_player][camel]) {
            actions.push_back(Action::bet_loser(camel));
        }
    }

    return actions;
}

}  // namespace camelup::rules
