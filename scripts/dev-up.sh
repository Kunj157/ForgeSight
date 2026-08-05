#!/usr/bin/env bash
# Start the ForgeSight local stack: API, ingestion, alarm-engine, simulators.
# Requires: built binaries in ./build, Mosquitto on :1883, Postgres DB `forgesight`.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${FORGESIGHT_BUILD:-$ROOT/build}"
LOG_DIR="${FORGESIGHT_LOG_DIR:-/tmp/forgesight-logs}"
DB_CONN="${FORGESIGHT_DB:-dbname=forgesight}"
BIND="${FORGESIGHT_BIND:-0.0.0.0}"
API_KEY="${FORGESIGHT_API_KEY:-}"
PID_FILE="$LOG_DIR/pids"

mkdir -p "$LOG_DIR"

die() { echo "error: $*" >&2; exit 1; }

need() {
  local bin="$1"
  [[ -x "$bin" ]] || die "missing executable: $bin (build the project first)"
}

need "$BUILD/api/api"
need "$BUILD/ingestion/ingestion"
need "$BUILD/alarm-engine/alarm-engine"

stop_stack() {
  if [[ -f "$PID_FILE" ]]; then
    while read -r pid; do
      kill "$pid" 2>/dev/null || true
    done < "$PID_FILE"
    rm -f "$PID_FILE"
  fi
}

if [[ "${1:-}" == "stop" ]]; then
  stop_stack
  echo "ForgeSight stack stopped"
  exit 0
fi

stop_stack
: > "$PID_FILE"

echo "DB:      $DB_CONN"
echo "Bind:    $BIND"
echo "API key: $([[ -n "$API_KEY" ]] && echo "enabled" || echo "disabled (local dev default)")"
echo "Logs:    $LOG_DIR"
echo "Starting services…"

api_args=(--database "$DB_CONN" --port 8080 --ws-port 8081 --bind "$BIND")
[[ -n "$API_KEY" ]] && api_args+=(--api-key "$API_KEY")

"$BUILD/api/api" "${api_args[@]}" \
  >"$LOG_DIR/api.log" 2>&1 &
echo $! >> "$PID_FILE"

"$BUILD/ingestion/ingestion" --db "$DB_CONN" \
  >"$LOG_DIR/ingestion.log" 2>&1 &
echo $! >> "$PID_FILE"

"$BUILD/alarm-engine/alarm-engine" --db "$DB_CONN" --seed \
  >"$LOG_DIR/alarm-engine.log" 2>&1 &
echo $! >> "$PID_FILE"

# Simulators (optional if Python deps missing)
if [[ -f "$ROOT/simulators/run.py" ]]; then
  (
    cd "$ROOT"
    PYTHONPATH="$ROOT" python3 -m simulators.run -c simulators/config.yaml
  ) >"$LOG_DIR/simulators.log" 2>&1 &
  echo $! >> "$PID_FILE"
fi

sleep 1

echo
echo "REST:       http://127.0.0.1:8080/api/devices"
echo "WebSocket:  ws://127.0.0.1:8081"
echo "UI:         $BUILD/ui/app/factory-pulse"
echo
echo "Tail logs:  tail -f $LOG_DIR/*.log"
echo "Stop:       $0 stop"

# /health always stays open even when --api-key is set, so it's a safe,
# auth-independent liveness check.
if curl -sf "http://127.0.0.1:8080/health" >/dev/null; then
  echo "API health: OK"
else
  echo "API health: FAIL — check $LOG_DIR/api.log" >&2
  exit 1
fi
