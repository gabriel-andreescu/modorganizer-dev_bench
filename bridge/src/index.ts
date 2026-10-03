import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  type CallToolResult,
  ListToolsRequestSchema,
  type Tool,
} from "@modelcontextprotocol/sdk/types.js";
import {
  callTool,
  catalogChangedSince,
  listTools,
  type CatalogTool,
} from "./proxy.ts";
import {
  resolveInstance,
  resolveTarget,
  Unavailable,
  type Target,
} from "./runtime.ts";
import { printSetupSnippet } from "./setup.ts";
import fallback from "./tools-fallback.json" with { type: "json" };

// JSON imports widen schema literals such as `type: "object"` to string.
const fallbackTools = (fallback as { tools: unknown[] }).tools as CatalogTool[];

const CATALOG_POLL_MS = 2_000;

function isToolResult(value: unknown): value is CallToolResult {
  return (
    typeof value === "object" &&
    value !== null &&
    Array.isArray((value as { content?: unknown }).content)
  );
}

function toMcp(tool: CatalogTool): Tool {
  return {
    name: tool.name,
    description: tool.description,
    inputSchema: tool.inputSchema as Tool["inputSchema"],
    annotations: { readOnlyHint: tool.readOnly ?? false },
  };
}

function isCompiledExecutable(): boolean {
  return process.argv[1]?.includes("~BUN") ?? false;
}

function parseArgs(argv: string[]): {
  install?: string;
  instance?: string;
  pid?: number;
  setup: boolean;
} {
  let install: string | undefined;
  let instance: string | undefined;
  let pid: number | undefined;
  let setup = false;
  for (let index = 0; index < argv.length; index++) {
    switch (argv[index]) {
      case "--install":
        install = argv[++index];
        break;
      case "--instance":
        instance = argv[++index];
        break;
      case "--pid":
        pid = Number(argv[++index]);
        break;
      case "setup":
        setup = true;
        break;
    }
  }
  return { install, instance, pid, setup };
}

function watchCatalog(server: Server, target: Target): () => void {
  let instanceKey: string | undefined;
  let sequence = 0;
  let timer: NodeJS.Timeout | undefined;
  let stopped = false;

  const tick = async () => {
    try {
      let changed = false;
      try {
        const instance = await resolveInstance(target);
        const result = await catalogChangedSince(
          instance,
          instance.session === instanceKey ? sequence : 0,
        );
        changed = instance.session !== instanceKey || result.changed;
        instanceKey = instance.session;
        sequence = result.headSeq;
      } catch (error) {
        if (!(error instanceof Unavailable)) return;
        changed = instanceKey !== undefined;
        instanceKey = undefined;
        sequence = 0;
      }
      if (changed) await server.sendToolListChanged();
    } catch (error) {
      console.error(
        `dev-bench-bridge: catalog watch failed: ${(error as Error).message}`,
      );
    } finally {
      if (!stopped)
        timer = setTimeout(() => void tick(), CATALOG_POLL_MS).unref();
    }
  };

  void tick();
  return () => {
    stopped = true;
    clearTimeout(timer);
  };
}

async function main(): Promise<void> {
  const args = parseArgs(process.argv.slice(2));

  if (args.setup) {
    resolveTarget(args);
    printSetupSnippet(
      process.execPath,
      isCompiledExecutable() ? [] : [process.argv[1]],
      args,
    );
    return;
  }

  const target = resolveTarget(args);
  const server = new Server(
    { name: "modorganizer-dev_bench", version: "0.1.1" },
    { capabilities: { tools: { listChanged: true } } },
  );

  server.setRequestHandler(ListToolsRequestSchema, async () => {
    try {
      return { tools: (await listTools(target)).map(toMcp) };
    } catch (error) {
      if (error instanceof Unavailable)
        return { tools: fallbackTools.map(toMcp) };
      throw error;
    }
  });

  server.setRequestHandler(CallToolRequestSchema, async (message, extra) => {
    try {
      const value = await callTool(
        target,
        message.params.name,
        message.params.arguments ?? {},
        extra.signal,
      );
      if (isToolResult(value)) return value;
      return {
        content: [{ type: "text", text: JSON.stringify(value ?? null) }],
      };
    } catch (error) {
      return {
        isError: true,
        content: [
          {
            type: "text",
            text: JSON.stringify({
              ok: false,
              reason: (error as Error).message,
            }),
          },
        ],
      };
    }
  });

  let stopWatch: (() => void) | undefined;
  server.oninitialized = () => {
    stopWatch = watchCatalog(server, target);
  };
  server.onclose = () => stopWatch?.();

  await server.connect(new StdioServerTransport());
}

main().catch((error) => {
  console.error(`dev-bench-bridge: ${(error as Error).message}`);
  process.exit(1);
});
