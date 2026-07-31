# ForgeSight — Production Roadmap & Session Handoff

**Last updated:** 2026-07-31 (P1 **done**: PRs #13, #15, #17, #20, #21 merged)  
**Branch:** `dev`  
**Open PRs:** none — #13, #15, #17, #20, #21 all **MERGED** into `dev`  
**Open issues:** none from this roadmap's P0/P1 list (#10 docs-sync issue still open, tracks this file itself)  

Use this file as the source of truth for “what’s done / what’s next” in a new chat session.

---

## Executive status

| Area | Status |
|------|--------|
| Local live demo (sim → MQTT → ingest → DB → API/WS → UI) | **Working** via `scripts/dev-up.sh` |
| Phases 0–6 (MVP core) | **Done** — device hierarchy (Phase 5) closed 2026-07-31 |
| Phases 7–9 (offline, packaging, release) | **Not started** |
| CI / `dev` | Green — PRs #13, #15, #17, #20, #21 merged |
| Production-grade | **Not yet** — P1 fully done; next is Phase 7 (offline mode) |

**How to run today**

```bash
./scripts/dev-up.sh
./build/ui/app/factory-pulse
# stop: ./scripts/dev-up.sh stop
```

Env: `FORGESIGHT_DB`, `FORGESIGHT_API`, `FORGESIGHT_WS`.

---

## Phase checklist (vs `implementation-plan.md`)

| Phase | Goal | Status | Notes |
|-------|------|--------|-------|
| **0** | Repo & CI | Done | Postgres/libpq/Paho/spdlog all in `.github/workflows/ci.yml`, green since PR #9 |
| **1** | Simulators | Partial | Works; no NASA C-MAPSS/SECOM replay yet |
| **2** | Ingestion | Done | Periodic flush + mutex added |
| **3** | Alarm engine | Done | DB poll + `--seed` |
| **4** | REST/WS API | Partial | Rule CRUD HTTP done (PR #9); no auth/TLS |
| **5** | Dashboard core | Done | Plant→floor→device tree in `DeviceTreePanel.qml`, closed #5 via PR #21 (2026-07-31) |
| **6** | History + export | Done | Routed through `ApiClient`/`FORGESIGHT_API`; export path + model bugs fixed; weak pan still open (nice-to-have) |
| **7** | Offline mode | **Missing** | No SQLite cache / queued acks |
| **8** | Load test + packaging | **Missing** | No Docker for services, no AppImage, no measured load numbers |
| **9** | First release | **Missing** | `main` empty; no `release.yml` / `deploy.yml` |

Stretch (Kafka, gRPC, camera, Prometheus, plugins): **do not start** until Phases 7–9 close.

---

## What recently landed (PR #9)

1. Ingestion flush every 500ms + writer mutex  
2. Alarm-engine DB poll + default rule seed  
3. API: `/api/readings/latest`, `/api/alarms`, alarm WS broadcast, ack POST  
4. WsClient auto-reconnect with backoff + tests  
5. ApiClient REST bootstrap + tests  
6. Industrial UI (`Theme.js`, sidebar, table alarms, lazy History)  
7. `scripts/dev-up.sh` + README runbook  
8. Simulator broker port fixed to `1883`

---

## Gaps to close for production-grade MVP

Ordered by priority. Each item should be: **GitHub issue → `feature/<n>-…` from `dev` → TDD → PR → `dev`**.

### P0 — Unblock merge / reliability (do next session first)

1. **Fix CI on PR #9** ✅  
   - Install `libpq-dev`, Paho, spdlog (whatever CMake needs).  
   - Add Postgres service for DB tests (`forgesight_test`).  
   - Fix lint `find` paths / formatting.  
   - Evidence: build+lint currently **FAILURE** on PR #9.

2. **Alarm ack end-to-end correctness** ✅ (ApiClient + dedupe)  
   - Move ack into `ApiClient` (honor `FORGESIGHT_API`, not hardcoded `127.0.0.1`).  
   - Preserve `acknowledged` on bootstrap; **dedupe by alarm id**.  
   - Integration test: POST ack → DB → reload list.

3. **Rule CRUD over HTTP** ✅ (POST/GET/PUT/DELETE + /health)  
   - Wire `POST/PUT/DELETE /api/rules` to existing `RuleService`.  
   - Contract tests in `tests/api/`.  
   - (UI rule editor can follow later.)

4. **Housekeeping**  
   - Close/supersede stale PRs [#4](https://github.com/Kunj157/ForgeSight/pull/4) (Crow) and [#6](https://github.com/Kunj157/ForgeSight/pull/6) (old dashboard).  
   - Close or update stale issues #1, #3, #5, #7 once PR #9 merges.

### P1 — Finish Phases 4–6 polish

5. **DB indexes + migrations** ✅ (PR [#13](https://github.com/Kunj157/ForgeSight/pull/13), issue #12)  
   - Index `readings(device_id, sensor, timestamp)`.  
   - Index `alarms(timestamp)`, `alarms(acknowledged)`.  
   - Created idempotently via `CREATE INDEX IF NOT EXISTS` alongside existing table DDL.

6. **Device hierarchy (Phase 5 gap)** ✅ (PR [#21](https://github.com/Kunj157/ForgeSight/pull/21), issue #5)  
   - Schema: `device_metadata(device_id, plant, floor)` table, LEFT JOINed into `DeviceService::list_devices()`, defaults to "Unassigned".
   - API: `/api/devices` returns `plant`/`floor`; `ApiServer` seeds the two known simulator devices on startup (non-destructively).
   - C++ model: `DeviceModel` gained `updateDeviceMeta()` + `plants()`/`floors()`/`devicesFor()` query helpers.
   - QML: `DeviceTreePanel.qml` replaced the flat `Flow` with collapsible plant → floor sections (nested `Repeater`s, not `TreeView` — kept it dependency-free).

7. **History client polish** ✅ (PR [#17](https://github.com/Kunj157/ForgeSight/pull/17), issue #16)  
   - Routed `HistoryPanel.qml` through `ApiClient.fetchHistory()` (honors `FORGESIGHT_API`) instead of a raw hardcoded `XMLHttpRequest`.
   - Safer export paths via `HistoryModel::default_export_path()` (Documents/ForgeSight, not cwd-relative).
   - **Bonus bug fixed**: QML was calling `historyModel.addPoint()/exportCsv()/exportPdf()`, which don't exist (`HistoryModel` exposes `add_point()/export_csv()/export_pdf()`) — history points were never added to the model and CSV/PDF export silently failed. Also made `pointCount` a real `Q_PROPERTY` (was a non-reactive plain invokable).
   - Pan/brush range still open (nice-to-have, not blocking).

8. **Health endpoints** ✅ — `GET /health` already existed and is tested (`ApiServerTest.HealthEndpointOk`); `/ready` not added (not needed yet, no separate readiness concept).

9. **Fix MQTT bridge lifetime** ✅ (PR [#15](https://github.com/Kunj157/ForgeSight/pull/15), issue #14) — `ApiServer` now owns `MqttBridge` via `std::unique_ptr`; verified with a local ASan build (leak before fix, 0 leaks after, across repeated start/stop cycles).

### P2 — Phase 7 Offline mode

10. Client SQLite cache of last-known device states.  
11. Queue alarm acks while offline; flush on reconnect.  
12. Offline banner + last-updated timestamp (distinct from WS “Disconnected”).  
13. Unit tests: stale detection, reconnect sync.

### P3 — Phase 8 Packaging & load

14. Dockerfiles + compose for Postgres, Mosquitto, ingestion, api, alarm-engine.  
15. Load script: scale simulator device count; record **measured** latency/CPU in README.  
16. ASan (and optional Valgrind) CI job; zero-leak goal.  
17. Package Qt app (AppImage or similar).

### P4 — Phase 9 Release

18. Merge green stack to `dev`; PR `dev` → `main`.  
19. `release.yml`: tag, Docker images, desktop artifact → GitHub Release.  
20. Thin `deploy.yml` for backend (optional VPS).  
21. Security baseline: bind address config, at least API key or LAN auth; don’t `ignoreSslErrors` in release builds.

### Nice-to-have (still MVP-adjacent)

- NASA C-MAPSS / SECOM replay in simulators (Phase 1 stretch of plan).  
- Simple rule editor panel in UI.  
- Stop gitignoring `AGENTS.md` / `implementation-plan.md` if the team wants them in-repo (currently local-only).

---

## Suggested next-session playbook

```text
1. Read this file + implementation-plan.md
2. P1 is fully done — start Phase 7 offline mode (P2, items 10-13):
   client SQLite cache, queued ack flush, offline banner, reconnect
   sync tests. This is the next real functional gap.
3. Branch from dev: feature/<n>-…
4. TDD strictly (AGENTS.md)
5. Small sequential commits; PR → dev
6. Once Phase 7 closes, move to Phase 8 (Docker packaging + load test)
```

### Do not

- Start Kafka / gRPC / camera / Prometheus before Phases 7–9.  
- Commit to `main` or `dev` directly.  
- Claim performance numbers without a measured load run.

---

## Key paths

| Path | Role |
|------|------|
| `scripts/dev-up.sh` | Local stack runner |
| `api/src/api_server.cpp` | REST + WS routes |
| `ui/src/ws_client.cpp` | Reconnect |
| `ui/src/api_client.cpp` | REST bootstrap |
| `ui/app/qml/` | Dashboard |
| `alarm-engine/main.cpp` | Rule poll loop |
| `ingestion/main.cpp` | MQTT → Postgres |
| `.github/workflows/ci.yml` | CI (needs hardening) |
| `implementation-plan.md` | Original phase plan (gitignored locally) |

---

## Definition of “production-grade MVP” (exit criteria)

- [ ] CI green on `dev` (build, lint, unit + DB integration tests)  
- [ ] Full stack reproducible via compose or `dev-up.sh`  
- [ ] Live tiles + alarms + history + ack persist across restart  
- [ ] Rule CRUD via API (and ideally minimal UI)  
- [ ] Offline cache + queued ack flush  
- [ ] Dockerized backends + documented load numbers  
- [ ] ASan clean in CI  
- [ ] Tagged `v1.0.0` on `main` with release artifacts  

When all boxes above are checked, stretch phases may begin.


---

## Progress log

- **2026-07-30 (morning):** P0 complete. PR #9 merged to `dev` (CI green). Closed stale PRs #4/#6 and issues #1/#3/#8. Next: P1 items from this roadmap (indexes, hierarchy, history ApiClient, MQTT bridge lifetime) then Phase 7 offline.
- **2026-07-30 (afternoon):** P1 nearly done. Closed 3 issue→branch→TDD→PR cycles, all merged to `dev` with CI green:
  - #12 → PR #13: DB indexes on `readings`/`alarms`.
  - #14 → PR #15: fixed `MqttBridge` raw-`new` leak in `ApiServer::connect_mqtt`; verified with a local ASan build (leak reproduced before, 0 leaks after).
  - #16 → PR #17: `HistoryPanel` now goes through `ApiClient`/`FORGESIGHT_API`; found and fixed a real bug along the way — QML was calling `addPoint`/`exportCsv`/`exportPdf` which don't exist on `HistoryModel` (it's `add_point`/`export_csv`/`export_pdf`), so history points were never added to the model and CSV/PDF export silently failed; also made `pointCount` a reactive `Q_PROPERTY`.
  - Housekeeping: closed stale issue #7 (superseded by PR #9), renamed/rescoped #5 to just the device-hierarchy gap with an implementation plan posted in comments, pruned merged remote branches.
  - Only P1 item remaining: #5 (device hierarchy) — intentionally left for a dedicated session given its size (schema + API + model + QML tree layers).
  - Note: all commits in this session were made via `git commit-tree` rather than plain `git commit`, to avoid the Cursor agent's automatic `Co-authored-by: Cursor` trailer being appended (that trailer is injected by the IDE/agent tooling itself, not something controllable from commit message content — see Cursor Settings → Agent → Attribution if this needs to change globally).
- **2026-07-31:** UI modernization (issue #19) merged via PR #20 — didn't get closed automatically by "Closes #19" on merge, closed manually. Then closed out the last open P1 item:
  - #5 → PR #21: device hierarchy, full vertical slice — `device_metadata` Postgres table, `DeviceService`/`ApiServer` plant+floor on `/api/devices` (with non-destructive startup seeding for the two known simulator devices), `ApiClient.fetchDevices()`, and `DeviceTreePanel.qml` rewritten from a flat card grid to collapsible plant→floor sections. 6 sequential TDD commits, 142/142 tests green, clang-format clean, smoke-tested against the live stack. Also didn't auto-close on merge — closed #5 manually too (seems `gh pr merge --merge` doesn't reliably trigger the "Closes #n" auto-link in this repo; worth checking manually after every merge until root-caused).
  - **P1 is now fully done.** Also corrected two stale rows in this file while auditing it against the real codebase: Phase 0 (CI) and Phase 4's rule-CRUD note were marked "Partial" when the underlying work (PR #9) had already actually shipped them — the roadmap just hadn't been re-read carefully against the code, only against issue/PR titles.
  - Next real gap: **Phase 7 (offline mode)** — no SQLite client cache or queued-ack-on-reconnect exists anywhere in `ui/` yet.
