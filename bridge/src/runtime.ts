import { readdir, readFile } from "node:fs/promises";
import { basename, isAbsolute, join, resolve } from "node:path";

export interface Target {
  label: string;
  registryDir: string;
  install?: string;
  instance?: string;
  pid?: number;
}

interface RuntimeIdentity {
  install: string;
  instance: string;
  session: string;
  pid: number;
  port: number;
  exe: string;
}

export interface Instance {
  baseUrl: string;
  pid: number;
  session: string;
}

export class Unavailable extends Error {}

function pathKey(path: string): string {
  const absolute = resolve(path);
  return process.platform === "win32" ? absolute.toLowerCase() : absolute;
}

export function resolveTarget(args: {
  install?: string;
  instance?: string;
  pid?: number;
  localAppData?: string;
}): Target {
  if (
    args.pid !== undefined &&
    (!Number.isInteger(args.pid) || args.pid <= 0)
  ) {
    throw new Error("--pid must be a positive process ID.");
  }

  const localAppData = args.localAppData ?? process.env.LOCALAPPDATA;
  if (!localAppData) {
    throw new Error(
      "LOCALAPPDATA is unavailable, so Dev Bench instances cannot be discovered.",
    );
  }

  const install = args.install ? resolve(args.install) : undefined;
  const selection = [
    install,
    args.instance === undefined ? undefined : `instance ${args.instance}`,
    args.pid === undefined ? undefined : `PID ${args.pid}`,
  ]
    .filter((value): value is string => value !== undefined)
    .join(", ");
  return {
    label: selection ? `MO2 (${selection})` : "MO2",
    registryDir: join(localAppData, "devbench", "mo2", "instances"),
    install,
    instance: args.instance,
    pid: args.pid,
  };
}

function matchesTarget(
  record: RuntimeIdentity,
  file: string,
  target: Target,
): boolean {
  return (
    Number.isInteger(record.pid) &&
    record.pid > 0 &&
    basename(file) === `${record.pid}.json` &&
    Number.isInteger(record.port) &&
    record.port >= 1 &&
    record.port <= 65535 &&
    typeof record.session === "string" &&
    record.session.length > 0 &&
    typeof record.exe === "string" &&
    isAbsolute(record.exe) &&
    basename(record.exe).toLowerCase() === "modorganizer.exe" &&
    typeof record.install === "string" &&
    typeof record.instance === "string" &&
    (target.pid === undefined || record.pid === target.pid) &&
    (target.install === undefined ||
      pathKey(record.install) === pathKey(target.install)) &&
    (target.instance === undefined ||
      record.instance.toLowerCase() === target.instance.toLowerCase())
  );
}

function sameIdentity(left: RuntimeIdentity, right: RuntimeIdentity): boolean {
  return (
    right.session === left.session &&
    right.pid === left.pid &&
    right.port === left.port &&
    typeof right.exe === "string" &&
    pathKey(right.exe) === pathKey(left.exe)
  );
}

function processRunning(pid: number): boolean {
  try {
    process.kill(pid, 0);
    return true;
  } catch (error) {
    // EPERM names a process this user may not signal, which still runs.
    return (error as NodeJS.ErrnoException).code === "EPERM";
  }
}

export async function resolveInstance(target: Target): Promise<Instance> {
  let files: string[];
  try {
    files = await readdir(target.registryDir);
  } catch (error) {
    if ((error as NodeJS.ErrnoException).code === "ENOENT") {
      throw new Unavailable(`MO2 not running: ${target.label}`);
    }
    throw error;
  }

  const active = await Promise.all(
    files
      .filter((file) => file.endsWith(".json"))
      .map(async (file) => {
        let record: RuntimeIdentity;
        try {
          record = JSON.parse(
            await readFile(join(target.registryDir, file), "utf8"),
          ) as RuntimeIdentity;
        } catch {
          return undefined;
        }
        if (!matchesTarget(record, file, target)) return undefined;
        // A killed MO2 leaves its record behind. Probing those ports queues requests on whichever
        // MO2 holds them now, and enough of them make a live instance miss the timeout.
        if (!processRunning(record.pid)) return undefined;

        const baseUrl = `http://127.0.0.1:${record.port}`;
        try {
          const response = await fetch(`${baseUrl}/api/health`, {
            signal: AbortSignal.timeout(3_000),
          });
          if (!response.ok) return undefined;
          const live = (await response.json()) as RuntimeIdentity;
          if (sameIdentity(record, live))
            return { baseUrl, pid: record.pid, session: record.session };
        } catch {
          return undefined;
        }
        return undefined;
      }),
  );

  const matches = [
    ...new Map(
      active
        .filter((instance): instance is Instance => instance !== undefined)
        .map((instance) => [instance.session, instance]),
    ).values(),
  ];
  if (matches.length === 0) {
    throw new Unavailable(`MO2 not running: ${target.label}`);
  }
  if (matches.length !== 1) {
    const pids = matches.map((instance) => instance.pid).join(", ");
    throw new Error(
      `Multiple live MO2 instances found (PIDs ${pids}). Select one with --install, --instance or --pid.`,
    );
  }
  return matches[0];
}
