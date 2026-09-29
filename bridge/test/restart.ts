import assert from "node:assert/strict";
import { setTimeout } from "node:timers/promises";
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StdioClientTransport } from "@modelcontextprotocol/sdk/client/stdio.js";
import fallback from "../src/tools-fallback.json" with { type: "json" };

const client = new Client(
  { name: "bridge-restart-check", version: "1" },
  { capabilities: {} },
);
const transport = new StdioClientTransport({
  command: process.argv[2],
  stderr: "pipe",
});
async function inspect() {
  return client.callTool({ name: "inspect", arguments: { action: "health" } });
}
async function until(
  predicate: (result: Awaited<ReturnType<typeof inspect>>) => boolean,
) {
  const deadline = Date.now() + 90000;
  while (Date.now() < deadline) {
    const result = await inspect();
    if (predicate(result)) return result;
    await setTimeout(250);
  }
  throw new Error("Timed out waiting for the manual host transition");
}
try {
  await client.connect(transport);
  const first = await inspect();
  assert.ok(!first.isError);
  const session = JSON.parse(
    (first.content as { text: string }[])[0].text,
  ).session;
  console.log("LIVE", session);
  await until((result) => result.isError === true);
  assert.equal((await client.listTools()).tools.length, fallback.tools.length);
  console.log(
    "OFFLINE: complete catalog remains available on the same MCP connection",
  );
  const next = await until((result) => !result.isError);
  const nextSession = JSON.parse(
    (next.content as { text: string }[])[0].text,
  ).session;
  assert.notEqual(nextSession, session);
  console.log("RECONNECTED", nextSession);
} finally {
  await client.close();
}
