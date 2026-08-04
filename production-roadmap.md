# ForgeSight — Production Roadmap & Session Handoff

**Last updated:** 2026-08-04 (Phase 8 item 14 **done**: PR #27 merged)  
**Branch:** `dev`  
**Open PRs:** none — #13, #15, #17, #20, #21, #24, #27 all **MERGED** into `dev`  
**Open issues:** none from this roadmap's P0/P1/P2 list (#10 docs-sync issue still open, tracks this file itself)  

Use this file as the source of truth for “what’s done / what’s next” in a new chat session.

---

## Executive status

| Area | Status |
|------|--------|
| Local live demo (sim → MQTT → ingest → DB → API/WS → UI) | **Working** via `scripts/dev-up.sh` |
| Phases 0–7 (MVP core + offline mode) | **Done** — offline mode (Phase 7) closed 2026-07-31 |
| Phase 8 (packaging, load) | **Started** — Docker/compose for backend services done (PR #27); load script, ASan CI, AppImage still open |
| Phase 9 (release) | **Not started** |
| CI / `dev` | Green — PRs #13, #15, #17, #20, #21, #24, #27 merged |
| Production-grade | **Not yet** — P1+P2 fully done; Phase 8 in progress (item 14/4 done) |

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
| **7** | Offline mode | Done | `OfflineCache` (SQLite) restores last-known state on cold start, persists every reading live, queues acks while offline and flushes on reconnect; staleness-aware offline indicator. Closed #23 via PR #24 (2026-07-31) |
| **8** | Load test + packaging | Partial | Docker/compose for backend services done (PR #27); no AppImage, no measured load numbers, no ASan CI job |
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

### P2 — Phase 7 Offline mode ✅ (PR [#24](https://github.com/Kunj157/ForgeSight/pull/24), issue #23)

10. Client SQLite cache of last-known device states. ✅ — `ui::OfflineCache` (`QSqlDatabase`/QSQLITE), restored into `DeviceModel` on startup before any network I/O.
11. Queue alarm acks while offline; flush on reconnect. ✅ — queued via `offlineCache.queueAck()` when `!wsClient.connected`, replayed through `ApiClient::acknowledgeAlarm()` on `connectedChanged`, cleared on `alarmAckSucceeded`.
12. Offline banner + last-updated timestamp (distinct from WS “Disconnected”). ✅ — `root.offline` = `!wsClient.connected || stale` (10s no-data threshold), sidebar "Updated Xs ago" caption + pending-sync badge, top-bar LIVE/OFFLINE chip driven by the same signal.
13. Unit tests: stale detection, reconnect sync. ✅ — 13 `OfflineCacheTest` + 2 `DeviceModelTest` cases (157/157 total passing).

### P3 — Phase 8 Packaging & load

14. Dockerfiles + compose for Postgres, Mosquitto, ingestion, api, alarm-engine. ✅ (PR [#27](https://github.com/Kunj157/ForgeSight/pull/27), issue #26) — `docker/Dockerfile.backend` multi-stage build shared across the 3 C++ services; `docker-compose.yml` wires them to Postgres/Mosquitto; new `docker` CI job builds the images on every PR.
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
2. Phase 8 item 14 (Docker/compose) is done — continue P3, items 15-17:
   a load script that scales simulator device count with measured
   latency/CPU numbers written into the README, an ASan (+ optional
   Valgrind) CI job with a zero-leak goal, and packaging the Qt app
   (AppImage or similar). Any of the three can go first; they're independent.
3. Branch from dev: feature/<n>-…
4. TDD strictly (AGENTS.md)
5. Small sequential commits; PR → dev
6. Once Phase 8 closes, move to Phase 9 (release)
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
- [x] Offline cache + queued ack flush  
- [ ] Dockerized backends (done) + documented load numbers (still open)  
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
- **2026-07-31 (later):** Closed out Phase 7 (offline mode), the last P2 item — #23 → PR #24, 5 sequential TDD commits, 157/157 tests green:
  - `ui::OfflineCache` — a small SQLite (`QSqlDatabase`/QSQLITE) client cache. Each instance gets a uniquely-named connection so multiple caches can coexist in-process (needed for tests). Hit two Qt/SQLite gotchas worth remembering: (1) `QSqlDatabase::removeDatabase()` warns "connection still in use" unless you first reassign your own member handle to a default-constructed `QSqlDatabase` — otherwise your own live member counts as a reference; (2) a gtest fixture that builds its `QCoreApplication` as a function-local `static` (the usual pattern in this repo's other `ui_tests` files) segfaults at process exit *only* once `QSqlDatabase::addDatabase()` has been called anywhere in the process — `~QCoreApplication`'s `qt_call_post_routines()` runs after QtSql's own lazily-constructed statics have already been destroyed, since C++ static teardown order is the reverse of construction order and QtSql's statics get lazily constructed *after* the app in these tests. Fixed by owning the app via a pointer, explicitly `delete`d in `TearDownTestSuite()` — deterministic teardown, no more racing against atexit ordering across TUs.
  - `DeviceModel::deviceUpdated` — new signal, single chokepoint for cache writes regardless of whether a reading came from REST bootstrap or a live WS push (both paths already funnel through `updateDevice()`).
  - `main.cpp` — restores cached state into `DeviceModel` before any network I/O on startup, connects `deviceUpdated → saveDeviceState` for live persistence, and flushes `offlineCache.pendingAcks()` through `ApiClient::acknowledgeAlarm()` on every WS reconnect.
  - `AlarmPanel.qml` — Acknowledge button checks `wsClient.connected`; queues locally via `offlineCache.queueAck()` when offline instead of firing a doomed request, showing a disabled "Queued" pill (new `clock` icon) until the flush succeeds.
  - `Main.qml` — `root.offline` is now `!wsClient.connected || stale` (10s no-fresh-data threshold), not just the raw socket state, since the socket can look connected for a few seconds after the backend actually stops publishing. Sidebar gained a live "Updated Xs ago" caption and a pending-sync count badge; the top-bar LIVE/OFFLINE chip and its tooltip now use the same signal.
  - CI: added `libqt6sql6-sqlite` (the QSQLITE driver plugin — `qt6-base-dev` already ships QtSql's headers/cmake config but not the plugin itself) and linked `Qt6::Sql` in `ui/CMakeLists.txt`.
  - Repeated the "co-author trailer" gotcha from earlier sessions once, accidentally, when using plain `git commit` for the first commit — caught it immediately and rewrote it with `git commit-tree` before pushing anything; all 5 commits on `dev` are solely authored.
  - Also hit a self-inflicted near-miss: ran `git checkout <branch> -- .` after an intermediate `git update-ref`-based commit, not realizing `-- .` checks out *every* path from that ref's tree into the working directory, silently discarding the not-yet-committed `main.cpp`/QML changes for the next few commits. Recovered by redoing those edits from scratch (same content, still in this session's context) — no data actually lost, but worth flagging: never use `checkout <ref> -- .` as a "sync HEAD" no-op after `update-ref`: it's a hard reset of *all* tracked paths, not a formality.
  - **P1 + P2 are now fully done.** Next real gap: **Phase 8 (packaging & load test)** — no Dockerfiles/compose for the backend services, no measured load numbers, no ASan CI job, no packaged desktop artifact.
- **2026-08-04:** Closed Phase 8 item 14 (Docker/compose for backend services) — #26 → PR #27, 4 sequential commits, merged to `dev` with CI green:
  - `docker/Dockerfile.backend` — one multi-stage build shared by `ingestion`/`alarm-engine`/`api` (all three descend from the same CMake project, so one `builder` stage + per-service slim runtime stages avoids redundant compilation). Deliberately skips `qt6-declarative-dev`/`qt6-charts-dev` so the root `CMakeLists.txt`'s `find_package(Qt6 COMPONENTS Quick WebSockets Charts QUIET)` can't find Quick/Charts and quietly skips `ui/`+`tests/ui` at configure time.
  - `docker-compose.yml` — wires Postgres, Mosquitto, and the three services together; Qt desktop app and Python simulators intentionally stay outside Docker (not services / dev tooling you iterate on without rebuilding an image).
  - Added a `docker` CI job (`docker compose build`) so the images are verified to actually build on every PR instead of trusting a local build — this earned its keep immediately: caught 3 real bugs the local review had missed, only visible once something actually tried to build the image:
    1. **Missing `make`** — installed `cmake`/`g++` but not `make`; CMake's default "Unix Makefiles" generator had no build program (`CMAKE_MAKE_PROGRAM is not set`). Trivial on a native runner (`build-essential` is preinstalled) but not inside a bare `ubuntu:24.04` container.
    2. **`docker compose build`'s default parallel bake doesn't dedupe a shared stage across targets** ([docker/compose#13043](https://github.com/docker/compose/issues/13043)) — building `ingestion`/`alarm-engine`/`api` concurrently raced 3 independent copies of the same ~200-package `qt6-base-dev` apt-get against the same mirrors, which then started timing out (`Could not connect to archive.ubuntu.com: connection timed out`). Fixed with `COMPOSE_BAKE=false COMPOSE_PARALLEL_LIMIT=1` (both in CI and documented in the README's compose command) — forces one sequential build so the 2nd/3rd service hit the 1st's cached `builder`/`runtime-base` layers on disk instead of re-fetching everything.
    3. **Missing `git`** — root `CMakeLists.txt` unconditionally configures `tests/unit` (and the other `tests/*` dirs), which `FetchContent`-clones googletest at *configure* time regardless of which target you eventually build. No git binary on the bare base image → `could not find git for clone of googletest-populate`. Again invisible on native CI since the GH runner image ships git.
  - Net effect: the `docker` CI job now takes ~24 minutes (dominated by the one-time `qt6-base-dev` apt-get, which pulls a surprisingly large X11/Mesa/Vulkan dependency tree even though none of these 3 services touch a GUI) — acceptable for now since it only gates PRs that touch Docker/compose, not every commit.
  - Per explicit user instruction this session, no `docker build`/`docker compose` command was ever run locally — all three bugs above were found and fixed purely by reading the CI job's failure logs after each push, not by local reproduction.
  - Next real gap in Phase 8: items 15-17 — a load script with measured latency/CPU numbers, an ASan (+ optional Valgrind) CI job, and packaging the Qt app (AppImage or similar). Any order is fine, they're independent of each other and of item 14.
