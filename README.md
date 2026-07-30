# ForgeSight

Qt/C++ desktop platform for real-time industrial device monitoring — live dashboards, alarm rules, and historical analytics over MQTT-fed sensor data.

## Prerequisites

- Qt 6.4+ (Quick, Charts, WebSockets, HttpServer, Network)
- CMake 3.20+, C++20 compiler
- PostgreSQL with database `forgesight`
- Mosquitto MQTT broker on `localhost:1883`
- Python 3 + `paho-mqtt`, `PyYAML` (simulators)

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Run the local stack

```bash
# Start API (:8080), WebSocket (:8081), ingestion, alarm-engine, simulators
chmod +x scripts/dev-up.sh
./scripts/dev-up.sh

# Launch the dashboard
./build/ui/app/factory-pulse
```

Optional env overrides:

| Variable | Default |
|----------|---------|
| `FORGESIGHT_DB` | `dbname=forgesight` |
| `FORGESIGHT_API` | `http://127.0.0.1:8080` |
| `FORGESIGHT_WS` | `ws://127.0.0.1:8081` |

Stop services:

```bash
./scripts/dev-up.sh stop
```

## Architecture (MVP)

```
Simulators → MQTT → Ingestion → PostgreSQL
                      ↓
              Alarm Engine (rules)
                      ↓
         API (REST + WebSocket) → Qt Dashboard
```

## Workflow

See `AGENTS.md` / `implementation-plan.md` (local): issue → `feature/*` branch → PR into `dev`. Never commit to `main`/`dev` directly.
