#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "camelup/actions.hpp"
#include "camelup/engine.hpp"

namespace {

std::size_t camel_count_on_board(const camelup::GameState& state) {
    std::size_t total = 0;
    for (const auto& stack : state.board) {
        total += stack.size();
    }
    return total;
}

int count_actions_by_type(const std::vector<camelup::Action>& actions, camelup::ActionType type) {
    int count = 0;
    for (const auto& action : actions) {
        if (action.type() == type) {
            ++count;
        }
    }
    return count;
}

bool has_take_leg_ticket_for_camel(const std::vector<camelup::Action>& actions, camelup::CamelId camel) {
    for (const auto& action : actions) {
        if (action.type() != camelup::ActionType::TakeLegTicket) {
            continue;
        }
        const auto payload = std::get<camelup::TakeLegTicketPayload>(action.payload);
        if (payload.camel == camel) {
            return true;
        }
    }
    return false;
}

bool has_winner_bet_for_camel(const std::vector<camelup::Action>& actions, camelup::CamelId camel) {
    for (const auto& action : actions) {
        if (action.type() != camelup::ActionType::BetWinner) {
            continue;
        }
        const auto payload = std::get<camelup::BetWinnerPayload>(action.payload);
        if (payload.camel == camel) {
            return true;
        }
    }
    return false;
}

bool has_loser_bet_for_camel(const std::vector<camelup::Action>& actions, camelup::CamelId camel) {
    for (const auto& action : actions) {
        if (action.type() != camelup::ActionType::BetLoser) {
            continue;
        }
        const auto payload = std::get<camelup::BetLoserPayload>(action.payload);
        if (payload.camel == camel) {
            return true;
        }
    }
    return false;
}

bool has_desert_placement_on_tile(const std::vector<camelup::Action>& actions, int tile) {
    for (const auto& action : actions) {
        if (action.type() != camelup::ActionType::PlaceDesertTile) {
            continue;
        }
        const auto payload = std::get<camelup::PlaceDesertTilePayload>(action.payload);
        if (payload.tile == tile) {
            return true;
        }
    }
    return false;
}

std::pair<int, int> find_camel_on_board(const camelup::GameState& state, camelup::CamelId camel) {
    for (int tile = 0; tile < camelup::kBoardTiles; ++tile) {
        const auto& stack = state.board[tile];
        for (int idx = 0; idx < static_cast<int>(stack.size()); ++idx) {
            if (stack[idx] == camel) {
                return {tile, idx};
            }
        }
    }
    return {-1, -1};
}

}  // namespace

