#include <cassert>
#include <cstddef>

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

}  // namespace

int main() {
    // TODO: Expand coverage with v1 rule tests: action legality, stack ordering edge cases, payouts, and seed replay determinism
    camelup::Engine engine(7);
    auto state = engine.new_game(3);

    assert(state.player_count == 3);
    assert(state.current_player == 0);
    assert(camel_count_on_board(state) == camelup::kCamelCount);

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
