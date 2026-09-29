import assert from "node:assert/strict";
import { setTimeout } from "node:timers/promises";

export async function liveClient(address: string) {
  const endpoint = new URL(address);
  const healthResponse = await fetch(new URL("/api/health", endpoint));
  assert.ok(healthResponse.ok);
  const health = await healthResponse.json();

  async function call<T>(
    tool: string,
    args: Record<string, unknown>,
  ): Promise<T> {
    const response = await fetch(new URL(`/api/tool/${tool}`, endpoint), {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        "X-Dev-Bench-Session": health.session,
      },
      body: JSON.stringify(args),
    });
    const result = await response.json();
    assert.ok(response.ok, JSON.stringify(result));
    return result;
  }

  async function operate<T = Record<string, unknown>>(
    tool: string,
    action: string,
    args: Record<string, unknown> = {},
  ): Promise<T> {
    await call(tool, { action, ...args });
    const deadline = Date.now() + 15000;
    while (Date.now() < deadline) {
      const result = await call<{ state: string; error?: string; result?: T }>(
        tool,
        {
          action: "operationStatus",
        },
      );
      assert.notEqual(result.state, "failed", result.error);
      if (result.state === "finished") return result.result!;
      await setTimeout(50);
    }
    throw new Error(`${tool} operation did not finish: ${action}`);
  }

  return { call, operate };
}