int main() {
    // TODO: Expand coverage with v1 rule tests: action legality, stack ordering edge cases, payouts, and seed replay determinism
    camelup::Engine engine(7);
    auto state = engine.new_game(3);

    assert(state.player_count == 3);
    assert(state.current_player == 0);
    assert(camel_count_on_board(state) == camelup::kCamelCount);
    assert(state.winner_bet_stack.empty());
    assert(state.loser_bet_stack.empty());

    for (int tile = 0; tile < camelup::kBoardTiles; ++tile) {
        assert(state.desert_tile_owner[tile] == -1);
    }

    for (camelup::CamelId camel = 0; camel < static_cast<camelup::CamelId>(camelup::kCamelCount); ++camel) {
        assert(state.leg_tickets_remaining[camel] == camelup::kLegTicketCount);
        assert(state.leg_ticket_values[camel][0] == 5);
        assert(state.leg_ticket_values[camel][1] == 3);
        assert(state.leg_ticket_values[camel][2] == 2);
    }

    for (int player = 0; player < camelup::kMaxPlayers; ++player) {
        const bool active_player = player < state.player_count;
        for (camelup::CamelId camel = 0; camel < static_cast<camelup::CamelId>(camelup::kCamelCount); ++camel) {
            assert(state.winner_bet_card_available[player][camel] == active_player);
            assert(state.loser_bet_card_available[player][camel] == active_player);
        }
    }

    auto legal = engine.legal_actions(state);
    assert(count_actions_by_type(legal, camelup::ActionType::RollDie) == 1);
    assert(count_actions_by_type(legal, camelup::ActionType::PlaceDesertTile) == (camelup::kBoardTiles - 2) * 2);
    assert(count_actions_by_type(legal, camelup::ActionType::TakeLegTicket) == camelup::kCamelCount);
    assert(count_actions_by_type(legal, camelup::ActionType::BetWinner) == camelup::kCamelCount);
    assert(count_actions_by_type(legal, camelup::ActionType::BetLoser) == camelup::kCamelCount);

    for (const auto& action : legal) {
        if (action.type() != camelup::ActionType::PlaceDesertTile) {
            continue;
        }
        const auto payload = std::get<camelup::PlaceDesertTilePayload>(action.payload);
        assert(payload.tile >= 1);
        assert(payload.tile < camelup::kBoardTiles - 1);
        assert(payload.move_delta == 1 || payload.move_delta == -1);
    }

    {
        auto modified = state;
        modified.leg_tickets_remaining[camelup::Camel::Blue] = 0;
        const auto modified_legal = engine.legal_actions(modified);
        assert(!has_take_leg_ticket_for_camel(modified_legal, camelup::Camel::Blue));
    }

    {
        auto modified = state;
        const auto current_player = modified.current_player;
        modified.winner_bet_card_available[current_player][camelup::Camel::Green] = false;
        modified.loser_bet_card_available[current_player][camelup::Camel::Orange] = false;
        const auto modified_legal = engine.legal_actions(modified);
        assert(!has_winner_bet_for_camel(modified_legal, camelup::Camel::Green));
        assert(!has_loser_bet_for_camel(modified_legal, camelup::Camel::Orange));
    }

    {
        auto modified = state;
        modified.desert_tile_owner[5] = 1;
        modified.desert_tiles[1] = {5, 1};
        modified.board[7].push_back(modified.board[0].back());
        modified.board[0].pop_back();
        const auto modified_legal = engine.legal_actions(modified);
        assert(!has_desert_placement_on_tile(modified_legal, 4));
        assert(!has_desert_placement_on_tile(modified_legal, 5));
        assert(!has_desert_placement_on_tile(modified_legal, 6));
        assert(!has_desert_placement_on_tile(modified_legal, 7));
    }

    {
        auto modified = state;
        const auto current_player = modified.current_player;
        modified.desert_tile_owner[8] = current_player;
        modified.desert_tiles[current_player] = {8, 1};
        const auto modified_legal = engine.legal_actions(modified);
        assert(has_desert_placement_on_tile(modified_legal, 7));
        assert(has_desert_placement_on_tile(modified_legal, 8));
        assert(has_desert_placement_on_tile(modified_legal, 9));
    }

    {
        auto modified = state;
        const auto after_place = engine.apply_action(modified, camelup::Action::place_desert_tile(3, 1));
        assert(after_place.desert_tiles[0].tile == 3);
        assert(after_place.desert_tiles[0].move_delta == 1);
        assert(after_place.desert_tile_owner[3] == 0);
        assert(after_place.current_player == 1);
    }

    {
        auto modified = state;
        modified.desert_tile_owner[4] = 0;
        modified.desert_tiles[0] = {4, 1};
        const auto after_move = engine.apply_action(modified, camelup::Action::place_desert_tile(7, -1));
        assert(after_move.desert_tile_owner[4] == -1);
        assert(after_move.desert_tile_owner[7] == 0);
        assert(after_move.desert_tiles[0].tile == 7);
        assert(after_move.desert_tiles[0].move_delta == -1);
        assert(after_move.current_player == 1);
    }

    {
        bool threw = false;
        try {
            static_cast<void>(engine.apply_action(state, camelup::Action::place_desert_tile(0, 1)));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    {
        auto modified = state;
        modified.desert_tile_owner[5] = 1;
        modified.desert_tiles[1] = {5, 1};
        bool threw = false;
        try {
            static_cast<void>(engine.apply_action(modified, camelup::Action::place_desert_tile(4, 1)));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    {
        auto modified = state;
        const auto after_first = engine.apply_action(modified, camelup::Action::take_leg_ticket(camelup::Camel::Blue));
        assert(after_first.player_leg_tickets[0].size() == 1);
        assert(after_first.player_leg_tickets[0][0].camel == camelup::Camel::Blue);
        assert(after_first.player_leg_tickets[0][0].value == 5);
        assert(after_first.leg_tickets_remaining[camelup::Camel::Blue] == 2);
        assert(after_first.current_player == 1);
    }

    {
        auto modified = state;
        modified = engine.apply_action(modified, camelup::Action::take_leg_ticket(camelup::Camel::Blue));
        modified = engine.apply_action(modified, camelup::Action::take_leg_ticket(camelup::Camel::Blue));
        modified = engine.apply_action(modified, camelup::Action::take_leg_ticket(camelup::Camel::Blue));

        assert(modified.player_leg_tickets[0].size() == 1);
        assert(modified.player_leg_tickets[1].size() == 1);
        assert(modified.player_leg_tickets[2].size() == 1);
        assert(modified.player_leg_tickets[0][0].value == 5);
        assert(modified.player_leg_tickets[1][0].value == 3);
        assert(modified.player_leg_tickets[2][0].value == 2);
        assert(modified.leg_tickets_remaining[camelup::Camel::Blue] == 0);

        const auto legal_after_exhaustion = engine.legal_actions(modified);
        assert(!has_take_leg_ticket_for_camel(legal_after_exhaustion, camelup::Camel::Blue));

        bool threw = false;
        try {
            static_cast<void>(engine.apply_action(modified, camelup::Action::take_leg_ticket(camelup::Camel::Blue)));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    {
        auto modified = state;
        const auto after_bet = engine.apply_action(modified, camelup::Action::bet_winner(camelup::Camel::Green));
        assert(after_bet.winner_bet_stack.size() == 1);
        assert(after_bet.winner_bet_stack[0].player == 0);
        assert(after_bet.winner_bet_stack[0].camel == camelup::Camel::Green);
        assert(!after_bet.winner_bet_card_available[0][camelup::Camel::Green]);
        assert(after_bet.current_player == 1);
    }

    {
        auto modified = state;
        const auto after_bet = engine.apply_action(modified, camelup::Action::bet_loser(camelup::Camel::Orange));
        assert(after_bet.loser_bet_stack.size() == 1);
        assert(after_bet.loser_bet_stack[0].player == 0);
        assert(after_bet.loser_bet_stack[0].camel == camelup::Camel::Orange);
        assert(!after_bet.loser_bet_card_available[0][camelup::Camel::Orange]);
        assert(after_bet.current_player == 1);
    }

    {
        auto modified = state;
        modified = engine.apply_action(modified, camelup::Action::bet_winner(camelup::Camel::Green));
        modified.current_player = 0;
        const auto legal_after_use = engine.legal_actions(modified);
        assert(!has_winner_bet_for_camel(legal_after_use, camelup::Camel::Green));

        bool threw = false;
        try {
            static_cast<void>(engine.apply_action(modified, camelup::Action::bet_winner(camelup::Camel::Green)));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    {
        auto modified = state;
        modified = engine.apply_action(modified, camelup::Action::bet_loser(camelup::Camel::Orange));
        modified.current_player = 0;
        const auto legal_after_use = engine.legal_actions(modified);
        assert(!has_loser_bet_for_camel(legal_after_use, camelup::Camel::Orange));

        bool threw = false;
        try {
            static_cast<void>(engine.apply_action(modified, camelup::Action::bet_loser(camelup::Camel::Orange)));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    {
        auto modified = state;
        for (auto& stack : modified.board) {
            stack.clear();
        }
        modified.current_player = 0;
        modified.player_count = 4;
        modified.die_available.fill(false);
        modified.die_available[camelup::Camel::Blue] = true;

        modified.board[0] = {camelup::Camel::Blue};
        modified.board[2] = {camelup::Camel::Green};
        modified.board[3] = {camelup::Camel::Yellow};
        modified.board[4] = {camelup::Camel::Orange};

        modified.desert_tile_owner.fill(-1);
        modified.desert_tile_owner[1] = 1;
        modified.desert_tile_owner[2] = 2;
        modified.desert_tile_owner[3] = 3;
        modified.desert_tiles[1] = {1, 1};
        modified.desert_tiles[2] = {2, 1};
        modified.desert_tiles[3] = {3, 1};

        const int money_before_sum = modified.money[0] + modified.money[1] + modified.money[2] + modified.money[3];
        const auto after_roll = engine.apply_action(modified, camelup::Action::roll_die());
        const int money_after_sum = after_roll.money[0] + after_roll.money[1] + after_roll.money[2] + after_roll.money[3];
        assert(money_after_sum == money_before_sum + 2);

        const auto [tile, idx] = find_camel_on_board(after_roll, camelup::Camel::Blue);
        assert(tile >= 2 && tile <= 4);
        assert(after_roll.board[tile].size() == 2);
        assert(idx == static_cast<int>(after_roll.board[tile].size()) - 1);
    }

    {
        auto modified = state;
        for (auto& stack : modified.board) {
            stack.clear();
        }
        modified.current_player = 0;
        modified.player_count = 4;
        modified.die_available.fill(false);
        modified.die_available[camelup::Camel::Blue] = true;

        modified.board[0] = {camelup::Camel::Green, camelup::Camel::Blue};
        modified.board[1] = {camelup::Camel::Yellow};
        modified.board[2] = {camelup::Camel::Orange};

        modified.desert_tile_owner.fill(-1);
        modified.desert_tile_owner[1] = 1;
        modified.desert_tile_owner[2] = 2;
        modified.desert_tile_owner[3] = 3;
        modified.desert_tiles[1] = {1, -1};
        modified.desert_tiles[2] = {2, -1};
        modified.desert_tiles[3] = {3, -1};

        const int money_before_sum = modified.money[0] + modified.money[1] + modified.money[2] + modified.money[3];
        const auto after_roll = engine.apply_action(modified, camelup::Action::roll_die());
        const int money_after_sum = after_roll.money[0] + after_roll.money[1] + after_roll.money[2] + after_roll.money[3];
        assert(money_after_sum == money_before_sum + 2);

        const auto [tile, idx] = find_camel_on_board(after_roll, camelup::Camel::Blue);
        assert(tile >= 0 && tile <= 2);
        assert(after_roll.board[tile].size() == 2);
        assert(idx == 0);
    }

    {
        auto modified = state;
        for (auto& stack : modified.board) {
            stack.clear();
        }
        modified.money.fill(3);
        modified.player_count = 4;
        modified.current_player = 0;
        modified.terminal = false;
        modified.die_available.fill(false);
        modified.die_available[camelup::Camel::Blue] = true;

        modified.board[15] = {camelup::Camel::Blue};
        modified.board[10] = {camelup::Camel::Green};
        modified.board[7] = {camelup::Camel::Yellow};
        modified.board[5] = {camelup::Camel::Orange};
        modified.board[1] = {camelup::Camel::White};

        modified.winner_bet_stack = {
            {0, camelup::Camel::Blue},
            {1, camelup::Camel::Green},
            {2, camelup::Camel::Blue},
            {3, camelup::Camel::Blue},
        };
        modified.loser_bet_stack = {
            {1, camelup::Camel::White},
            {2, camelup::Camel::Orange},
            {3, camelup::Camel::White},
        };

        const auto after_finish = engine.apply_action(modified, camelup::Action::roll_die());
        assert(after_finish.terminal);
        assert(after_finish.money[0] == 12);
        assert(after_finish.money[1] == 10);
        assert(after_finish.money[2] == 7);
        assert(after_finish.money[3] == 11);

        const auto after_extra = engine.apply_action(after_finish, camelup::Action::roll_die());
        assert(after_extra.money == after_finish.money);
    }

    const auto before_player = state.current_player;
    const auto before_money = state.money[before_player];

    state = engine.apply_action(state, camelup::Action::roll_die());
    assert(camel_count_on_board(state) == camelup::kCamelCount);
    assert(state.current_player != before_player);
    assert(state.money[before_player] == before_money + 1);

    // Consume all dice
    for (int i = 0; i < camelup::kCamelCount - 1; ++i) {
        state = engine.apply_action(state, camelup::Action::roll_die());
    }
    assert(state.leg_number == 1);

    // Next roll starts a new leg
    state = engine.apply_action(state, camelup::Action::roll_die());
    assert(state.leg_number == 2);

    return 0;
}
