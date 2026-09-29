import { test } from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, mkdir, writeFile, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import http from "node:http";
import { resolveInstance, resolveTarget, Unavailable } from "../src/runtime.ts";

test("discovery selects one live instance and requires a selector when several match", async () => {
  const root = await mkdtemp(path.join(os.tmpdir(), "mo2-bench-"));
  const records = path.join(root, "devbench", "mo2", "instances");
  await mkdir(records, { recursive: true });
  const servers: http.Server[] = [];

  async function launch(pid: number, install: string, instance: string) {
    const record = {
      install: path.join(root, install),
      instance,
      session: `session-${pid}`,
      pid,
      port: 0,
      exe: path.join(root, install, "ModOrganizer.exe"),
    };
    const server = http.createServer((_request, response) => {
      response.setHeader("Content-Type", "application/json");
      response.end(JSON.stringify(record));
    });
    servers.push(server);
    await new Promise<void>((r) => server.listen(0, "127.0.0.1", r));
    record.port = (server.address() as { port: number }).port;
    await writeFile(path.join(records, `${pid}.json`), JSON.stringify(record));
    return record;
  }
  const target = (
    args: { install?: string; instance?: string; pid?: number } = {},
  ) => resolveTarget({ ...args, localAppData: root });

  try {
    await assert.rejects(resolveInstance(target()), Unavailable);
    const first = await launch(101, "stable", "Skyrim");
    assert.equal((await resolveInstance(target())).session, "session-101");
    await writeFile(
      path.join(records, "102.json"),
      JSON.stringify({ ...first, pid: 102, session: "stale" }),
    );
    assert.equal((await resolveInstance(target())).session, "session-101");

    await launch(103, "stable", "Fallout 4");
    await launch(104, "beta", "Skyrim");
    await assert.rejects(
      resolveInstance(target()),
      /Multiple live MO2 instances/,
    );
    assert.equal(
      (await resolveInstance(target({ instance: "fallout 4" }))).pid,
      103,
    );
    assert.equal(
      (await resolveInstance(target({ install: path.join(root, "beta") }))).pid,
      104,
    );
    assert.equal(
      (
        await resolveInstance(
          target({ install: path.join(root, "stable"), instance: "Skyrim" }),
        )
      ).pid,
      101,
    );
    assert.equal((await resolveInstance(target({ pid: 104 }))).pid, 104);
    await assert.rejects(
      resolveInstance(target({ instance: "Skyrim" })),
      /Multiple live MO2 instances/,
    );
    await assert.rejects(resolveInstance(target({ pid: 999 })), Unavailable);
  } finally {
    for (const server of servers)
      await new Promise<void>((r) => server.close(() => r()));
    await rm(root, { recursive: true, force: true });
  }
});
