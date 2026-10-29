import { motion, useReducedMotion } from "motion/react";

import { InView } from "../motion/in-view";

type N = { id: string; x: number; y: number; title: string; sub: string; accent?: boolean; optional?: boolean };

const W = 1080;
const H = 360;
const NODES: N[] = [
  { id: "sim", x: 70, y: 110, title: "Simulators", sub: "CMAPSS · SECOM" },
  { id: "mqtt", x: 240, y: 110, title: "MQTT", sub: "Mosquitto" },
  { id: "ing", x: 410, y: 110, title: "Ingestion", sub: "C++20 · thread pool" },
  { id: "pg", x: 590, y: 110, title: "PostgreSQL", sub: "readings · alarms" },
  { id: "alarm", x: 590, y: 270, title: "Alarm engine", sub: "rule evaluator" },
  { id: "api", x: 790, y: 190, title: "API", sub: "REST · WebSocket · gRPC" },
  { id: "app", x: 990, y: 190, title: "Qt 6 desktop", sub: "3D floor · offline", accent: true },
  { id: "kafka", x: 410, y: 270, title: "Kafka", sub: "optional stream", optional: true },
];
const EDGES: [string, string][] = [
  ["sim", "mqtt"],
  ["mqtt", "ing"],
  ["ing", "pg"],
  ["ing", "kafka"],
  ["pg", "alarm"],
  ["alarm", "pg"],
  ["pg", "api"],
  ["api", "app"],
];

const node = (id: string) => NODES.find((n) => n.id === id)!;

function edgePath(a: N, b: N, i: number) {
  if (a.id === "alarm" && b.id === "pg") return `M ${a.x + 22} ${a.y - 26} L ${b.x + 22} ${b.y + 26}`;
  if (a.id === "pg" && b.id === "alarm") return `M ${a.x - 22} ${a.y + 26} L ${b.x - 22} ${b.y - 26}`;
  const x1 = a.x + (b.x > a.x ? 70 : 0);
  const y1 = a.y + (b.y > a.y && Math.abs(b.x - a.x) < 10 ? 26 : 0);
  const x2 = b.x - (b.x > a.x ? 70 : 0);
  const y2 = b.y - (b.y > a.y && Math.abs(b.x - a.x) < 10 ? 26 : 0);
  if (a.id === "ing" && b.id === "kafka") return `M ${a.x} ${a.y + 26} L ${b.x} ${b.y - 26}`;
  const mx = (x1 + x2) / 2;
  return `M ${x1} ${y1} C ${mx} ${y1}, ${mx} ${y2}, ${x2} ${y2}` + (i < 0 ? "" : "");
}

export function Architecture() {
  const reduced = useReducedMotion();
  return (
    <section id="architecture" className="mx-auto max-w-6xl px-4 py-16 sm:px-6">
      <InView>
        <h2 className="text-3xl font-semibold tracking-tight sm:text-4xl">Architecture</h2>
        <p className="mt-3 max-w-2xl text-fg-2">
          Readings flow over MQTT into PostgreSQL. The alarm engine polls and raises alarms. The API serves history and rules over REST and
          pushes live readings and alarms over WebSocket, with gRPC for service clients.
        </p>
      </InView>
      <InView delay={0.1} className="mt-8">
        <div className="card overflow-x-auto rounded-2xl p-4">
          <svg viewBox={`0 0 ${W} ${H}`} className="min-w-[760px] w-full" role="img" aria-label="ForgeSight architecture diagram">
            <defs>
              <linearGradient id="edge" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2={W} y2="0">
                <stop offset="0" stopColor="#5b8cff" stopOpacity="0.15" />
                <stop offset="1" stopColor="#9a7bff" stopOpacity="0.8" />
              </linearGradient>
            </defs>
            {EDGES.map(([a, b], i) => {
              const d = edgePath(node(a), node(b), i);
              const optional = node(b).optional;
              return (
                <g key={`${a}-${b}`}>
                  <motion.path
                    d={d}
                    fill="none"
                    stroke={optional ? "#2b3341" : "url(#edge)"}
                    strokeWidth={1.6}
                    strokeDasharray={optional ? "4 6" : undefined}
                    initial={reduced ? false : { pathLength: 0 }}
                    whileInView={{ pathLength: 1 }}
                    viewport={{ once: true }}
                    transition={{ duration: 0.9, delay: 0.15 * i, ease: "easeInOut" }}
                  />
                  {!reduced && !optional && (
                    <circle r="3.2" fill={b === "app" ? "#3ecf8e" : "#9fb8ff"}>
                      <animateMotion dur={`${2.2 + (i % 3) * 0.5}s`} repeatCount="indefinite" path={d} begin={`${i * 0.3}s`} />
                    </circle>
                  )}
                </g>
              );
            })}
            {NODES.map((n, i) => (
              <motion.g
                key={n.id}
                initial={reduced ? false : { opacity: 0, y: 8 }}
                whileInView={{ opacity: 1, y: 0 }}
                viewport={{ once: true }}
                transition={{ duration: 0.5, delay: 0.08 * i }}
              >
                <rect
                  x={n.x - 70}
                  y={n.y - 26}
                  width={140}
                  height={52}
                  rx={12}
                  fill={n.accent ? "#131c33" : "#10141b"}
                  stroke={n.accent ? "#5b8cff" : n.optional ? "#2b3341" : "#232b38"}
                  strokeDasharray={n.optional ? "4 4" : undefined}
                />
                <text x={n.x} y={n.y - 3} textAnchor="middle" fill="#edf1f7" fontSize="13" fontWeight="600" fontFamily="Inter, sans-serif">
                  {n.title}
                </text>
                <text x={n.x} y={n.y + 14} textAnchor="middle" fill="#7c879a" fontSize="10" fontFamily="JetBrains Mono, monospace">
                  {n.sub}
                </text>
              </motion.g>
            ))}
          </svg>
        </div>
      </InView>
    </section>
  );
}
