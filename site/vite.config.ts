import tailwindcss from "@tailwindcss/vite";
import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";

// GitHub Pages serves the site under /<repo>/; the workflow passes that in.
export default defineConfig({
  base: process.env.PAGES_BASE_PATH ? `${process.env.PAGES_BASE_PATH}/` : "/",
  plugins: [react(), tailwindcss()],
  build: { outDir: "dist", chunkSizeWarningLimit: 1000 },
});
