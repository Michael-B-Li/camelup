# Camel Up Simulation Engine

Camel Up simulation engine and optional UI viewer.
This project targets **Camel Up v1** (5 racing camels, no crazy camels).

## Layout

- `include/camelup/`: public engine headers
- `include/camelup/rules/`: rules module headers
- `src/`: engine implementation and simple CLI entrypoint
- `src/rules/`: rules module implementations
- `src/ui_main.cpp`: optional terminal UI viewer
- `tests/`: minimal sanity tests

## Build

```bash
cmake -S . -B build
cmake --build build
```

Disable UI target if needed:

```bash
cmake -S . -B build -DCAMELUP_BUILD_UI=OFF
```

## Run

```bash
./build/camelup
./build/camelup_ui
ctest --test-dir build
```

UI usage:

```bash
./build/camelup_ui --seed 42 --players 3
./build/camelup_ui --auto --turn-limit 150
```

## Current status

- Supports deterministic seeded game creation
- Implements Camel Up v1 opening setup rolls (each camel rolled once onto tiles 1 to 3)
- Supports die rolling and camel stack movement (including carrying stacked camels)
- Applies desert tile effects during movement
  - Oasis `+1` and mirage `-1`
  - Desert tile owner gains 1 coin on trigger
  - Correct over/under stacking behaviour after tile effect
- Models key Camel Up v1 state data
  - Desert tile ownership and placement slots
  - Leg ticket pools and player leg tickets
  - Winner/loser bet stacks and per-player final bet card availability
- Uses typed action payloads via `std::variant`
- Legal action generation is in a dedicated rules module
  - Enforces desert tile placement constraints
  - Enforces leg ticket exhaustion
  - Enforces winner/loser card availability
- Implements non-roll action effects in `apply_action`
  - Place desert tile with replace semantics
  - Take leg ticket with 5/3/2 value progression
  - Bet winner and bet loser with card consumption
- Implements leg-end resolution
  - Scores leg tickets against current 1st/2nd race order
  - Clears leg tickets
  - Resets leg ticket availability
  - Removes desert tiles
  - Resets dice and increments leg number
- Implements end-of-game winner/loser bet payout resolution
- Optional terminal UI to watch state progression turn-by-turn

### Not implemented yet

- Full rules audit against official Camel Up v1 wording and edge cases
- Broader test suite
- Production-style CLI runner replacing the current demo entrypoint
