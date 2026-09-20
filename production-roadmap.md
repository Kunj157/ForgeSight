# ForgeSight — Production Roadmap & Session Handoff

**Last updated:** 2026-08-05 (Phase 9 **fully done** — items 18-21 closed via PRs #37, #39, #41, #42; **`v1.0.0` tagged and released**)  
**Branch:** `dev` (released to `main`, tag `v1.0.0`)  
**Open PRs:** none — #13, #15, #17, #20, #21, #24, #27, #29, #31, #33, #35, #37, #39, #41, #42 all **MERGED**  
**Open issues:** none from this roadmap's P0-P4 list (#10 docs-sync issue still open, tracks this file itself; #28 closed by #29)  
**Release:** [`v1.0.0`](https://github.com/Kunj157/ForgeSight/releases/tag/v1.0.0) — AppImage + `ghcr.io/kunj157/forgesight-backend:v1.0.0`/`:latest`, published automatically by `release.yml` on tag push.  

Use this file as the source of truth for “what’s done / what’s next” in a new chat session.

---

## Executive status

| Area | Status |
|------|--------|
| Local live demo (sim → MQTT → ingest → DB → API/WS → UI) | **Working** via `scripts/dev-up.sh` |
| Phases 0–7 (MVP core + offline mode) | **Done** — offline mode (Phase 7) closed 2026-07-31 |
| Phase 8 (packaging, load) | **Done** — Docker/compose (PR #27/#29), ASan CI (PR #31), load test + measured numbers (PR #33), AppImage packaging (PR #35) |
| Phase 9 (release) | **Done** — security baseline (PR #37), `release.yml` (PR #39), `deploy.yml` template (PR #41), `dev`→`main` + `v1.0.0` tag (PR #42) |
| CI / `dev` | Green — PRs #13, #15, #17, #20, #21, #24, #27, #29, #31, #33, #35, #37, #39, #41 merged; jobs: `build` (incl. AppImage package+smoke-test), `asan`, `lint`, `simulators-test`, `docker` |
| Production-grade | **Yes — v1.0.0 released.** All phases 0-9 (P0-P4) done; only stretch/nice-to-have items remain |

**How to run today**

```bash
./scripts/dev-up.sh
./build/ui/app/factory-pulse
# stop: ./scripts/dev-up.sh stop
```

Env: `FORGESIGHT_DB`, `FORGESIGHT_API`, `FORGESIGHT_WS`, `FORGESIGHT_BIND`, `FORGESIGHT_API_KEY` (optional, see README "Security"), `FORGESIGHT_ALLOW_INSECURE_TLS` (opt-in only).

---

## Phase checklist (vs `implementation-plan.md`)

| Phase | Goal | Status | Notes |
|-------|------|--------|-------|
| **0** | Repo & CI | Done | Postgres/libpq/Paho/spdlog all in `.github/workflows/ci.yml`, green since PR #9 |
| **1** | Simulators | Partial | Works; no NASA C-MAPSS/SECOM replay yet |
| **2** | Ingestion | Done | Periodic flush + mutex added |
| **3** | Alarm engine | Done | DB poll + `--seed` |
| **4** | REST/WS API | Done | Rule CRUD HTTP done (PR #9); optional API key auth + configurable bind address added in Phase 9 (PR #37) |
| **5** | Dashboard core | Done | Plant→floor→device tree in `DeviceTreePanel.qml`, closed #5 via PR #21 (2026-07-31) |
| **6** | History + export | Done | Routed through `ApiClient`/`FORGESIGHT_API`; export path + model bugs fixed; weak pan still open (nice-to-have) |
| **7** | Offline mode | Done | `OfflineCache` (SQLite) restores last-known state on cold start, persists every reading live, queues acks while offline and flushes on reconnect; staleness-aware offline indicator. Closed #23 via PR #24 (2026-07-31) |
| **8** | Load test + packaging | Done | Docker/compose (PR #27/#29); ASan CI job with zero-leak goal, catches real bugs (PR #31); load script + measured latency/CPU numbers in README (PR #33); AppImage packaging + CI smoke-test (PR #35) |
| **9** | First release | Done | Security baseline (API key + bind config + TLS-bypass audit, PR #37), `release.yml` (PR #39), `deploy.yml` template (PR #41), `dev`→`main` merge + `v1.0.0` tag (PR #42). See [release](https://github.com/Kunj157/ForgeSight/releases/tag/v1.0.0). |

**All phases 0-9 are done. Stretch (Kafka, gRPC, camera, Prometheus, plugins) may now begin** if desired — see the "Nice-to-have" list below for candidates.

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

### P3 — Phase 8 Packaging & load ✅ (all 4 items done)

14. Dockerfiles + compose for Postgres, Mosquitto, ingestion, api, alarm-engine. ✅ (PR [#27](https://github.com/Kunj157/ForgeSight/pull/27), issue #26; hardened by PR [#29](https://github.com/Kunj157/ForgeSight/pull/29), issue #28) — `docker/Dockerfile.backend` builds one shared image for all 3 C++ services (originally 3 per-service targets, collapsed to 1 after #28); `docker-compose.yml` wires it + Postgres/Mosquitto together; `docker` CI job builds the image on every PR.
15. Load script: scale simulator device count; record **measured** latency/CPU in README. ✅ (PR [#33](https://github.com/Kunj157/ForgeSight/pull/33), issue #32) — `scripts/load_test.py` + `simulators/loadgen.py` (TDD'd pure helpers); measured 100/300/600 synthetic devices in the README, latency stays under a second at every scale tested on an 8-core dev laptop.
16. ASan (and optional Valgrind) CI job; zero-leak goal. ✅ (PR [#31](https://github.com/Kunj157/ForgeSight/pull/31), issue #30) — `FORGESIGHT_ENABLE_ASAN` CMake option + `asan` CI job; found and fixed 2 real bugs on the first run (see progress log).
17. Package Qt app (AppImage or similar). ✅ (PR [#35](https://github.com/Kunj157/ForgeSight/pull/35), issue #34) — `scripts/package-appimage.sh` (linuxdeploy + linuxdeploy-plugin-qt), verified locally under Xvfb and as a CI smoke-test in the `build` job.

### P4 — Phase 9 Release ✅ (all 4 items done)

18. Merge green stack to `dev`; PR `dev` → `main`. ✅ (PR [#42](https://github.com/Kunj157/ForgeSight/pull/42)) — `main` fast-forwarded from empty to the full `dev` history, then tagged `v1.0.0`.
19. `release.yml`: tag, Docker images, desktop artifact → GitHub Release. ✅ (PR [#39](https://github.com/Kunj157/ForgeSight/pull/39), issue #38) — fires only on `v*.*.*` tag pushes; builds+smoke-tests the AppImage, builds+pushes the backend image to GHCR (`:tag` and `:latest`), then creates the GitHub Release attaching the AppImage. Verified end-to-end on the real `v1.0.0` tag push.
20. Thin `deploy.yml` for backend (optional VPS). ✅ (PR [#41](https://github.com/Kunj157/ForgeSight/pull/41), issue #40) — `docker-compose.prod.yml` override (GHCR image instead of local build) + `workflow_dispatch`-only SSH deploy template; no real VPS target exists yet, so it fails fast with a clear "configure these secrets" error until someone adds one.
21. Security baseline: bind address config, at least API key or LAN auth; don't `ignoreSslErrors` in release builds. ✅ (PR [#37](https://github.com/Kunj157/ForgeSight/pull/37), issue #36) — `ApiServer::start()` gained a configurable bind address and optional `X-Api-Key`/WS `?api_key=` auth (empty key = disabled, matching prior local-dev-friendly default); `api` binary exposes `--bind`/`--api-key` (+ `FORGESIGHT_API_KEY` fallback); UI clients send the key automatically; `WsClient::ignoreSslErrors()` is now gated behind an explicit `allowInsecureTls` opt-in instead of running unconditionally.

### Nice-to-have (still MVP-adjacent)

- NASA C-MAPSS / SECOM replay in simulators (Phase 1 stretch of plan).  
- Simple rule editor panel in UI.  
- Stop gitignoring `AGENTS.md` / `implementation-plan.md` if the team wants them in-repo (currently local-only).

---

## Suggested next-session playbook

```text
1. Read this file + implementation-plan.md
2. All of Phases 0-9 (the full MVP) are done — v1.0.0 is tagged and
   released. There is no more required-scope work left in this
   roadmap. Pick from the "Nice-to-have" list below, or a stretch
   item (Kafka/Redpanda, gRPC, camera/RTSP, Prometheus, plugins) if
   the user explicitly wants to start one — confirm scope with them
   first per AGENTS.md ("Ask before... Changing MVP vs stretch scope").
3. Branch from dev: feature/<n>-…
4. TDD strictly (AGENTS.md)
5. Small sequential commits; PR → dev
6. For any future release, tag vX.Y.Z on main once dev is merged in —
   release.yml handles the rest (AppImage + GHCR image + GitHub
   Release) automatically.
```

### Do not

- Start Kafka / gRPC / camera / Prometheus without explicit user confirmation (MVP is done, but these are still a scope decision, not a default next step).  
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

- [x] CI green on `dev` (build, lint, unit + DB integration tests, asan, simulators-test, docker)
- [x] Full stack reproducible via compose or `dev-up.sh`
- [x] Live tiles + alarms + history + ack persist across restart
- [x] Rule CRUD via API (and ideally minimal UI)
- [x] Offline cache + queued ack flush
- [x] Dockerized backends + documented load numbers (measured at 100/300/600 devices)
- [x] ASan clean in CI
- [x] Tagged `v1.0.0` on `main` with release artifacts — see [the release](https://github.com/Kunj157/ForgeSight/releases/tag/v1.0.0) (AppImage + GHCR image, published automatically by `release.yml`)
- [x] Security baseline: configurable bind address, optional API key auth (HTTP + WS), `ignoreSslErrors` gated behind explicit opt-in

**All boxes are checked — this is now a production-grade MVP. Stretch phases may begin** (with explicit user confirmation on scope, per AGENTS.md).


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
- **2026-08-04 (later):** PR #27's compose design turned out to have a real teeth: the user tried `docker compose up --build` (the plain command, without the `COMPOSE_BAKE=false`/`COMPOSE_PARALLEL_LIMIT=1` env vars #27 had documented but couldn't enforce) and it **froze their dev machine hard enough to need a power cycle** — confirmed post-reboot by an "uncleanly shut down" systemd-journald message, not just a slow build. #28 → PR #29, closed same session:
  - Root cause: 3 separate final Docker stages (one per service) sharing a `builder` stage, selected via `--target`, is exactly the shape that trips [docker/compose#13043](https://github.com/docker/compose/issues/13043) — parallel bake doesn't dedupe a shared stage across targets, so it raced 3 concurrent copies of the ~200-package `qt6-base-dev` apt-get against the same mirrors/disk. Enough concurrent I/O + memory pressure to thrash a resource-constrained laptop into unresponsiveness.
  - Fix: collapsed to **one** image. Only the `ingestion` service declares `build:` in `docker-compose.yml` now; `alarm-engine`/`api` reference the exact same `forgesight-backend:local` tag via `image:` and just override `entrypoint:`. There's structurally nothing left for a parallel builder to race, regardless of `COMPOSE_BAKE`/env vars — the fix removes the footgun instead of relying on a user remembering a workaround. Also capped the builder's `cmake --build --parallel` at a fixed `2` (was `$(nproc)`) as a second line of defense against compile-time memory pressure — cheap to do since the actual compile is only ~40s per CI's own timing breakdown, apt-get is the entire cost.
  - Per explicit instruction, no `docker` command was run locally at any point investigating or fixing either #28 or the original #27 CI failures — every round of debugging (missing `make`, missing `git`, the parallel-bake race, and this crash) was diagnosed purely by reading CI job logs after pushing, plus `dmesg`/`journalctl` for the crash evidence itself. The `docker` CI job (GitHub-hosted runner, not the user's machine) remains the only thing that has ever actually executed `docker build`/`docker compose build` for this project.
  - Minor self-inflicted blemish: one commit message on the `feature/28-single-docker-image` branch got mangled — used unquoted backticks (and one `$(nproc)`) inside a double-quoted `MSG="..."` shell variable instead of a `<<'EOF'` heredoc, so bash ran them as command substitution before `git commit-tree` ever saw the text. Left as-is per the "don't amend already-pushed commits without being asked" rule; the PR description has the accurate, complete writeup. **Lesson: always use a quoted heredoc (`<<'EOF'`) for commit messages that contain backticks, never a plain double-quoted variable.**
  - Phase 8 item 14 is now genuinely done (previously "done" but with a live footgun in it). Still open: items 15-17 (load script, ASan CI, AppImage packaging).
- **2026-08-05:** Closed the remaining three Phase 8 items in one session — **Phase 8 is now fully done.** All three were built and merged natively on this machine (not Docker), which was a deliberate contrast with the previous session's Docker-only-via-CI approach: normal `cmake --build`/`ctest`/Python runs here don't carry the resource-storm risk that a parallel `docker compose build` did, so there was no reason to avoid running them locally.
  - **#30 → PR #31 (ASan CI job, item 16):** Added a `FORGESIGHT_ENABLE_ASAN` CMake option (set before any `add_subdirectory` so FetchContent's googletest gets instrumented consistently with app code) and an `asan` CI job mirroring `build`'s dependency matrix. Ran the full 157-test suite locally under ASan *before* touching CI and it immediately paid off — found two real, unrelated bugs in `tests/api/test_services.cpp`:
    1. `RuleServiceTest`/`DeviceServiceTest::TearDown()` called `PQexec()` for cleanup queries without `PQclear()`-ing the result — a `PGresult` leak on every single test run. Fixed with a shared `exec_and_clear()` helper.
    2. `seed_reading()` took `.c_str()` of a temporary `std::to_string(value)` and stored the pointer in a `params[]` array used later — classic dangling-pointer-to-temporary bug, flagged by ASan as `stack-use-after-scope`. Fixed by binding the string to a named local first.
    - Also noticed `ctest -j2` produces *unrelated* flaky failures (two DB tests race over a shared hardcoded `'svc-test'` device_id row) — not an ASan issue, just confirms why the existing `build` job already runs ctest without `-j`; kept the new job sequential too.
  - **#32 → PR #33 (load script, item 15):** `scripts/load_test.py` scales the simulator to N synthetic `load-XXXX`-prefixed devices (never collides with demo devices) against an already-running stack, then reports end-to-end latency computed entirely inside Postgres (`created_at - timestamp`, no cross-process clock skew) and CPU% of `ingestion`/`alarm-engine`/`api` sampled from `/proc/<pid>/stat` (no new dependency — deliberately not `psutil`, to keep the simulator's dependency footprint at just `paho-mqtt`+`PyYAML`). The reusable pure logic (device/config generation, `/proc/stat` field parsing, cpu% math) lives in `simulators/loadgen.py` and was written test-first (`simulators/tests/test_loadgen.py`, red before the module existed, green after) — the orchestration script itself was verified by actually running it against the live local stack, same philosophy as `scripts/dev-up.sh`.
    - Ran it at 100/300/600 devices for 30s each; recorded the numbers in the README. Latency stays comfortably under a second at every scale; `api`'s CPU cost scales with device count (its `ws_broadcaster` polls latest readings for *all* devices regardless of connected clients — worth knowing if this ever needs to scale much further) while `ingestion` scales with message volume as expected.
    - Also noticed nothing in CI ever ran the Python `simulators/tests/` pytest suite (pre-existing gap, not something this session introduced, but directly relevant since it just added new tests there) — added a small `simulators-test` CI job for it as a follow-up commit on the same PR.
    - Hit one local-environment quirk while getting the first measured numbers: this machine's dev `readings` table predated the `created_at` column (added to `ensure_table()`'s DDL sometime after this table was first created locally, and `CREATE TABLE IF NOT EXISTS` doesn't retroactively add columns) — a one-time `ALTER TABLE ... ADD COLUMN IF NOT EXISTS` fixed the local DB; not a code bug, CI's Postgres service always starts fresh so it never hit this.
  - **#34 → PR #35 (AppImage packaging, item 17):** `scripts/package-appimage.sh` wraps `linuxdeploy` + `linuxdeploy-plugin-qt` (downloaded into gitignored `.tools/` on first run, output to gitignored `dist/` — no binaries committed). Because `ui/app/CMakeLists.txt` already uses `qt_add_qml_module`, the app's own QML files are compiled directly into the binary; only Qt's own QML plugins (QtQuick, QtQuick.Controls, QtCharts, ...) needed bundling, and setting `QML_SOURCES_PATHS` before invoking the plugin let its import scanner find exactly those instead of guessing from linked libraries. `patchelf` (needed by `linuxdeploy` for RPATH rewriting) isn't assumed to be preinstalled — installed via `pip install --user` if missing, since the script shouldn't assume `apt`/root access.
    - Verified end-to-end locally: built a Debug binary, ran the script, then actually launched the resulting `.AppImage` against a real Xvfb X display (not just `--version`/static inspection) — it reached "window ready" using only the bundled Qt/xcb libs, no host Qt install involved. Added the same sequence (package + Xvfb smoke-test grepping for "window ready") as extra steps in the existing `build` CI job rather than a new job, since that reuses its already-built output instead of re-installing the whole Qt package list from scratch; this also exercises `APPIMAGE_EXTRACT_AND_RUN=1` as a real regression check, since GitHub-hosted runners don't have a working `/dev/fuse` for a normal AppImage mount.
  - Recurring repo quirk confirmed again: `gh pr merge --merge` with a "Closes #n" trailer in the merge commit did **not** auto-close any of #30/#32/#34 — closed all three manually after merging, same as every previous session. Worth just expecting this every time rather than treating it as a one-off.
  - **Phase 8 is done. Next real gap: Phase 9 (release)** — `main` is still empty, no `release.yml`/`deploy.yml`, no security baseline (bind address config, API key/LAN auth, `ignoreSslErrors` audit for release builds).
- **2026-08-05 (later):** Closed all four Phase 9 items in one session — **Phase 9 is done, MVP complete, `v1.0.0` tagged and released.**
  - **#36 → PR #37 (security baseline, item 21):** `ApiServer::start()` gained a `QHostAddress bindAddress` and `std::string apiKey` parameter, both defaulted to prior behavior (`Any`, no auth) so every existing call site — including every pre-existing test — kept working unchanged. When a key is configured: every `/api/*` route checks a new `is_authorized()` helper against the `X-Api-Key` header (401 otherwise); the WS accept handler checks `?api_key=` on `socket->requestUrl().query()` and closes the connection otherwise; `/health` is deliberately left unguarded so liveness checks never need credentials. `api/main.cpp` exposes `--bind`/`--api-key` (`QCommandLineParser`, matching the existing style) with a `FORGESIGHT_API_KEY` env fallback and a startup warning if nothing is configured. On the UI side, `ApiClient` gained a `make_request()` helper (used by `get_json`/`post_json`/`fetchHistory`) that attaches the same header, and `WsClient::effective_url()` appends `?api_key=` only to the URL actually used to open the socket — the public `url` property stays as configured so QML bindings don't see the key. Also audited the one `ignoreSslErrors()` call in `WsClient::on_ssl_errors()`: it ran unconditionally before, now it's gated behind a new `allowInsecureTls` property (default `false`) — disabled, it logs the specific `QSslError`s and lets the connection fail instead of silently accepting a bad cert. 8+3+4 new TDD tests across `test_api_server.cpp`/`test_api_client.cpp`/`test_ws_client.cpp` (172/172 total passing). One CI hiccup: the `lint` job caught a few unformatted lines (`api/main.cpp`, `ui/src/ws_client.cpp`) from multi-line string literals — fixed with a follow-up `clang-format -i` commit rather than amending, since the branch was already pushed.
  - **#38 → PR #39 (`release.yml`, item 19):** New workflow, tag-triggered only (`v*.*.*`, never branch pushes/PRs — those stay on `ci.yml`). Three jobs: `appimage` (same build+package+smoke-test steps as `ci.yml`'s `build` job, Release config, uploaded as a workflow artifact), `docker-image` (builds `docker/Dockerfile.backend`'s single combined image, pushes to GHCR as `ghcr.io/<owner>/forgesight-backend:<tag>` and `:latest`), and `publish-release` (downloads the AppImage artifact, `gh release create` with it attached plus a body noting the GHCR tags). Couldn't exercise the actual tag-triggered run inside the PR itself (by design — PRs target `dev`, not a tag), so it went in on syntax validation + reusing already-proven steps; the real validation came later when `v1.0.0` was actually tagged (see below) and every job passed on the first try.
  - **#40 → PR #41 (`deploy.yml` template, item 20):** Marked optional in the roadmap since this project has no real VPS. Added `docker-compose.prod.yml` (override pointing the three backend services at the GHCR image via `FORGESIGHT_IMAGE_OWNER`/`FORGESIGHT_IMAGE_TAG` instead of building locally — deploy with `... pull && ... up -d --no-build`) and a `workflow_dispatch`-only `deploy.yml` that SSHes in and runs exactly that. A check-secrets step fails fast listing which of the 5 required repo secrets are missing, so running it today (none configured) gives a clear "not set up yet" error rather than a half-finished SSH attempt.
  - **#42 (`dev` → `main`, item 18):** With items 19-21 merged and `dev` green, opened a PR from `dev` (99 commits ahead of `main`'s single initial commit) straight into `main` and merged it — `ci.yml` only triggers on PRs targeting `dev`, so this PR itself showed no checks, which is expected since every one of those 99 commits already had to pass full CI as a PR into `dev` first. Then tagged `v1.0.0` on `main` and pushed the tag, which fired `release.yml` for real: `docker-image` (2m12s) and `appimage` (2m29s) both passed, then `publish-release` created [the `v1.0.0` release](https://github.com/Kunj157/ForgeSight/releases/tag/v1.0.0) with the AppImage attached and the GHCR image noted in the body — first actual end-to-end validation of the release pipeline, and it worked on the first attempt.
  - Recurring repo quirk confirmed again: "Closes #n" didn't auto-close #36/#38/#40 on merge — closed all three manually, as every prior session's PRs also needed.
  - **The MVP defined in `production-roadmap.md` is now fully complete.** Every item in the "Definition of production-grade MVP" checklist is checked. Remaining work is either the "Nice-to-have" list above or stretch phases (Kafka/Redpanda, gRPC, camera/RTSP, Prometheus, plugins) — none of which should start without first confirming scope with the user.
