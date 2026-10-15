import { media } from "../links";
import { BorderTrail } from "../motion/border-trail";
import { InView } from "../motion/in-view";
import { SpotlightCard } from "../motion/spotlight-card";

const FEATURES = [
  { title: "Live 3D plant floor", body: "Qt Quick 3D with GPU instancing: one draw call for every device, with shadows and SSAO. Falls back to a 2.5D isometric view on software GL.", img: "floor-3d-600.webp", wide: true, critical: true },
  { title: "Offline-first", body: "A SQLite cache restores the last-known state on a cold start, queues alarm acks while disconnected and flushes them on reconnect.", img: "alarms-light.webp" },
  { title: "Rule-based alarms", body: "Threshold rules per device and sensor with info / warning / critical severities, managed from the app or the REST API.", img: "rules.webp" },
  { title: "History and export", body: "Historical trends with pan/zoom, anomaly markers and a crosshair readout. Export to CSV or PDF.", img: "history.webp" },
  { title: "Three APIs", body: "REST for queries and rule CRUD, WebSocket push for live readings and alarms, and an opt-in gRPC surface. API-key auth on all of them.", img: null },
  { title: "Ships as one binary", body: "A self-contained Linux AppImage for the dashboard, and one Docker image for ingestion, the alarm engine and the API.", img: null },
];

export function Features() {
  return (
    <section id="features" className="mx-auto max-w-6xl px-4 py-16 sm:px-6">
      <InView>
        <h2 className="text-3xl font-semibold tracking-tight sm:text-4xl">From sensor to screen, end to end</h2>
        <p className="mt-3 max-w-2xl text-fg-2">Production-grade pieces, each tested in CI (including an AddressSanitizer run).</p>
      </InView>
      <div className="mt-10 grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
        {FEATURES.map((f, i) => (
          <InView key={f.title} delay={i * 0.05} className={f.wide ? "sm:col-span-2" : ""}>
            <SpotlightCard className="h-full">
              {f.critical && <BorderTrail />}
              <div className="p-6">
                <h3 className="font-semibold">{f.title}</h3>
                <p className="mt-2 text-sm text-fg-2">{f.body}</p>
              </div>
              {f.img && (
                <div className="px-6 pb-6">
                  <img src={media(f.img)} alt="" loading="lazy" className="w-full rounded-lg border border-line" />
                </div>
              )}
            </SpotlightCard>
          </InView>
        ))}
      </div>
    </section>
  );
}
