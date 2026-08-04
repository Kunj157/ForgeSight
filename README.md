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

## Run the backend via Docker Compose

An alternative to the native setup above: Postgres, Mosquitto, ingestion, alarm-engine, and the API all run in containers, so you don't need PostgreSQL/Mosquitto/Qt installed on the host at all. Only Docker is required.

```bash
COMPOSE_BAKE=false COMPOSE_PARALLEL_LIMIT=1 docker compose up --build
```

`ingestion`, `alarm-engine`, and `api` all build from the same `docker/Dockerfile.backend`, sharing one `builder` stage so the project only gets compiled once. Compose's default parallel build (via buildx bake) doesn't dedupe that shared stage across services though — it just races all 3 through their own independent `apt-get`, tripling network load for zero benefit. The env vars above force a single sequential build so the 2nd/3rd service reuse the 1st's cached `builder` layer.

This brings up:

| Service | Purpose | Exposed on host |
|---|---|---|
| `postgres` | Database (`forgesight`) | `5432` |
| `mosquitto` | MQTT broker | `1883` |
| `ingestion` | MQTT → Postgres | — |
| `alarm-engine` | Rule evaluation (seeds default rules on first start) | — |
| `api` | REST + WebSocket | `8080`, `8081` |

The Qt desktop app and the Python simulators are **not** containerized — the desktop app isn't a service, and you'll usually want to run the simulators natively so you can iterate on `simulators/config.yaml` without rebuilding an image. Point them at the compose-exposed MQTT port (the default `localhost:1883` already matches):

```bash
pip3 install --user paho-mqtt PyYAML
PYTHONPATH=. python3 -m simulators.run -c simulators/config.yaml
```

Then launch the dashboard the same way as the native setup:

```bash
./build/ui/app/factory-pulse
```

Stop the stack (add `-v` to also drop the Postgres volume and start from an empty DB next time):

```bash
docker compose down
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
