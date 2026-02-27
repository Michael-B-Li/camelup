#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "camelup/actions.hpp"
#include "camelup/engine.hpp"
#include "camelup/types.hpp"

namespace {

char camel_symbol(camelup::CamelId camel) {
    switch (static_cast<camelup::Camel>(camel)) {
        case camelup::Camel::Blue:
            return 'B';
        case camelup::Camel::Green:
            return 'G';
        case camelup::Camel::Yellow:
            return 'Y';
        case camelup::Camel::Orange:
            return 'O';
        case camelup::Camel::White:
            return 'W';
    }
    return '?';
}

std::string camel_name(camelup::CamelId camel) {
    switch (static_cast<camelup::Camel>(camel)) {
        case camelup::Camel::Blue:
            return "Blue";
        case camelup::Camel::Green:
            return "Green";
        case camelup::Camel::Yellow:
            return "Yellow";
        case camelup::Camel::Orange:
            return "Orange";
        case camelup::Camel::White:
            return "White";
    }
    return "Unknown";
}

void print_board(const camelup::GameState& state) {
    std::cout << "\nBoard state\n";
    for (int tile = 0; tile < camelup::kBoardTiles; ++tile) {
        if (state.board[tile].empty()) {
            continue;
        }
        std::cout << "Tile " << tile << ": [";
        for (std::size_t i = 0; i < state.board[tile].size(); ++i) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << camel_symbol(state.board[tile][i]);
        }
        std::cout << "]\n";
    }
}

void print_race_order(const camelup::GameState& state) {
    std::vector<char> order;
    order.reserve(camelup::kCamelCount);
    for (int tile = camelup::kBoardTiles - 1; tile >= 0; --tile) {
        const auto& stack = state.board[tile];
        for (int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            order.push_back(camel_symbol(stack[static_cast<std::size_t>(i)]));
        }
    }

    std::cout << "Race order (1st -> last): ";
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (i > 0) {
            std::cout << " > ";
        }
        std::cout << order[i];
    }
    std::cout << '\n';
}

void print_desert_tiles(const camelup::GameState& state) {
    std::cout << "Desert tiles: ";
    bool printed = false;
    for (int player = 0; player < state.player_count; ++player) {
        const auto& tile = state.desert_tiles[player];
        if (tile.tile < 0) {
            continue;
        }
        if (printed) {
            std::cout << ", ";
        }
        std::cout << "P" << player << "@" << tile.tile << " (" << (tile.move_delta >= 0 ? "+" : "") << tile.move_delta << ")";
        printed = true;
    }
    if (!printed) {
        std::cout << "(none)";
    }
    std::cout << '\n';
}

void print_leg_tickets(const camelup::GameState& state) {
    std::cout << "Leg tickets remaining: ";
    for (camelup::CamelId camel = 0; camel < static_cast<camelup::CamelId>(camelup::kCamelCount); ++camel) {
        if (camel > 0) {
            std::cout << ", ";
        }
        std::cout << camel_symbol(camel) << "=" << state.leg_tickets_remaining[camel];
    }
    std::cout << '\n';

    for (int player = 0; player < state.player_count; ++player) {
        std::cout << "P" << player << " leg bets: ";
        const auto& tickets = state.player_leg_tickets[player];
        if (tickets.empty()) {
            std::cout << "(none)";
        } else {
            for (std::size_t i = 0; i < tickets.size(); ++i) {
                if (i > 0) {
                    std::cout << ", ";
                }
                std::cout << camel_symbol(tickets[i].camel) << ":" << tickets[i].value;
            }
        }
        std::cout << '\n';
    }
}

void print_final_bets(const camelup::GameState& state) {
    std::cout << "Winner bet stack: ";
    if (state.winner_bet_stack.empty()) {
        std::cout << "(none)";
    } else {
        for (std::size_t i = 0; i < state.winner_bet_stack.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            const auto& card = state.winner_bet_stack[i];
            std::cout << "P" << static_cast<int>(card.player) << ":" << camel_symbol(card.camel);
        }
    }
    std::cout << '\n';

    std::cout << "Loser bet stack: ";
    if (state.loser_bet_stack.empty()) {
        std::cout << "(none)";
    } else {
        for (std::size_t i = 0; i < state.loser_bet_stack.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            const auto& card = state.loser_bet_stack[i];
            std::cout << "P" << static_cast<int>(card.player) << ":" << camel_symbol(card.camel);
        }
    }
    std::cout << '\n';
}

std::string action_label(const camelup::Action& action) {
    switch (action.type()) {
        case camelup::ActionType::RollDie:
            return "Roll die";
        case camelup::ActionType::PlaceDesertTile: {
            const auto payload = std::get<camelup::PlaceDesertTilePayload>(action.payload);
            return "Place desert tile at " + std::to_string(payload.tile) + " (" +
                   (payload.move_delta >= 0 ? "+" : "") + std::to_string(payload.move_delta) + ")";
        }
        case camelup::ActionType::TakeLegTicket: {
            const auto payload = std::get<camelup::TakeLegTicketPayload>(action.payload);
            return "Take leg ticket: " + camel_name(payload.camel);
        }
        case camelup::ActionType::BetWinner: {
            const auto payload = std::get<camelup::BetWinnerPayload>(action.payload);
            return "Bet winner: " + camel_name(payload.camel);
        }
        case camelup::ActionType::BetLoser: {
            const auto payload = std::get<camelup::BetLoserPayload>(action.payload);
            return "Bet loser: " + camel_name(payload.camel);
        }
    }
    return "Unknown action";
}

