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
}

bool parse_int_arg(char* value, int& out) {
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

        if (!auto_mode) {
            std::cout << "Command: [Enter]=roll, a=auto, q=quit: ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                break;
            }
            if (input == "q") {
                break;
            }
            if (input == "a") {
                auto_mode = true;
            }
        }

        state = engine.apply_action(state, camelup::Action::roll_die());
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
