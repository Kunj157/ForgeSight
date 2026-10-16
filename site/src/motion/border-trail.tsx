import { useReducedMotion } from "motion/react";

/** A short comet of light travelling around the parent's border (Motion Primitives' Border Trail). */
export function BorderTrail({ color = "var(--critical)", seconds = 5, size = 90 }: { color?: string; seconds?: number; size?: number }) {
  const reduced = useReducedMotion();
  if (reduced) return null;
  return (
    <div
      aria-hidden
      className="pointer-events-none absolute inset-0 rounded-[inherit] border border-transparent"
      style={{
        mask: "linear-gradient(transparent, transparent), linear-gradient(#000, #000)",
        maskClip: "padding-box, border-box",
        maskComposite: "intersect",
        WebkitMaskComposite: "source-in",
      }}
    >
      <div
        className="absolute aspect-square"
        style={{
          width: size,
          background: `radial-gradient(circle, ${color}, transparent 70%)`,
          offsetPath: `rect(0 auto auto 0 round ${size}px)`,
          animation: `trail ${seconds}s linear infinite`,
        }}
      />
    </div>
  );
}
