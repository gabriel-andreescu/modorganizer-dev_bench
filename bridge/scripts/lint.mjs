import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { fileURLToPath } from "node:url";

const bridge = fileURLToPath(new URL("..", import.meta.url));
const windows = process.platform === "win32";

function npm(args) {
  const result = spawnSync(windows ? "npm.cmd" : "npm", args, { cwd: bridge, stdio: "inherit", shell: windows });
  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

// ESLint's type-aware rules need the bridge's dependencies, which a fresh checkout such as a CI runner lacks.
if (!existsSync(new URL("../node_modules", import.meta.url))) {
  npm(["ci", "--no-audit", "--no-fund"]);
}
npm(["run", "lint"]);
