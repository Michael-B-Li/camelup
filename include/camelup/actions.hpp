#pragma once

#include <variant>

#include "camelup/types.hpp"

namespace camelup {

enum class ActionType {
    RollDie,
    PlaceDesertTile,
    TakeLegTicket,
    BetWinner,
    BetLoser
};

// Roll one available die and move the corresponding camel stack
struct RollDiePayload {};

// Place a desert tile at `tile` with movement effect `move_delta` (+1 or -1 in full rules)
struct PlaceDesertTilePayload {
    int tile{-1};
    int move_delta{1};
};

// Take a leg betting ticket for the specified camel
struct TakeLegTicketPayload {
    CamelId camel{0};
};

// Place a game winner bet card for the specified camel
struct BetWinnerPayload {
    CamelId camel{0};
};

// Place a game loser bet card for the specified camel
struct BetLoserPayload {
    CamelId camel{0};
};

// Tagged payload for all action-specific data
using ActionPayload = std::variant<
    RollDiePayload,
    PlaceDesertTilePayload,
    TakeLegTicketPayload,
    BetWinnerPayload,
    BetLoserPayload>;

// Action envelope containing both an explicit action tag and typed payload
struct Action {
    ActionPayload payload{RollDiePayload{}};

    // Typed constructors initialise payload only so action kind is always derived from payload
    explicit Action(RollDiePayload roll = {}) noexcept
        : payload(roll) {}
    explicit Action(PlaceDesertTilePayload place) noexcept
        : payload(place) {}
    explicit Action(TakeLegTicketPayload ticket) noexcept
        : payload(ticket) {}
    explicit Action(BetWinnerPayload winner) noexcept
        : payload(winner) {}
    explicit Action(BetLoserPayload loser) noexcept
        : payload(loser) {}

    [[nodiscard]] ActionType type() const noexcept {
        if (std::holds_alternative<RollDiePayload>(payload)) {
            return ActionType::RollDie;
        }
        if (std::holds_alternative<PlaceDesertTilePayload>(payload)) {
            return ActionType::PlaceDesertTile;
        }
        if (std::holds_alternative<TakeLegTicketPayload>(payload)) {
            return ActionType::TakeLegTicket;
        }
        if (std::holds_alternative<BetWinnerPayload>(payload)) {
            return ActionType::BetWinner;
        }
        return ActionType::BetLoser;
    }

    // Convenience factories for call sites
    static Action roll_die() noexcept { return Action(RollDiePayload{}); }
    static Action place_desert_tile(int tile, int move_delta) noexcept {
        return Action(PlaceDesertTilePayload{tile, move_delta});
    }
    static Action take_leg_ticket(CamelId camel) noexcept {
        return Action(TakeLegTicketPayload{camel});
    }
    static Action bet_winner(CamelId camel) noexcept { return Action(BetWinnerPayload{camel}); }
    static Action bet_loser(CamelId camel) noexcept { return Action(BetLoserPayload{camel}); }
};

}  // namespace camelup
