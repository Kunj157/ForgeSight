import { useRef, useState, type ReactNode } from "react";

/** Card with a cursor-following radial light and optional glow (Spotlight + Glow Effect). */
export function SpotlightCard({
  children,
  className = "",
  tone = "rgba(91,140,255,0.16)",
  glow = false,
}: {
  children: ReactNode;
  className?: string;
  tone?: string;
  glow?: boolean;
}) {
  const ref = useRef<HTMLDivElement>(null);
  const [pos, setPos] = useState<{ x: number; y: number } | null>(null);
  return (
    <div
      ref={ref}
      onPointerMove={(e) => {
        const r = ref.current!.getBoundingClientRect();
        setPos({ x: e.clientX - r.left, y: e.clientY - r.top });
      }}
      onPointerLeave={() => setPos(null)}
      className={`card relative overflow-hidden rounded-2xl transition-transform duration-300 hover:-translate-y-0.5 ${className}`}
      style={glow ? { boxShadow: "0 0 0 1px rgba(91,140,255,0.35), 0 0 60px -20px rgba(91,140,255,0.6)" } : undefined}
    >
      <div
        aria-hidden
        className="pointer-events-none absolute inset-0 transition-opacity duration-300"
        style={{
          opacity: pos ? 1 : 0,
          background: pos ? `radial-gradient(320px circle at ${pos.x}px ${pos.y}px, ${tone}, transparent 65%)` : undefined,
        }}
      />
      <div className="relative">{children}</div>
    </div>
  );
}
