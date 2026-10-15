// The site shows the same screenshots and clips as the README (docs/media).
import { cpSync, mkdirSync } from "node:fs";

mkdirSync("public/media", { recursive: true });
cpSync("../docs/media", "public/media", { recursive: true });
