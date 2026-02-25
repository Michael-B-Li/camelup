#pragma once

#include <vector>

#include "camelup/actions.hpp"
#include "camelup/game_state.hpp"

namespace camelup::rules {

std::vector<Action> legal_actions(const GameState& state);

}  // namespace camelup::rules
