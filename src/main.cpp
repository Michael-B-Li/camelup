#include <iostream>

#include "camelup/actions.hpp"
#include "camelup/engine.hpp"

int main() {
    // TODO: add a proper CLI runner for full v1 gameplay flows
    camelup::Engine engine(42);
    auto state = engine.new_game(2);

    for (int turn = 0; turn < 10 && !state.terminal; ++turn) {
        state = engine.apply_action(state, camelup::Action::roll_die());
    }

    std::cout << "Game snapshot after 10 rolls or terminal state\n";
    for (int tile = 0; tile < camelup::kBoardTiles; ++tile) {
        if (state.board[tile].empty()) {
            continue;
        }
        std::cout << "Tile " << tile << ": ";
        for (auto camel : state.board[tile]) {
            std::cout << static_cast<int>(camel) << ' ';
        }
        std::cout << '\n';
    }

    return 0;
}
