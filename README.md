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
- Supports die rolling and camel stack movement (including carrying stacked camels)
- Models key Camel Up v1 state data
  - Desert tile ownership and placement slots
  - Leg ticket pools and player leg tickets
  - Winner/loser bet stacks and per-player final bet card availability
- Uses typed action payloads via `std::variant`
- Legal action generation is in a dedicated rules module
  - Enforces desert tile placement constraints
  - Enforces leg ticket exhaustion
  - Enforces winner/loser card availability
- Optional terminal UI to watch state progression turn-by-turn

### Not implemented yet

- Full non-roll action effects in `apply_action`
- Desert tile movement effects during camel movement (`+1` oasis, `-1` mirage)
- Camel Up v1 initial opening roll setup
- End-of-game winner/loser payout resolution
