import { APPIMAGE, IMAGE, RELEASES, REPO } from "../links";
import { InView } from "../motion/in-view";
import { SpotlightCard } from "../motion/spotlight-card";

const STACK = ["C++20", "Qt 6 · Qt Quick 3D", "QtCharts", "PostgreSQL", "MQTT · Mosquitto", "Kafka (opt.)", "gRPC", "SQLite offline cache", "GoogleTest", "AddressSanitizer", "Docker", "AppImage"];

export function Download() {
  return (
    <section id="download" className="mx-auto max-w-6xl px-4 py-16 sm:px-6">
      <InView>
        <h2 className="text-3xl font-semibold tracking-tight sm:text-4xl">Get ForgeSight</h2>
      </InView>
      <div className="mt-8 grid gap-4 lg:grid-cols-3">
        <InView>
          <SpotlightCard glow className="h-full p-6">
            <h3 className="font-semibold">Desktop app</h3>
            <p className="mt-2 text-sm text-fg-2">A self-contained Linux AppImage. Point it at your API with FORGESIGHT_API / FORGESIGHT_WS.</p>
            <a href={APPIMAGE} className="mt-5 inline-flex rounded-xl bg-gradient-to-r from-accent to-accent-2 px-4 py-2 text-sm font-semibold text-white hover:brightness-110">
              Download AppImage
            </a>
            <a href={RELEASES} className="mt-3 block text-xs text-fg-2 hover:text-fg">All releases →</a>
          </SpotlightCard>
        </InView>
        <InView delay={0.05}>
          <SpotlightCard className="h-full p-6">
            <h3 className="font-semibold">Backend</h3>
            <p className="mt-2 text-sm text-fg-2">One image runs ingestion, the alarm engine and the API.</p>
            <pre className="mt-4 overflow-x-auto rounded-lg border border-line bg-bg p-3 font-mono text-xs text-fg-2">
{`docker pull ${IMAGE}
docker compose up -d`}
            </pre>
          </SpotlightCard>
        </InView>
        <InView delay={0.1}>
          <SpotlightCard className="h-full p-6">
            <h3 className="font-semibold">From source</h3>
            <p className="mt-2 text-sm text-fg-2">Qt 6.4+ (Quick 3D optional), CMake 3.20+ and a C++20 compiler.</p>
            <pre className="mt-4 overflow-x-auto rounded-lg border border-line bg-bg p-3 font-mono text-xs text-fg-2">
{`git clone ${REPO}
cmake -B build && cmake --build build
./scripts/dev-up.sh`}
            </pre>
          </SpotlightCard>
        </InView>
      </div>
      <InView delay={0.1} className="mt-12">
        <div className="flex flex-wrap gap-2">
          {STACK.map((s) => (
            <span key={s} className="rounded-full border border-line bg-surface-1 px-3 py-1 font-mono text-xs text-fg-2">
              {s}
            </span>
          ))}
        </div>
      </InView>
    </section>
  );
}

export function Footer() {
  return (
    <footer className="border-t border-line">
      <div className="mx-auto flex max-w-6xl flex-col gap-3 px-4 py-10 text-sm text-fg-muted sm:flex-row sm:items-center sm:px-6">
        <span>ForgeSight · MIT licensed</span>
        <span className="sm:ml-auto flex gap-5">
          <a href={REPO} className="hover:text-fg">GitHub</a>
          <a href={`${REPO}#readme`} className="hover:text-fg">Docs</a>
          <a href={RELEASES} className="hover:text-fg">Releases</a>
        </span>
      </div>
    </footer>
  );
}
