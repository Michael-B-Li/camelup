#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "camelup/actions.hpp"
#include "camelup/engine.hpp"
#include "camelup/types.hpp"

namespace {

enum class Policy {
    RollOnly,
    FirstLegal,
    RandomLegal
};

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

bool parse_int_arg(const char* value, int& out) {
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        return false;
    }
    out = static_cast<int>(parsed);
    return true;
}

bool parse_policy(const std::string& value, Policy& out) {
    if (value == "roll") {
        out = Policy::RollOnly;
        return true;
    }
    if (value == "first") {
        out = Policy::FirstLegal;
        return true;
    }
    if (value == "random") {
        out = Policy::RandomLegal;
        return true;
    }
    return false;
}

void print_usage() {
    std::cout
        << "Usage: camelup [--seed N] [--players N] [--turn-limit N] [--policy roll|first|random] [--verbose]\n";
}

std::vector<camelup::CamelId> build_race_order(const camelup::GameState& state) {
    std::vector<camelup::CamelId> order;
    order.reserve(camelup::kCamelCount);
    for (int tile = camelup::kBoardTiles - 1; tile >= 0; --tile) {
        const auto& stack = state.board[tile];
        for (int idx = static_cast<int>(stack.size()) - 1; idx >= 0; --idx) {
            order.push_back(stack[static_cast<std::size_t>(idx)]);
        }
    }
    return order;
}

void print_summary(const camelup::GameState& state, int turns_played) {
    std::cout << "Turns played: " << turns_played << '\n';
    std::cout << "Leg: " << state.leg_number << '\n';
    std::cout << "Terminal: " << (state.terminal ? "yes" : "no") << '\n';

    const auto race_order = build_race_order(state);
    std::cout << "Race order (1st -> last): ";
    for (std::size_t i = 0; i < race_order.size(); ++i) {
        if (i > 0) {
            std::cout << " > ";
        }
        std::cout << camel_symbol(race_order[i]);
    }
    std::cout << '\n';

    std::cout << "Money: ";
    for (int player = 0; player < state.player_count; ++player) {
        if (player > 0) {
            std::cout << ", ";
        }
        std::cout << "P" << player << "=" << state.money[player];
    }
    std::cout << '\n';

    std::cout << "Board\n";
    for (int tile = 0; tile < camelup::kBoardTiles; ++tile) {
        if (state.board[tile].empty()) {
            continue;
        }
        std::cout << "  Tile " << tile << ": [";
        for (std::size_t i = 0; i < state.board[tile].size(); ++i) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << camel_symbol(state.board[tile][i]);
        }
        std::cout << "]\n";
    }
}

const camelup::Action& choose_action(const std::vector<camelup::Action>& legal_actions,
                                     Policy policy,
                                     std::mt19937& chooser_rng) {
    if (legal_actions.empty()) {
        throw std::runtime_error("no legal actions available");
    }

    if (policy == Policy::RollOnly) {
        for (const auto& action : legal_actions) {
            if (action.type() == camelup::ActionType::RollDie) {
                return action;
            }
        }
        return legal_actions.front();
    }

    if (policy == Policy::FirstLegal) {
        return legal_actions.front();
    }

    std::uniform_int_distribution<std::size_t> pick(0, legal_actions.size() - 1);
    return legal_actions[pick(chooser_rng)];
}

}  // namespace

int main(int argc, char** argv) {
    int seed = 42;
    int players = 2;
    int turn_limit = 500;
    bool verbose = false;
    Policy policy = Policy::RollOnly;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--verbose") {
            verbose = true;
            continue;
        }
        if (arg == "--seed" || arg == "--players" || arg == "--turn-limit" || arg == "--policy") {
            if (i + 1 >= argc) {
                print_usage();
                return 1;
            }

            if (arg == "--policy") {
                if (!parse_policy(argv[++i], policy)) {
                    print_usage();
                    return 1;
                }
                continue;
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

    try {
        camelup::Engine engine(static_cast<std::uint32_t>(seed));
        auto state = engine.new_game(players);
        std::mt19937 chooser_rng(static_cast<std::uint32_t>(seed ^ 0x9e3779b9U));

        int turn = 0;
        while (!state.terminal && turn < turn_limit) {
            const auto legal_actions = engine.legal_actions(state);
            const auto& action = choose_action(legal_actions, policy, chooser_rng);

            if (verbose) {
                std::cout << "Turn " << turn << " P" << static_cast<int>(state.current_player)
                          << " -> " << action_label(action) << '\n';
            }

            state = engine.apply_action(state, action);
            ++turn;
        }

        print_summary(state, turn);
        if (!state.terminal && turn >= turn_limit) {
            std::cout << "Stopped at turn limit\n";
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "camelup failed: " << ex.what() << '\n';
        return 1;
    }
}
