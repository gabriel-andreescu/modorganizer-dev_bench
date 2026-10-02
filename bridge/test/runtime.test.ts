import { test } from "node:test";
import assert from "node:assert/strict";
import { spawn, spawnSync, type ChildProcess } from "node:child_process";
import { once } from "node:events";
import { mkdtemp, mkdir, writeFile, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import http from "node:http";
import { resolveInstance, resolveTarget, Unavailable } from "../src/runtime.ts";

// Discovery only probes records whose PID is a running process, so each fake MO2 needs a real one.
async function fixture() {
  const root = await mkdtemp(path.join(os.tmpdir(), "mo2-bench-"));
  const records = path.join(root, "devbench", "mo2", "instances");
  await mkdir(records, { recursive: true });
  const servers: http.Server[] = [];
  const processes: ChildProcess[] = [];
  const probes: number[] = [];

  async function running(): Promise<number> {
    const child = spawn(
      process.execPath,
      ["-e", "setInterval(() => {}, 1000)"],
      { stdio: "ignore" },
    );
    processes.push(child);
    await once(child, "spawn");
    return child.pid!;
  }

  function exited(): number {
    return spawnSync(process.execPath, ["-e", ""]).pid;
  }

  async function write(record: Record<string, unknown> & { pid: number }) {
    await writeFile(
      path.join(records, `${record.pid}.json`),
      JSON.stringify(record),
    );
  }

  async function launch(install: string, instance: string) {
    const pid = await running();
    const record = {
      install: path.join(root, install),
      instance,
      session: `session-${pid}`,
      pid,
      port: 0,
      exe: path.join(root, install, "ModOrganizer.exe"),
    };
    const server = http.createServer((_request, response) => {
      probes.push(pid);
      response.setHeader("Content-Type", "application/json");
      response.end(JSON.stringify(record));
    });
    servers.push(server);
    await new Promise<void>((r) => server.listen(0, "127.0.0.1", r));
    record.port = (server.address() as { port: number }).port;
    await write(record);
    return record;
  }

  async function close() {
    for (const child of processes) child.kill();
    for (const server of servers)
      await new Promise<void>((r) => server.close(() => r()));
    await rm(root, { recursive: true, force: true });
  }

  const target = (
    args: { install?: string; instance?: string; pid?: number } = {},
  ) => resolveTarget({ ...args, localAppData: root });

  return { root, probes, running, exited, write, launch, close, target };
}

test("discovery selects one live instance and requires a selector when several match", async () => {
  const { root, running, write, launch, close, target } = await fixture();
  try {
    await assert.rejects(resolveInstance(target()), Unavailable);
    const first = await launch("stable", "Skyrim");
    assert.equal((await resolveInstance(target())).pid, first.pid);
    await write({ ...first, pid: await running(), session: "stale" });
    assert.equal((await resolveInstance(target())).pid, first.pid);

    const second = await launch("stable", "Fallout 4");
    const third = await launch("beta", "Skyrim");
    await assert.rejects(
      resolveInstance(target()),
      /Multiple live MO2 instances/,
    );
    assert.equal(
      (await resolveInstance(target({ instance: "fallout 4" }))).pid,
      second.pid,
    );
    assert.equal(
      (await resolveInstance(target({ install: path.join(root, "beta") }))).pid,
      third.pid,
    );
    assert.equal(
      (
        await resolveInstance(
          target({ install: path.join(root, "stable"), instance: "Skyrim" }),
        )
      ).pid,
      first.pid,
    );
    assert.equal(
      (await resolveInstance(target({ pid: third.pid }))).pid,
      third.pid,
    );
    await assert.rejects(
      resolveInstance(target({ instance: "Skyrim" })),
      /Multiple live MO2 instances/,
    );
    await assert.rejects(resolveInstance(target({ pid: 999 })), Unavailable);
  } finally {
    await close();
  }
});

test("discovery does not probe records of exited processes", async () => {
  const { probes, exited, write, launch, close, target } = await fixture();
  try {
    const live = await launch("stable", "Skyrim");
    for (let stale = 0; stale < 3; ++stale)
      await write({ ...live, pid: exited(), session: `stale-${stale}` });

    assert.equal((await resolveInstance(target())).pid, live.pid);
    assert.deepEqual(probes, [live.pid]);
  } finally {
    await close();
  }
});
