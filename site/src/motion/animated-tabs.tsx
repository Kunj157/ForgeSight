import { motion } from "motion/react";

/** Tabs whose active pill slides between options (Motion Primitives' Animated Background). */
export function AnimatedTabs<T extends string>({
  options,
  value,
  onChange,
}: {
  options: { value: T; label: string }[];
  value: T;
  onChange: (v: T) => void;
}) {
  return (
    <div className="inline-flex rounded-xl border border-line bg-surface-1 p-1">
      {options.map((o) => (
        <button
          key={o.value}
          onClick={() => onChange(o.value)}
          className={`relative rounded-lg px-4 py-1.5 text-sm font-medium transition-colors ${
            value === o.value ? "text-fg" : "text-fg-2 hover:text-fg"
          }`}
        >
          {value === o.value && (
            <motion.span
              layoutId="tab-pill"
              className="absolute inset-0 rounded-lg border border-line-strong bg-surface-3"
              transition={{ type: "spring", stiffness: 420, damping: 34 }}
            />
          )}
          <span className="relative">{o.label}</span>
        </button>
      ))}
    </div>
  );
}
