import { test } from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, mkdir, writeFile, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import http from "node:http";
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StdioClientTransport } from "@modelcontextprotocol/sdk/client/stdio.js";
import fallback from "../src/tools-fallback.json" with { type: "json" };

const richResult = {
  content: [{ type: "image", mimeType: "image/png", data: "iVBORw0KGgo=" }],
  structuredContent: { windows: [{ title: "MO2 fixture" }] },
};

test("one MCP session survives offline startup, host exit and a new host session", async () => {
  const root = await mkdtemp(path.join(os.tmpdir(), "mo2-bridge-"));
  const records = path.join(root, "devbench", "mo2", "instances");
  await mkdir(records, { recursive: true });
  const environment = Object.fromEntries(
    Object.entries(process.env).filter(
      (entry): entry is [string, string] => entry[1] !== undefined,
    ),
  );
  environment.LOCALAPPDATA = root;
  const transport = new StdioClientTransport({
    command: process.execPath,
    args: ["--import", "tsx", "src/index.ts"],
    env: environment,
    stderr: "pipe",
  });
  const client = new Client(
    { name: "bridge-test", version: "1" },
    { capabilities: {} },
  );
  let server: http.Server | undefined;
  async function launch(session: string) {
    const record = {
      install: root,
      instance: "Fixture",
      session,
      pid: 777,
      port: 0,
      exe: path.join(root, "ModOrganizer.exe"),
    };
    server = http.createServer((request, response) => {
      response.setHeader("Content-Type", "application/json");
      if (request.url === "/api/health") response.end(JSON.stringify(record));
      else if (request.url === "/api/tools")
        response.end(JSON.stringify(fallback));
      else if (request.url?.startsWith("/api/events"))
        response.end(JSON.stringify({ headSeq: 0, events: [] }));
      else {
        assert.equal(request.headers["x-dev-bench-session"], session);
        response.end(
          JSON.stringify(
            request.url === "/api/tool/capture" ? richResult : { session },
          ),
        );
      }
    });
    await new Promise<void>((r) => server!.listen(0, "127.0.0.1", r));
    record.port = (server.address() as any).port;
    await writeFile(
      path.join(records, `${record.pid}.json`),
      JSON.stringify(record),
    );
  }
  async function close() {
    if (server) {
      server.closeAllConnections();
      await new Promise<void>((r) => server!.close(() => r()));
      server = undefined;
    }
  }
  try {
    await client.connect(transport);
    const tools = await client.listTools();
    assert.equal(tools.tools.length, fallback.tools.length);
    assert.equal(
      (await client.callTool({ name: "inspect", arguments: {} })).isError,
      true,
    );
    await launch("one");
    assert.match(
      JSON.stringify(await client.callTool({ name: "inspect", arguments: {} })),
      /one/,
    );
    assert.deepEqual(
      await client.callTool({ name: "capture", arguments: {} }),
      richResult,
    );
    await close();
    assert.equal(
      (await client.callTool({ name: "inspect", arguments: {} })).isError,
      true,
    );
    await launch("two");
    assert.match(
      JSON.stringify(await client.callTool({ name: "inspect", arguments: {} })),
      /two/,
    );
  } finally {
    await client.close();
    await close();
    await rm(root, { recursive: true, force: true });
  }
});
