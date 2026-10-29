import { AnimatedNumber } from "../motion/animated-number";
import { InView } from "../motion/in-view";
import { SpotlightCard } from "../motion/spotlight-card";

// Measured numbers from the README's load-testing section (8-core dev laptop,
// native build, 1 reading/device/sec).
const ROWS = [
  { devices: 100, msgs: 300, p50: 42, p95: 560, max: 575 },
  { devices: 300, msgs: 900, p50: 82, p95: 183, max: 625 },
  { devices: 600, msgs: 1800, p50: 127, p95: 239, max: 704 },
];

export function Performance() {
  return (
    <section id="performance" className="mx-auto max-w-6xl px-4 py-16 sm:px-6">
      <InView>
        <h2 className="text-3xl font-semibold tracking-tight sm:text-4xl">Measured, not promised</h2>
        <p className="mt-3 max-w-2xl text-fg-2">
          End-to-end ingestion latency, from reading generation to the row landing in Postgres, at 1 reading per device per second. Load-tested with{" "}
          <code className="font-mono text-xs text-fg">scripts/load_test.py</code>.
        </p>
      </InView>
      <div className="mt-10 grid gap-4 sm:grid-cols-3">
        {[
          { label: "simulated devices", value: 600, suffix: "" },
          { label: "messages per second", value: 1800, suffix: "" },
          { label: "p95 latency at 600 devices", value: 239, suffix: " ms" },
        ].map((k, i) => (
          <InView key={k.label} delay={i * 0.06}>
            <SpotlightCard className="p-6">
              <div className="font-mono text-4xl font-bold tracking-tight">
                <AnimatedNumber value={k.value} suffix={k.suffix} />
              </div>
              <div className="mt-1 text-sm text-fg-2">{k.label}</div>
            </SpotlightCard>
          </InView>
        ))}
      </div>
      <InView delay={0.1} className="mt-6">
        <div className="card overflow-x-auto rounded-2xl">
          <table className="w-full min-w-[520px] text-left text-sm">
            <thead className="text-xs uppercase tracking-wider text-fg-muted">
              <tr className="border-b border-line">
                <th className="px-5 py-3 font-semibold">Devices</th>
                <th className="px-5 py-3 font-semibold">Msg/s</th>
                <th className="px-5 py-3 font-semibold">p50</th>
                <th className="px-5 py-3 font-semibold">p95</th>
                <th className="px-5 py-3 font-semibold">max</th>
              </tr>
            </thead>
            <tbody className="font-mono">
              {ROWS.map((r) => (
                <tr key={r.devices} className="border-b border-line last:border-0">
                  <td className="px-5 py-3">{r.devices}</td>
                  <td className="px-5 py-3">~{r.msgs.toLocaleString("en-US")}</td>
                  <td className="px-5 py-3">{r.p50} ms</td>
                  <td className="px-5 py-3">{r.p95} ms</td>
                  <td className="px-5 py-3">{r.max} ms</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </InView>
    </section>
  );
}
