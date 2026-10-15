import { motion, useReducedMotion } from "motion/react";
import { lazy, Suspense, useEffect, useState } from "react";

import { APPIMAGE, media, REPO } from "../links";

// three.js loads after first paint and never blocks the headline.
const HeroScene = lazy(() => import("./hero-scene"));

function canUseWebGL() {
  try {
    const canvas = document.createElement("canvas");
    return Boolean(canvas.getContext("webgl2") ?? canvas.getContext("webgl"));
  } catch {
    return false;
  }
}

export function Hero() {
  const reduced = useReducedMotion() ?? false;
  const [webgl, setWebgl] = useState<boolean | null>(null);
  useEffect(() => setWebgl(canUseWebGL()), []);

  return (
    <section className="relative isolate overflow-hidden pt-28 pb-16 sm:pt-36">
      <div className="dot-grid absolute inset-0 -z-20" aria-hidden />
      <div
        className="absolute inset-x-0 top-0 -z-10 h-[620px] opacity-80 sm:h-[760px] lg:left-[22%]"
        style={{
          maskImage: "linear-gradient(to bottom, black 50%, transparent 95%), linear-gradient(to right, transparent 0%, black 35%)",
          maskComposite: "intersect",
          WebkitMaskComposite: "source-in",
        }}
        aria-hidden
      >
        {webgl ? (
          <Suspense fallback={null}>
            <HeroScene reduced={reduced} />
          </Suspense>
        ) : webgl === false ? (
          <img src={media("floor-3d.webp")} alt="" className="h-full w-full object-cover opacity-40" />
        ) : null}
        <div className="absolute inset-0 bg-gradient-to-b from-bg/20 via-transparent to-bg" />
      </div>

      <div className="mx-auto max-w-6xl px-4 sm:px-6">
        <motion.div
          initial={reduced ? false : { opacity: 0, y: 16 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.7, ease: [0.16, 1, 0.3, 1] }}
          className="max-w-3xl"
        >
          <a
            href={`${REPO}/releases`}
            className="inline-flex items-center gap-2 rounded-full border border-line bg-surface-1/70 px-3 py-1 text-xs text-fg-2 backdrop-blur hover:border-line-strong"
          >
            <span className="relative flex h-2 w-2">
              <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-ok opacity-60" />
              <span className="relative inline-flex h-2 w-2 rounded-full bg-ok" />
            </span>
            New: a live 3D plant floor in the desktop app
          </a>
          <h1 className="mt-6 text-4xl font-semibold tracking-tight sm:text-6xl">
            <span className="text-gradient">Every sensor on the plant floor,</span>
            <br />
            live, in one control room.
          </h1>
          <p className="mt-5 max-w-2xl text-base text-fg-2 sm:text-lg">
            ForgeSight is an open-source industrial monitoring stack: MQTT ingestion into PostgreSQL, a rule-based alarm
            engine, REST / WebSocket / gRPC APIs and a Qt&nbsp;6 desktop dashboard. Tested to 600 devices with sub-second latency.
          </p>
          <p className="text-shimmer mt-3 font-mono text-xs">mqtt → ingestion → postgres → alarm engine → ws → qt</p>
          <div className="mt-8 flex flex-wrap gap-3">
            <a
              href={APPIMAGE}
              className="group relative inline-flex items-center gap-2 rounded-xl bg-gradient-to-r from-accent to-accent-2 px-5 py-2.5 text-sm font-semibold text-white shadow-[0_0_40px_-10px_rgba(91,140,255,0.8)] transition hover:brightness-110"
            >
              Download for Linux
              <span className="font-mono text-[11px] opacity-80">AppImage</span>
            </a>
            <a
              href={REPO}
              className="inline-flex items-center gap-2 rounded-xl border border-line-strong bg-surface-1/70 px-5 py-2.5 text-sm font-semibold text-fg backdrop-blur transition hover:bg-surface-2"
            >
              View on GitHub
            </a>
          </div>
        </motion.div>
      </div>
    </section>
  );
}
