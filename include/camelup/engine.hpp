#pragma once

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "camelup/actions.hpp"
#include "camelup/game_state.hpp"

namespace camelup {

class Engine {
public:
    explicit Engine(std::uint32_t seed = std::random_device{}());

    GameState new_game(int player_count);
    std::vector<Action> legal_actions(const GameState& state) const;
    GameState apply_action(const GameState& state, const Action& action);

private:
    std::mt19937 rng_;

    static void reset_leg_dice(GameState& state);
    std::pair<CamelId, int> roll_die(GameState& state);
    static void move_camel_stack(GameState& state, CamelId camel, int distance);
};

}  // namespace camelup
