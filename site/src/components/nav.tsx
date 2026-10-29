import { motion, useScroll, useTransform } from "motion/react";

import { REPO } from "../links";

export function Nav() {
  const { scrollY } = useScroll();
  const bg = useTransform(scrollY, [0, 80], ["rgba(6,8,12,0)", "rgba(6,8,12,0.78)"]);
  const border = useTransform(scrollY, [0, 80], ["rgba(27,33,43,0)", "rgba(27,33,43,1)"]);
  return (
    <motion.header style={{ backgroundColor: bg, borderColor: border }} className="fixed inset-x-0 top-0 z-50 border-b backdrop-blur-md">
      <nav className="mx-auto flex h-14 max-w-6xl items-center gap-6 px-4 sm:px-6">
        <a href="#top" className="flex items-center gap-2 font-semibold">
          <span className="grid h-7 w-7 place-items-center rounded-lg bg-gradient-to-b from-accent to-accent-2 shadow-[0_0_20px_-4px_rgba(154,123,255,0.8)]">
            <svg viewBox="0 0 24 24" className="h-4 w-4" fill="none" stroke="white" strokeWidth="2" strokeLinejoin="round">
              <path d="M12 3l9 5-9 5-9-5z M3 14l9 5 9-5" />
            </svg>
          </span>
          ForgeSight
        </a>
        <div className="ml-auto hidden items-center gap-6 text-sm text-fg-2 md:flex">
          <a href="#product" className="hover:text-fg">Product</a>
          <a href="#features" className="hover:text-fg">Features</a>
          <a href="#architecture" className="hover:text-fg">Architecture</a>
          <a href="#performance" className="hover:text-fg">Performance</a>
          <a href="#download" className="hover:text-fg">Download</a>
        </div>
        <a href={REPO} className="ml-auto rounded-lg border border-line-strong px-3 py-1.5 text-sm text-fg hover:bg-surface-2 md:ml-0">
          GitHub
        </a>
      </nav>
    </motion.header>
  );
}
