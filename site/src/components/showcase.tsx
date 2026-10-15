import { AnimatePresence, motion } from "motion/react";
import { useState } from "react";

import { media } from "../links";
import { AnimatedTabs } from "../motion/animated-tabs";
import { BorderTrail } from "../motion/border-trail";
import { InView } from "../motion/in-view";

const SHOTS = {
  floor: { file: "floor-3d.webp", caption: "3D plant floor: instanced pillars track live values; critical devices pulse red. Click to select, synced with the device list." },
  devices: { file: "devices.webp", caption: "Live device cards with sparklines, trend chips and KPI counters. Stays smooth at 600 sensors." },
  alarms: { file: "alarms.webp", caption: "Alarm triage: severity filters, pulsing live criticals, one-click acknowledge. Acks queue offline and sync on reconnect." },
  history: { file: "history.webp", caption: "History with pan/zoom, a snapping crosshair, anomaly markers and CSV/PDF export." },
  rules: { file: "rules.webp", caption: "Rule editor with a live sentence preview. Changes take effect on the alarm engine's next poll." },
} as const;
type Key = keyof typeof SHOTS;

export function Showcase() {
  const [tab, setTab] = useState<Key>("floor");
  return (
    <section id="product" className="mx-auto max-w-6xl px-4 py-16 sm:px-6">
      <InView className="flex flex-col items-center gap-6 text-center">
        <h2 className="text-3xl font-semibold tracking-tight sm:text-4xl">A desktop app built for the control room</h2>
        <p className="max-w-2xl text-fg-2">
          Native Qt Quick, with no browser in the loop. A dark, ISA-101-style theme (colour reserved for abnormal states), a light mode, and motion that
          tells you what changed.
        </p>
        <div className="max-w-full overflow-x-auto">
          <AnimatedTabs
            value={tab}
            onChange={setTab}
            options={[
              { value: "floor", label: "3D floor" },
              { value: "devices", label: "Devices" },
              { value: "alarms", label: "Alarms" },
              { value: "history", label: "History" },
              { value: "rules", label: "Rules" },
            ]}
          />
        </div>
      </InView>
      <InView delay={0.1} className="mt-8">
        <div className="card relative overflow-hidden rounded-2xl p-1.5">
          <BorderTrail color="var(--accent)" seconds={8} size={160} />
          <AnimatePresence mode="wait">
            <motion.img
              key={tab}
              src={media(SHOTS[tab].file)}
              alt={SHOTS[tab].caption}
              initial={{ opacity: 0, scale: 0.985, filter: "blur(6px)" }}
              animate={{ opacity: 1, scale: 1, filter: "blur(0px)" }}
              exit={{ opacity: 0, scale: 0.985, filter: "blur(6px)" }}
              transition={{ duration: 0.35, ease: [0.16, 1, 0.3, 1] }}
              className="block w-full rounded-xl"
              loading="lazy"
            />
          </AnimatePresence>
        </div>
        <p className="mt-4 text-center text-sm text-fg-2">{SHOTS[tab].caption}</p>
      </InView>
    </section>
  );
}
