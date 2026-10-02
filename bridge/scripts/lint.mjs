import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { fileURLToPath } from "node:url";

const bridge = fileURLToPath(new URL("..", import.meta.url));

// npm is a shell script on every platform (npm.cmd on Windows), so it runs through the shell as one command.
function npm(command) {
  const result = spawnSync(`npm ${command}`, { cwd: bridge, stdio: "inherit", shell: true });
  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

// ESLint's type-aware rules need the bridge's dependencies, which a fresh checkout such as a CI runner lacks.
if (!existsSync(new URL("../node_modules", import.meta.url))) {
  npm("ci --no-audit --no-fund");
}
npm("run lint");
