import assert from "node:assert/strict";
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StreamableHTTPClientTransport } from "@modelcontextprotocol/sdk/client/streamableHttp.js";

const endpoint = new URL(process.argv[2]);
const health = await fetch(new URL("/api/health", endpoint)).then((response) =>
  response.json(),
);
const client = new Client(
  { name: "dev-bench-live-check", version: "1" },
  { capabilities: {} },
);
try {
  await client.connect(new StreamableHTTPClientTransport(endpoint));
  const tools = await client.listTools();
  assert.ok(tools.tools.some((tool) => tool.name === "mods"));
  assert.ok(tools.tools.some((tool) => tool.name === "ui"));
  const result = await client.callTool({
    name: "inspect",
    arguments: { action: "health" },
  });
  assert.ok(!result.isError);
  assert.equal(
    JSON.parse((result.content as { text: string }[])[0].text).session,
    health.session,
  );
  console.log(
    JSON.stringify({
      transport: "MCP HTTP",
      tools: tools.tools.length,
      session: health.session,
    }),
  );
} finally {
  await client.close();
}
