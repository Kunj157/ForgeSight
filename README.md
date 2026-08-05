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

## Package the desktop app (AppImage)

`scripts/package-appimage.sh` bundles the built `factory-pulse` binary, its Qt libraries, and the QML modules it imports (QtQuick, QtQuick.Controls, QtCharts, ...) into a single portable `.AppImage` — no Qt install needed on the target machine. It downloads [`linuxdeploy`](https://github.com/linuxdeploy/linuxdeploy) and its Qt plugin into `.tools/` on first run (not committed; gitignored, like `/dist/` where the output goes).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./scripts/package-appimage.sh
./dist/ForgeSight-x86_64.AppImage
```

The app's own QML files are compiled directly into the binary via `qt_add_qml_module` (see `ui/app/CMakeLists.txt`), so only Qt's own QML plugins need bundling — `QML_SOURCES_PATHS` is set before invoking `linuxdeploy-plugin-qt` so its import scanner can find exactly the ones this app actually uses. If `patchelf` (used by `linuxdeploy` to rewrite RPATHs) isn't already installed, the script installs it via `pip install --user` rather than assuming `apt`/root access.

## Run the backend via Docker Compose

An alternative to the native setup above: Postgres, Mosquitto, ingestion, alarm-engine, and the API all run in containers, so you don't need PostgreSQL/Mosquitto/Qt installed on the host at all. Only Docker is required.

```bash
docker compose up --build
```

`ingestion`, `alarm-engine`, and `api` all run the *same* built image (`forgesight-backend:local`, from `docker/Dockerfile.backend`) with a different `entrypoint:` per service — only the `ingestion` service declares `build:`, so this is a single image build no matter how compose is invoked. (An earlier version of this file gave each service its own build target sharing a `builder` stage; `docker compose build`'s default parallel mode doesn't dedupe a shared stage across targets, so it raced 3 copies of the heaviest step — installing the Qt toolchain — and was enough concurrent disk/memory pressure to freeze a real dev machine. See #28.)

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

## Load testing

`scripts/load_test.py` scales the device simulator up to N synthetic devices against an already-running backend stack (`scripts/dev-up.sh` or Docker Compose) and reports end-to-end ingestion latency (reading generation → `readings` row landing in Postgres, measured entirely via Postgres's own clock to avoid cross-process clock skew) plus CPU usage of `ingestion`/`alarm-engine`/`api` during the run. Synthetic devices use a `load-XXXX` id prefix so a run never collides with the demo devices, and their rows are deleted from the database when the run finishes (`--keep-data` to skip that).

```bash
./scripts/dev-up.sh
PYTHONPATH=. python3 scripts/load_test.py --devices 100 --duration 30
```

Measured on a dev laptop (8-core, 15 GB RAM; native build, not Docker) at 1 sensor reading/device/sec:

| Devices | Msg/s | ingestion CPU (avg/peak) | api CPU (avg/peak) | latency p50 | latency p95 | latency max |
|---:|---:|---|---|---:|---:|---:|
| 100 | ~300 | 5.5% / 12.0% | 8.9% / 22.0% | 42 ms | 560 ms | 575 ms |
| 300 | ~900 | 10.8% / 20.0% | 17.6% / 33.0% | 82 ms | 183 ms | 625 ms |
| 600 | ~1800 | 17.2% / 22.0% | 28.8% / 46.0% | 127 ms | 239 ms | 704 ms |

CPU% is percentage of one core. `api`'s cost scales with device count because `ws_broadcaster` polls latest readings for all devices to push over the WebSocket regardless of how many UI clients are connected; `ingestion` scales with message volume as expected. Latency stays well under a second at every scale tested here — comfortably fast enough for a live dashboard — with headroom to spare on modest hardware.

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