void print_legal_actions(const std::vector<camelup::Action>& actions) {
    std::cout << "Legal actions\n";
    for (std::size_t i = 0; i < actions.size(); ++i) {
        std::cout << "  [" << i << "] " << action_label(actions[i]) << '\n';
    }
}

int find_roll_action_index(const std::vector<camelup::Action>& actions) {
    for (std::size_t i = 0; i < actions.size(); ++i) {
        if (actions[i].type() == camelup::ActionType::RollDie) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void print_status(const camelup::GameState& state, int turn) {
    std::cout << "\n===== Camel Up v1 UI =====\n";
    std::cout << "Turn: " << turn << " | Leg: " << state.leg_number
              << " | Current player: P" << static_cast<int>(state.current_player) << '\n';

    std::cout << "Money: ";
    for (int p = 0; p < state.player_count; ++p) {
        if (p > 0) {
            std::cout << ", ";
        }
        std::cout << "P" << p << "=" << state.money[p];
    }
    std::cout << '\n';

    std::cout << "Dice remaining this leg: ";
    bool printed = false;
    for (camelup::CamelId camel = 0; camel < static_cast<camelup::CamelId>(camelup::kCamelCount); ++camel) {
        if (!state.die_available[camel]) {
            continue;
        }
        if (printed) {
            std::cout << ' ';
        }
        std::cout << camel_symbol(camel);
        printed = true;
    }
    if (!printed) {
        std::cout << "(none)";
    }
    std::cout << '\n';

    print_board(state);
    print_race_order(state);
    print_desert_tiles(state);
    print_leg_tickets(state);
    print_final_bets(state);
}

bool parse_int_arg(const char* value, int& out) {
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        return false;
    }
    out = static_cast<int>(parsed);
    return true;
}

void print_usage() {
    std::cout << "Usage: camelup_ui [--seed N] [--players N] [--turn-limit N] [--auto]\n";
}

}  // namespace

int main(int argc, char** argv) {
    int seed = 42;
    int players = 2;
    int turn_limit = 200;
    bool auto_mode = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--auto") {
            auto_mode = true;
            continue;
        }
        if (arg == "--seed" || arg == "--players" || arg == "--turn-limit") {
            if (i + 1 >= argc) {
                print_usage();
                return 1;
            }
            int parsed = 0;
            if (!parse_int_arg(argv[++i], parsed)) {
                print_usage();
                return 1;
            }
            if (arg == "--seed") {
                seed = parsed;
            } else if (arg == "--players") {
                players = parsed;
            } else {
                turn_limit = parsed;
            }
            continue;
        }

        print_usage();
        return 1;
    }

    camelup::Engine engine(static_cast<std::uint32_t>(seed));
    camelup::GameState state;
    try {
        state = engine.new_game(players);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to create game: " << ex.what() << '\n';
        return 1;
    }

    int turn = 0;
    while (!state.terminal && turn < turn_limit) {
        print_status(state, turn);
        const auto legal_actions = engine.legal_actions(state);
        if (legal_actions.empty()) {
            std::cout << "No legal actions available\n";
            break;
        }

        int chosen_index = -1;
        const int roll_index = find_roll_action_index(legal_actions);

        if (auto_mode) {
            chosen_index = roll_index >= 0 ? roll_index : 0;
        } else {
            print_legal_actions(legal_actions);
            std::cout << "Command: index, r=roll, a=auto-roll, q=quit: ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                break;
            }
            if (input == "q") {
                break;
            }
            if (input == "a") {
                auto_mode = true;
                chosen_index = roll_index >= 0 ? roll_index : 0;
            } else if (input == "r") {
                chosen_index = roll_index >= 0 ? roll_index : 0;
            } else {
                int parsed = 0;
                if (!parse_int_arg(input.c_str(), parsed)) {
                    std::cout << "Invalid command\n";
                    continue;
                }
                if (parsed < 0 || parsed >= static_cast<int>(legal_actions.size())) {
                    std::cout << "Action index out of range\n";
                    continue;
                }
                chosen_index = parsed;
            }
        }

        state = engine.apply_action(state, legal_actions[static_cast<std::size_t>(chosen_index)]);
        ++turn;
    }

    print_status(state, turn);
    if (state.terminal) {
        std::cout << "Game finished: a camel reached tile " << camelup::kBoardTiles - 1 << ".\n";
    } else if (turn >= turn_limit) {
        std::cout << "Stopped at turn limit.\n";
    } else {
        std::cout << "Exited before terminal state.\n";
    }

    return 0;
}
