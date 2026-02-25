#include "camelup/engine.hpp"
#include "camelup/rules/legal_actions.hpp"

#include <algorithm> // any_of, min
#include <stdexcept>

namespace camelup {

namespace {

constexpr std::array<int, kLegTicketCount> kLegTicketDefaults = {5, 3, 2};

// Locate a camel anywhere on the board and return {tile_index, stack_index}
// stack_index is measured bottom->top within that tile
std::pair<int, int> find_camel(const GameState& state, CamelId camel) {
    for (int tile = 0; tile < kBoardTiles; ++tile) {
        const auto& stack = state.board[tile];
        for (int idx = 0; idx < static_cast<int>(stack.size()); ++idx) {
            if (stack[idx] == camel) {
                return {tile, idx};
            }
        }
    }
    return {-1, -1};
}

// In a leg, each camel die can be rolled once
bool has_available_die(const GameState& state) {
    return std::any_of(state.die_available.begin(), state.die_available.end(), [](bool available) {
        return available;
    });
}

}  // namespace

Engine::Engine(std::uint32_t seed) : rng_(seed) {}

GameState Engine::new_game(int player_count) {
    if (player_count < 2 || player_count > kMaxPlayers) {
        throw std::invalid_argument("player_count must be between 2 and 8");
    }

    GameState state;
    // Initial game state for Camel Up (v1)
    state.player_count = player_count;
    state.current_player = 0;
    state.leg_number = 1;
    state.terminal = false;
    state.money.fill(3);
    state.desert_tile_owner.fill(-1);
    state.leg_tickets_remaining.fill(kLegTicketCount);
    reset_leg_dice(state);

    for (CamelId camel = 0; camel < static_cast<CamelId>(kCamelCount); ++camel) {
        state.leg_ticket_values[camel] = kLegTicketDefaults;
    }

    for (int player = 0; player < kMaxPlayers; ++player) {
        const bool active_player = player < player_count;
        for (CamelId camel = 0; camel < static_cast<CamelId>(kCamelCount); ++camel) {
            state.winner_bet_card_available[player][camel] = active_player;
            state.loser_bet_card_available[player][camel] = active_player;
        }
    }

    // Basic setup used in this scaffold: all camels start on tile 0
    // TODO: Implement Camel Up v1 opening setup (initial die rolls) instead of a fixed stack
    for (CamelId camel = 0; camel < static_cast<CamelId>(kCamelCount); ++camel) {
        state.board[0].push_back(camel);
    }

    return state;
}

std::vector<Action> Engine::legal_actions(const GameState& state) const {
    return rules::legal_actions(state);
}

GameState Engine::apply_action(const GameState& state, const Action& action) {
    GameState next = state;
    // Preserve terminal states (no further mutation once game is over)
    if (next.terminal) {
        return next;
    }

    switch (action.type()) {
        case ActionType::RollDie: {
            // If the leg is exhausted, start a new leg before rolling again
            if (!has_available_die(next)) {
                reset_leg_dice(next);
                ++next.leg_number;
            }

            // Roll one available camel die and move that camel stack
            const auto [camel, distance] = roll_die(next);
            move_camel_stack(next, camel, distance);

            // Current player receives 1 coin for rolling
            next.money[next.current_player] += 1;

            // End of turn: pass to next player in seating order
            next.current_player = static_cast<PlayerId>((next.current_player + 1) % next.player_count);
            break;
        }
        case ActionType::PlaceDesertTile:
        case ActionType::TakeLegTicket:
        case ActionType::BetWinner:
        case ActionType::BetLoser: {
            // Placeholder action handling: rotate turn only
            // TODO: Implement full v1 behaviour and scoring effects for non-roll actions
            next.current_player = static_cast<PlayerId>((next.current_player + 1) % next.player_count);
            break;
        }
    }

    // Current finish condition: any camel on final tile ends the game
    // TODO: Add end-of-game payout resolution (winner/loser bets) before finalising scores
    if (!next.board[kBoardTiles - 1].empty()) {
        next.terminal = true;
    }

    return next;
}

void Engine::reset_leg_dice(GameState& state) {
    // Every camel die becomes available at leg start
    state.die_available.fill(true);
}

std::pair<CamelId, int> Engine::roll_die(GameState& state) {
    std::vector<CamelId> available;
    available.reserve(kCamelCount);

    // Gather only dice that have not been rolled in this leg
    for (CamelId camel = 0; camel < static_cast<CamelId>(kCamelCount); ++camel) {
        if (state.die_available[camel]) {
            available.push_back(camel);
        }
    }

    if (available.empty()) {
        throw std::runtime_error("no available dice to roll");
    }

    // Randomly choose one available camel die
    std::uniform_int_distribution<int> camel_pick(0, static_cast<int>(available.size() - 1));
    const CamelId camel = available[camel_pick(rng_)];

    // Camel Up movement distance is 1 to 3
    std::uniform_int_distribution<int> distance_roll(1, 3);
    const int distance = distance_roll(rng_);

    // Mark chosen die as consumed for this leg
    state.die_available[camel] = false;
    return {camel, distance};
}

void Engine::move_camel_stack(GameState& state, CamelId camel, int distance) {
    // Moving camel carries every camel above it on the stack
    const auto [tile, idx] = find_camel(state, camel);
    if (tile < 0 || idx < 0) {
        throw std::runtime_error("camel not found on board");
    }

    auto& source = state.board[tile];
    std::vector<CamelId> carried(source.begin() + idx, source.end());
    source.erase(source.begin() + idx, source.end());

    // Clamp movement at the finish tile in this scaffold
    // TODO: Apply desert tile +/-1 movement and under/over stacking rules after landing
    const int target_tile = std::min(tile + distance, kBoardTiles - 1);

    // Landing behaviour: carried stack is placed on top of destination stack
    auto& destination = state.board[target_tile];
    destination.insert(destination.end(), carried.begin(), carried.end());
}

}  // namespace camelup
