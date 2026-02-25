#include "camelup/engine.hpp"
#include "camelup/rules/legal_actions.hpp"

#include <algorithm> // any_of, min
#include <stdexcept>

namespace camelup {

namespace {

constexpr std::array<int, kLegTicketCount> kLegTicketDefaults = {5, 3, 2};
// Final bet rewards in play order for correct guesses
constexpr std::array<int, 5> kFinalBetPayouts = {8, 5, 3, 2, 1};

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

// Validate placement by matching against generated legal actions
bool is_legal_place_desert_tile_action(const GameState& state, const PlaceDesertTilePayload& payload) {
    const auto legal = rules::legal_actions(state);
    return std::any_of(legal.begin(), legal.end(), [&payload](const Action& action) {
        if (action.type() != ActionType::PlaceDesertTile) {
            return false;
        }
        const auto& legal_payload = std::get<PlaceDesertTilePayload>(action.payload);
        return legal_payload.tile == payload.tile && legal_payload.move_delta == payload.move_delta;
    });
}

bool is_legal_take_leg_ticket_action(const GameState& state, const TakeLegTicketPayload& payload) {
    const auto legal = rules::legal_actions(state);
    return std::any_of(legal.begin(), legal.end(), [&payload](const Action& action) {
        if (action.type() != ActionType::TakeLegTicket) {
            return false;
        }
        const auto& legal_payload = std::get<TakeLegTicketPayload>(action.payload);
        return legal_payload.camel == payload.camel;
    });
}

// Validate winner bet by checking generated legal actions
bool is_legal_bet_winner_action(const GameState& state, const BetWinnerPayload& payload) {
    const auto legal = rules::legal_actions(state);
    return std::any_of(legal.begin(), legal.end(), [&payload](const Action& action) {
        if (action.type() != ActionType::BetWinner) {
            return false;
        }
        const auto& legal_payload = std::get<BetWinnerPayload>(action.payload);
        return legal_payload.camel == payload.camel;
    });
}

// Validate loser bet by checking generated legal actions
bool is_legal_bet_loser_action(const GameState& state, const BetLoserPayload& payload) {
    const auto legal = rules::legal_actions(state);
    return std::any_of(legal.begin(), legal.end(), [&payload](const Action& action) {
        if (action.type() != ActionType::BetLoser) {
            return false;
        }
        const auto& legal_payload = std::get<BetLoserPayload>(action.payload);
        return legal_payload.camel == payload.camel;
    });
}

int clamp_tile_index(int tile) {
    return std::max(0, std::min(tile, kBoardTiles - 1));
}

int final_bet_payout_for_correct_index(int correct_index) {
    if (correct_index < 0) {
        return 1;
    }
    if (correct_index >= static_cast<int>(kFinalBetPayouts.size())) {
        return 1;
    }
    return kFinalBetPayouts[correct_index];
}

// Build race order from first to last
std::vector<CamelId> build_race_order(const GameState& state) {
    std::vector<CamelId> order;
    order.reserve(kCamelCount);

    for (int tile = kBoardTiles - 1; tile >= 0; --tile) {
        const auto& stack = state.board[tile];
        for (int idx = static_cast<int>(stack.size()) - 1; idx >= 0; --idx) {
            order.push_back(stack[static_cast<std::size_t>(idx)]);
        }
    }
    if (order.empty()) {
        throw std::runtime_error("race order not found on board");
    }
    return order;
}

void resolve_leg_tickets(GameState& state, const std::vector<CamelId>& race_order) {
    if (race_order.size() < 2) {
        throw std::runtime_error("insufficient race order for leg scoring");
    }

    const CamelId first = race_order[0];
    const CamelId second = race_order[1];

    for (int player = 0; player < state.player_count; ++player) {
        for (const auto& ticket : state.player_leg_tickets[player]) {
            if (ticket.camel == first) {
                state.money[player] += ticket.value;
            } else if (ticket.camel == second) {
                state.money[player] += 1;
            } else {
                state.money[player] -= 1;
            }
        }
        state.player_leg_tickets[player].clear();
    }
    for (int player = state.player_count; player < kMaxPlayers; ++player) {
        state.player_leg_tickets[player].clear();
    }
}

void reset_for_next_leg(GameState& state) {
    state.leg_tickets_remaining.fill(kLegTicketCount);
    state.desert_tile_owner.fill(-1);
    for (int player = 0; player < kMaxPlayers; ++player) {
        state.desert_tiles[player] = {-1, 1};
    }
    state.die_available.fill(true);
    ++state.leg_number;
}

void resolve_leg_end(GameState& state) {
    const auto race_order = build_race_order(state);
    resolve_leg_tickets(state, race_order);
    reset_for_next_leg(state);
}

// Resolve one final bet stack using target camel and play order payouts
void resolve_final_bet_stack(std::array<int, kMaxPlayers>& money,
                             const std::vector<FinalBetCard>& stack,
                             CamelId target_camel) {
    int correct_count = 0;
    for (const auto& card : stack) {
        const int player = static_cast<int>(card.player);
        if (player < 0 || player >= kMaxPlayers) {
            continue;
        }
        if (card.camel == target_camel) {
            money[player] += final_bet_payout_for_correct_index(correct_count);
            ++correct_count;
        } else {
            money[player] -= 1;
        }
    }
}

// Resolve both final bet stacks when race ends
void resolve_end_of_game_payouts(GameState& state) {
    const auto race_order = build_race_order(state);
    const CamelId winner = race_order.front();
    const CamelId loser = race_order.back();
    resolve_final_bet_stack(state.money, state.winner_bet_stack, winner);
    resolve_final_bet_stack(state.money, state.loser_bet_stack, loser);
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

    // Camel Up v1 opening setup
    // Roll each camel die once and place that camel on tile 1..3
    reset_leg_dice(state);
    for (int roll = 0; roll < kCamelCount; ++roll) {
        const auto [camel, distance] = roll_die(state);
        state.board[distance].push_back(camel);
    }
    // Leg 1 starts with all dice available after opening setup
    reset_leg_dice(state);

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
            // Defensive recovery for malformed states with no available dice
            if (!has_available_die(next)) {
                resolve_leg_end(next);
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
        case ActionType::PlaceDesertTile: {
            // Extract and validate placement intent before mutating state
            const auto payload = std::get<PlaceDesertTilePayload>(action.payload);
            if (!is_legal_place_desert_tile_action(next, payload)) {
                throw std::invalid_argument("illegal place desert tile action");
            }

            const PlayerId current_player = next.current_player;
            const int previous_tile = next.desert_tiles[current_player].tile;
            // Remove previous tile ownership when player moves their desert tile
            if (previous_tile >= 0 && previous_tile < kBoardTiles &&
                next.desert_tile_owner[previous_tile] == static_cast<int>(current_player)) {
                next.desert_tile_owner[previous_tile] = -1;
            }

            // Write new tile placement and owner lookup entry
            next.desert_tiles[current_player] = {payload.tile, payload.move_delta};
            next.desert_tile_owner[payload.tile] = static_cast<int>(current_player);

            // End turn after successful placement
            next.current_player = static_cast<PlayerId>((next.current_player + 1) % next.player_count);
            break;
        }
        case ActionType::TakeLegTicket: {
            // Validate ticket request for current state
            const auto payload = std::get<TakeLegTicketPayload>(action.payload);
            if (!is_legal_take_leg_ticket_action(next, payload)) {
                throw std::invalid_argument("illegal take leg ticket action");
            }

            // Determine ticket value from remaining count
            const CamelId camel = payload.camel;
            const int remaining = next.leg_tickets_remaining[camel];
            if (remaining <= 0 || remaining > kLegTicketCount) {
                throw std::runtime_error("invalid leg ticket state");
            }

            const int next_ticket_index = kLegTicketCount - remaining;
            const int ticket_value = next.leg_ticket_values[camel][next_ticket_index];

            // Record ticket on player and consume one from supply
            next.player_leg_tickets[next.current_player].push_back({camel, ticket_value});
            next.leg_tickets_remaining[camel] = remaining - 1;
            next.current_player = static_cast<PlayerId>((next.current_player + 1) % next.player_count);
            break;
        }
        case ActionType::BetWinner: {
            // Validate that current player still has this winner card
            const auto payload = std::get<BetWinnerPayload>(action.payload);
            if (!is_legal_bet_winner_action(next, payload)) {
                throw std::invalid_argument("illegal winner bet action");
            }

            const PlayerId current_player = next.current_player;
            // Push bet in play order then mark card as used
            next.winner_bet_stack.push_back({current_player, payload.camel});
            next.winner_bet_card_available[current_player][payload.camel] = false;
            next.current_player = static_cast<PlayerId>((next.current_player + 1) % next.player_count);
            break;
        }
        case ActionType::BetLoser: {
            // Validate that current player still has this loser card
            const auto payload = std::get<BetLoserPayload>(action.payload);
            if (!is_legal_bet_loser_action(next, payload)) {
                throw std::invalid_argument("illegal loser bet action");
            }

            const PlayerId current_player = next.current_player;
            // Push bet in play order then mark card as used
            next.loser_bet_stack.push_back({current_player, payload.camel});
            next.loser_bet_card_available[current_player][payload.camel] = false;
            next.current_player = static_cast<PlayerId>((next.current_player + 1) % next.player_count);
            break;
        }
    }

    // Race ends as soon as a camel reaches the final tile
    if (!next.board[kBoardTiles - 1].empty()) {
        next.terminal = true;
        // Final winner and loser bets are settled once on transition to terminal
        resolve_end_of_game_payouts(next);
    }

    // Leg ends when all dice are consumed and race is not terminal
    if (!next.terminal && action.type() == ActionType::RollDie && !has_available_die(next)) {
        resolve_leg_end(next);
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

    // Base landing tile from die roll
    const int landing_tile = std::min(tile + distance, kBoardTiles - 1);

    int final_tile = landing_tile;
    bool place_under_stack = false;

    // Desert tile triggers +1 oasis or -1 mirage and pays 1 coin to owner
    const int owner = state.desert_tile_owner[landing_tile];
    if (owner >= 0 && owner < kMaxPlayers) {
        if (owner < state.player_count) {
            state.money[owner] += 1;
        }

        int move_delta = state.desert_tiles[owner].move_delta;
        if (move_delta != -1 && move_delta != 1) {
            move_delta = 1;
        }
        final_tile = clamp_tile_index(landing_tile + move_delta);
        place_under_stack = (move_delta < 0);
    }

    // Oasis stacks on top and mirage stacks underneath
    auto& destination = state.board[final_tile];
    if (place_under_stack) {
        destination.insert(destination.begin(), carried.begin(), carried.end());
    } else {
        destination.insert(destination.end(), carried.begin(), carried.end());
    }
}

}  // namespace camelup
