# ForgeSight — Production Roadmap & Session Handoff

**Last updated:** 2026-07-30 (P0 CI/ack/rules in progress on PR #9)  
**Branch:** `feature/historical-graphs`  
**Open PR:** https://github.com/Kunj157/ForgeSight/pull/9 → `dev` (Closes #8)  
**Issue:** https://github.com/Kunj157/ForgeSight/issues/8  

Use this file as the source of truth for “what’s done / what’s next” in a new chat session.

---

## Executive status

| Area | Status |
|------|--------|
| Local live demo (sim → MQTT → ingest → DB → API/WS → UI) | **Working** via `scripts/dev-up.sh` |
| Phases 0–6 (MVP core) | **Mostly done**, with holes listed below |
| Phases 7–9 (offline, packaging, release) | **Not started** |
| CI on PR #9 | **Green** (build + lint) |
| Production-grade | **Not yet** — reliable local demo, not shippable |

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
| **0** | Repo & CI | Partial | CI thin: missing libpq/Paho/spdlog/Postgres in workflow |
| **1** | Simulators | Partial | Works; no NASA C-MAPSS/SECOM replay yet |
| **2** | Ingestion | Done | Periodic flush + mutex added |
| **3** | Alarm engine | Done | DB poll + `--seed` |
| **4** | REST/WS API | Partial | Missing rule CRUD HTTP; no auth/TLS |
| **5** | Dashboard core | Partial | Flat cards, not plant→floor→device tree |
| **6** | History + export | Partial | Charts/export work; hardcoded API URL in QML; weak pan |
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

5. **DB indexes + migrations**  
   - Index `readings(device_id, sensor, timestamp)`.  
   - Index `alarms(timestamp)`, `alarms(acknowledged)`.  
   - Stop relying only on embedded `CREATE TABLE IF NOT EXISTS`.

6. **Device hierarchy (Phase 5 gap)**  
   - Schema/API: plant → floor → device.  
   - QML tree or grouped model (not only flat Flow cards).

7. **History client polish**  
   - Route HistoryPanel through `ApiClient` / `FORGESIGHT_API`.  
   - Pan or brush range; safer export paths (not cwd-only).

8. **Health endpoints**  
   - `GET /health` (and optionally `/ready`) on API; document in `dev-up.sh`.

9. **Fix MQTT bridge lifetime** in `api/src/api_server.cpp` (callback `new` without ownership).

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
2. gh pr checks 9   → fix CI until green
3. Merge PR #9 into dev (after green)
4. Open issues for P0 remaining items (ack E2E, rule CRUD) if not covered
5. Branch from dev: feature/<n>-…
6. TDD strictly (AGENTS.md)
7. Small sequential commits; PR → dev
8. Only after P0–P1: start Phase 7 offline
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
