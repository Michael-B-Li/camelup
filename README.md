# Camel Up Simulation Engine

Camel Up simulation engine and optional UI viewer.
This project targets **Camel Up v1** (5 racing camels, no crazy camels).

## Layout

- `include/camelup/`: public engine headers
- `src/`: engine implementation and simple CLI entrypoint
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
- Supports die rolling and camel stack movement
- Includes placeholder action types for future rule expansion
- Optional terminal UI to watch state progression turn-by-turn
